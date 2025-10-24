// src/bus/handlers.c
#include "handlers.h"
#include "../memory/memory.h"
#include "../cache/cache.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../mesi/mesi.h"

/* Registra los handlers en la tabla del bus */
void bus_register_handlers(Bus* bus) {
    bus->handlers[BUS_RD]   = handle_busrd;
    bus->handlers[BUS_RDX]  = handle_busrdx;
    bus->handlers[BUS_UPGR] = handle_busupgr;
    bus->handlers[BUS_WB]   = handle_buswb;
}

static int compute_block(int addr) { return addr / DOUBLES_PER_LINE; }
static int compute_set(int block) { return block % SETS; }

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

void handle_busrd(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Handler BusRd (PE%d, addr=%d)\n", src_pe, addr);
    bus->last_shared = 0;
    int block = compute_block(addr);

    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;

        Cache *other_cache = bus->caches[i];
        if (!other_cache) continue;

        /* Proteger acceso a la cache del PE i */
        pthread_mutex_lock(&other_cache->lock);
        CacheLine* line = cache_get_line(other_cache, addr);
        if (line && line->valid) {
            bus->last_shared = 1;
            if (line->state == M) {
                unsigned long block_num = line->tag * SETS + compute_set(block);
                int base = (int)(block_num * DOUBLES_PER_LINE);
                for (unsigned long off = 0; off < DOUBLES_PER_LINE; off++) {
                    int phys = base + (int)off;
                    Segment seg = addr_to_segment(phys);
                    int seg_base = seg_base_addr(seg);
                    int offset_in_seg = phys - seg_base;
                    if (offset_in_seg < 0) {
                        fprintf(stderr, "Error: Escritura fuera de los límites en handle_busrd (base=%d, offset=%lu)\n", base, off);
                        pthread_mutex_unlock(&other_cache->lock);
                        exit(EXIT_FAILURE);
                    }
                    mem_write(seg, offset_in_seg, line->data[off]);
                }
                line->dirty = 0;
                line->state = S;
                other_cache->writebacks++; /* contabiliza writeback localmente */
                printf("[BUS] PE%d: M -> S y writeback addr(base)=%d\n", i, base);
            } else if (line->state == E) {
                line->state = S;
                printf("[BUS] PE%d: E -> S addr=%d\n", i, addr);
            }
        }
        pthread_mutex_unlock(&other_cache->lock);
    }

    if (!bus->last_shared) {
        printf("[BUS] Ninguna cache tenía la línea; responder desde memoria\n");
    }
}

void handle_busrdx(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Handler BusRdX (PE%d, addr=%d)\n", src_pe, addr);
    bus->last_shared = 0;
    int block = compute_block(addr);

    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;

        Cache *other_cache = bus->caches[i];
        if (!other_cache) continue;

        pthread_mutex_lock(&other_cache->lock);
        CacheLine* line = cache_get_line(other_cache, addr);
        if (line && line->valid) {
            bus->last_shared = 1;
            if (line->state == M) {
                unsigned long block_num = line->tag * SETS + compute_set(block);
                int base = (int)(block_num * DOUBLES_PER_LINE);
                for (unsigned long off = 0; off < DOUBLES_PER_LINE; off++) {
                    int phys = base + (int)off;
                    Segment seg = addr_to_segment(phys);
                    int seg_base = seg_base_addr(seg);
                    int offset_in_seg = phys - seg_base;
                    if (offset_in_seg < 0) {
                        fprintf(stderr, "Error: Escritura fuera de los límites en handle_busrdx (base=%d, offset=%lu)\n", base, off);
                        pthread_mutex_unlock(&other_cache->lock);
                        exit(EXIT_FAILURE);
                    }
                    mem_write(seg, offset_in_seg, line->data[off]);
                }
                line->dirty = 0;
                other_cache->writebacks++;
                printf("[BUS] PE%d: writeback por BusRdX (base=%d)\n", i, base);
            }
            line->state = I;
            line->valid = 0;
            other_cache->invalidations_received++;
            printf("[BUS] PE%d: invalidada addr=%d\n", i, addr);
        }
        pthread_mutex_unlock(&other_cache->lock);
    }
}

void handle_busupgr(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Handler BusUpgr (PE%d, addr=%d)\n", src_pe, addr);
    int block = compute_block(addr);

    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;

        Cache *other_cache = bus->caches[i];
        if (!other_cache) continue;

        pthread_mutex_lock(&other_cache->lock);
        CacheLine* line = cache_get_line(other_cache, addr);
        if (line && line->valid) {
            printf("[BUS] PE%d: %s -> I para addr=%d\n", i, mesi_state_to_str(line->state), addr);
            line->state = I;
            line->valid = 0;
            other_cache->invalidations_received++;
        }
        pthread_mutex_unlock(&other_cache->lock);
    }
}

void handle_buswb(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Handler BusWB (PE%d, addr=%d)\n", src_pe, addr);

    Cache *src_cache = bus->caches[src_pe];
    if (!src_cache) return;

    /* Protegemos la cache del emisor (por si acaso) */
    pthread_mutex_lock(&src_cache->lock);
    CacheLine* line = cache_get_line(src_cache, addr);
    if (line && line->state == M) {
        int block = compute_block(addr);
        unsigned long block_num = line->tag * SETS + compute_set(block);
        int base = (int)(block_num * DOUBLES_PER_LINE);
        for (unsigned long off = 0; off < DOUBLES_PER_LINE; off++) {
            int phys = base + (int)off;
            Segment seg = addr_to_segment(phys);
            int seg_base = seg_base_addr(seg);
            int offset_in_seg = phys - seg_base;
            if (offset_in_seg < 0) {
                fprintf(stderr, "Error: Escritura fuera de los límites en handle_buswb (base=%d, offset=%lu)\n", base, off);
                pthread_mutex_unlock(&src_cache->lock);
                exit(EXIT_FAILURE);
            }
            mem_write(seg, offset_in_seg, line->data[off]);
        }
        src_cache->writebacks++;
        line->dirty = 0;
        line->state = S;
        printf("[BUS] PE%d: M -> S luego de WB base=%d\n", src_pe, base);
    }
    pthread_mutex_unlock(&src_cache->lock);
}
