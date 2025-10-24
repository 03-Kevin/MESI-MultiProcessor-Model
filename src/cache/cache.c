// src/cache/cache.c
#include "cache.h"
#include "../memory/memory.h"
#include "../bus/bus.h"
#include "../include/config.h"
#include "../mesi/mesi.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// forward
static int find_line_to_replace(CacheSet *set, Cache *cache, int set_index, int *way_out);
static void record_transition(Cache* cache, MESI_State st);

/* ------------------ Helpers de mapeo ------------------ */

static int compute_block(int addr) {
    return addr / DOUBLES_PER_LINE;
}
static int compute_set(int block) {
    return block % SETS;
}
static unsigned long compute_tag(int block) {
    return (unsigned long)(block / SETS);
}
static int compute_offset(int addr) {
    return addr % DOUBLES_PER_LINE;
}

static int reconstruct_base(unsigned long tag, int set_index) {
    unsigned long block_num = tag * (unsigned long)SETS + (unsigned long)set_index;
    return (int)(block_num * (unsigned long)DOUBLES_PER_LINE);
}

/* helper: devuelve la dirección base (física) del segmento */
static int seg_base_addr(Segment seg) {
    switch (seg) {
        case VECTOR_A: return VECTOR_A_ADDR;
        case VECTOR_B: return VECTOR_B_ADDR;
        case SUMS:     return SUMS_ADDR;
        case DONE:     return DONE_ADDR;
        case RESULT:   return RESULT_ADDR;
        default:       return 0;
    }
}

/* ------------------ Inicialización ------------------ */

void cache_init(Cache *cache) {
    if (!cache) return;
    cache->bus = NULL;
    for (int i = 0; i < SETS; i++)
        for (int j = 0; j < WAYS; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].state = I;
            cache->sets[i].lines[j].dirty = 0;
            cache->sets[i].lines[j].age = 0;
            cache->sets[i].lines[j].tag = 0;
            for (unsigned long k = 0; k < DOUBLES_PER_LINE; k++)
                cache->sets[i].lines[j].data[k] = 0.0;
        }
    pthread_mutex_init(&cache->lock, NULL);
    cache->read_misses = cache->write_misses = cache->invalidations_received = 0;
    cache->writebacks = 0;
    cache->total_reads = cache->total_writes = cache->read_hits = cache->write_hits = 0;
    for (int i = 0; i < 4; i++) cache->transitions[i] = 0;
}

/* ------------------ Acceso y reemplazo ------------------ */

CacheLine* cache_get_line(Cache *cache, int addr) {
    int block = compute_block(addr);
    int set_index = compute_set(block);
    unsigned long tag = compute_tag(block);
    CacheSet *set = &cache->sets[set_index];

    for (int i = 0; i < WAYS; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            return &set->lines[i];
        }
    }
    return NULL;
}

static void writeback_line(Cache *cache, CacheLine *line, int set_index) {
    if (!line || !line->valid) return;
    if (line->dirty || line->state == M) {
        int base = reconstruct_base(line->tag, set_index);
        for (unsigned long off = 0; off < DOUBLES_PER_LINE; off++) {
            int phys = base + (int)off;
            Segment seg = addr_to_segment(phys);
            int seg_base = seg_base_addr(seg);
            int offset_in_seg = phys - seg_base;
            if (offset_in_seg < 0) {
                fprintf(stderr, "[CACHE] Error: offset_in_seg negativo en writeback (phys=%d seg_base=%d)\n", phys, seg_base);
                exit(EXIT_FAILURE);
            }
            mem_write(seg, offset_in_seg, line->data[off]);
        }
        cache->writebacks++;
        line->dirty = 0;
        printf("[CACHE] Writeback de block (tag=%lu set=%d) (base addr %d)\n", line->tag, set_index, base);
    }
}

static int find_line_to_replace(CacheSet *set, Cache *cache, int set_index, int *way_out) {
    int lru_index = 0;
    int max_age = -1;

    for (int i = 0; i < WAYS; i++) {
        if (!set->lines[i].valid) {
            *way_out = i;
            return i;
        }
        if ((int)set->lines[i].age > max_age) {
            max_age = (int)set->lines[i].age;
            lru_index = i;
        }
    }

    *way_out = lru_index;
    writeback_line(cache, &set->lines[lru_index], set_index);
    set->lines[lru_index].valid = 0;
    set->lines[lru_index].dirty = 0;
    set->lines[lru_index].state = I;
    set->lines[lru_index].age = 0;
    return lru_index;
}

static void update_lru(CacheSet *set, int accessed_index) {
    for (int i = 0; i < WAYS; i++) {
        if (set->lines[i].valid) {
            set->lines[i].age++;
        }
    }
    if (accessed_index >= 0 && accessed_index < WAYS)
        set->lines[accessed_index].age = 0;
}

static void record_transition(Cache* cache, MESI_State st) {
    if (!cache) return;
    if (st >= 0 && st <= 3) cache->transitions[st]++;
}


/* ------------------ Operaciones públicas ------------------ */

double cache_read(Cache *cache, int addr, int pe_id) {
    if (!cache) return 0.0;

    pthread_mutex_lock(&cache->lock);
    cache->total_reads++;

    int block = compute_block(addr);
    int set_index = compute_set(block);
    unsigned long tag = compute_tag(block);
    int offset = compute_offset(addr);
    CacheSet *set = &cache->sets[set_index];

    /* Buscar hit */
    for (int i = 0; i < WAYS; i++) {
        CacheLine *line = &set->lines[i];
        if (line->valid && line->tag == tag) {
            cache->read_hits++;
            printf("[PE%d] CACHE HIT: Addr=%d Set=%d Way=%d State=%s\n",
                   pe_id, addr, set_index, i, mesi_state_to_str(line->state));
            double val = line->data[offset];
            update_lru(set, i);
            pthread_mutex_unlock(&cache->lock);
            return val;
        }
    }

    /* MISS: registrar, liberar lock y hacer broadcast */
    cache->read_misses++;
    printf("[PE%d] CACHE MISS: Addr=%d Set=%d -> BusRd\n", pe_id, addr, set_index);

    /* Liberar lock antes de emitir la petición al bus (evita deadlocks) */
    pthread_mutex_unlock(&cache->lock);

    bus_broadcast(cache->bus, BUS_RD, addr, pe_id);

    /* Volver a tomar lock para completar la carga de la línea */
    pthread_mutex_lock(&cache->lock);

    /* Recomputar set (puede haber cambiado por otras acciones) */
    set = &cache->sets[set_index];

    int way;
    find_line_to_replace(set, cache, set_index, &way);
    CacheLine *line = &set->lines[way];
    line->valid = 1;
    line->tag = tag;
    line->dirty = 0;

    if (cache->bus && cache->bus->last_shared) {
        line->state = S;
        record_transition(cache, S);
    } else {
        line->state = E;
        record_transition(cache, E);
    }

    int base = reconstruct_base(tag, set_index);
    for (unsigned long o = 0; o < DOUBLES_PER_LINE; o++) {
        int phys = base + (int)o;
        Segment seg = addr_to_segment(phys);
        int seg_base = seg_base_addr(seg);
        int offset_in_seg = phys - seg_base;
        line->data[o] = mem_read(seg, offset_in_seg);
    }

    update_lru(set, way);
    double val = line->data[offset];
    pthread_mutex_unlock(&cache->lock);
    return val;
}

void cache_write(Cache *cache, int addr, double value, int pe_id) {
    if (!cache) return;
    pthread_mutex_lock(&cache->lock);
    cache->total_writes++;

    int block = compute_block(addr);
    int set_index = compute_set(block);
    unsigned long tag = compute_tag(block);
    int offset = compute_offset(addr);
    CacheSet *set = &cache->sets[set_index];

    CacheLine *line = cache_get_line(cache, addr);
    if (line) {
        cache->write_hits++;
        /* Caso hit */
        if (line->state == S) {
            /* Necesitamos BusUpgr: liberar lock, emitir, y luego reacceder con lock */
            pthread_mutex_unlock(&cache->lock);
            bus_broadcast(cache->bus, BUS_UPGR, addr, pe_id);
            pthread_mutex_lock(&cache->lock);

            /* Revalidar que la línea sigue ahí (si no, caer al manejo de miss) */
            line = cache_get_line(cache, addr);
            if (!line || !line->valid || line->tag != tag) {
                /* Caer en camino de miss: no hay línea válida tras BusUpgr */
                cache->write_misses++;
                pthread_mutex_unlock(&cache->lock);
                /* Emitir BusRdX para obtener la línea en M */
                bus_broadcast(cache->bus, BUS_RDX, addr, pe_id);
                pthread_mutex_lock(&cache->lock);
                /* Reemplazar e inicializar como en miss */
                int way;
                find_line_to_replace(set, cache, set_index, &way);
                line = &set->lines[way];
                line->valid = 1;
                line->tag = tag;
                line->state = M;
                record_transition(cache, M);
                line->dirty = 0;
                int base = reconstruct_base(tag, set_index);
                for (unsigned long o = 0; o < DOUBLES_PER_LINE; o++) {
                    int phys = base + (int)o;
                    Segment seg = addr_to_segment(phys);
                    int seg_base = seg_base_addr(seg);
                    int offset_in_seg = phys - seg_base;
                    line->data[o] = mem_read(seg, offset_in_seg);
                }
                line->data[offset] = value;
                line->dirty = 1;
                update_lru(set, way);
                pthread_mutex_unlock(&cache->lock);
                return;
            }
            /* Si la línea sigue válida, la promovemos a M */
            line->state = M;
            record_transition(cache, M);
        } else if (line->state == E) {
            line->state = M;
            record_transition(cache, M);
        }
        line->data[offset] = value;
        line->dirty = 1;
        update_lru(set, (int)(line - set->lines));
        pthread_mutex_unlock(&cache->lock);
        return;
    }

    /* Miss de escritura */
    cache->write_misses++;
    printf("[PE%d] CACHE WRITE MISS: Addr=%d Set=%d -> BusRdX\n", pe_id, addr, set_index);

    /* liberar lock antes de emitir BusRdX */
    pthread_mutex_unlock(&cache->lock);
    bus_broadcast(cache->bus, BUS_RDX, addr, pe_id);

    /* volver a tomar lock y completar la línea en estado M */
    pthread_mutex_lock(&cache->lock);
    int way;
    find_line_to_replace(set, cache, set_index, &way);
    line = &set->lines[way];
    line->valid = 1;
    line->tag = tag;
    line->state = M;
    record_transition(cache, M);
    line->dirty = 0;

    int base = reconstruct_base(tag, set_index);
    for (unsigned long o = 0; o < DOUBLES_PER_LINE; o++) {
        int phys = base + (int)o;
        Segment seg = addr_to_segment(phys);
        int seg_base = seg_base_addr(seg);
        int offset_in_seg = phys - seg_base;
        line->data[o] = mem_read(seg, offset_in_seg);
    }

    line->data[offset] = value;
    line->dirty = 1;
    update_lru(set, way);
    pthread_mutex_unlock(&cache->lock);
}
/* ------------------ Flush / Debug / Métricas ------------------ */

void cache_flush(Cache *cache) {
    if (!cache) return;
    pthread_mutex_lock(&cache->lock);

    for (int set_index = 0; set_index < SETS; set_index++) {
        CacheSet *set = &cache->sets[set_index];
        for (int way = 0; way < WAYS; way++) {
            CacheLine *line = &set->lines[way];
            if (!line->valid) continue;
            if (line->dirty || line->state == M) {
                int base = reconstruct_base(line->tag, set_index);
                for (unsigned long off = 0; off < DOUBLES_PER_LINE; off++) {
                    int phys = base + (int)off;
                    Segment seg = addr_to_segment(phys);
                    int seg_base = seg_base_addr(seg);
                    int offset_in_seg = phys - seg_base;
                    mem_write(seg, offset_in_seg, line->data[off]);
                }
                cache->writebacks++;
                line->dirty = 0;
            }
            line->valid = 0;
            line->state = I;
            line->age = 0;
            line->tag = 0;
        }
    }

    pthread_mutex_unlock(&cache->lock);
}

void cache_print_metrics(Cache *cache, int pe_id) {
    if (!cache) return;

    printf("\n[MÉTRICAS - PE%d]\n", pe_id);
    printf("Lecturas fallidas (read misses): %lu\n", cache->read_misses);
    printf("Escrituras fallidas (write misses): %lu\n", cache->write_misses);
    printf("Writebacks realizados: %lu\n", cache->writebacks);
    printf("Invalidaciones recibidas: %lu\n", cache->invalidations_received);
    printf("Transiciones de estado MESI:\n");
    printf("  - Modified (M): %lu\n", cache->transitions[M]);
    printf("  - Exclusive (E): %lu\n", cache->transitions[E]);
    printf("  - Shared (S): %lu\n", cache->transitions[S]);
    printf("  - Invalid (I): %lu\n", cache->transitions[I]);
}

void debug_print_cache_states(Cache *caches, int num_pes) {
    printf("\n[DEBUG] Estados finales de las cachés:\n");
    for (int i = 0; i < num_pes; i++) {
        Cache *cache = &caches[i];
        printf("[DEBUG] Caché PE%d:\n", i);
        for (int set_index = 0; set_index < SETS; set_index++) {
            CacheSet *set = &cache->sets[set_index];
            for (int way = 0; way < WAYS; way++) {
                CacheLine *line = &set->lines[way];
                if (line->valid) {
                    int phys = (int)((line->tag * SETS + set_index) * DOUBLES_PER_LINE);
                    printf("  Set %d, Way %d: Addr=%d, State=%s\n",
                           set_index, way, phys, mesi_state_to_str(line->state));
                }
            }
        }
    }
}
void cache_record_transition(Cache *cache, MESI_State st) {
    if (!cache) return;
    pthread_mutex_lock(&cache->lock);
    if (st >= 0 && st <= 3) cache->transitions[st]++;
    pthread_mutex_unlock(&cache->lock);
}

void cache_increment_writebacks(Cache *cache) {
    if (!cache) return;
    pthread_mutex_lock(&cache->lock);
    cache->writebacks++;
    pthread_mutex_unlock(&cache->lock);
}
