#include "cache.h"
#include "bus/bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Inicializar caché
void cache_init(Cache *cache)
{
    cache->bus = NULL;
    for (int i = 0; i < SETS; i++)
        for (int j = 0; j < WAYS; j++)
        {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].state = I;
        }
}

// Encuentra línea libre o usa política de reemplazo simple
int find_line_to_replace(CacheSet *set)
{
    for (int i = 0; i < WAYS; i++)
        if (!set->lines[i].valid)
            return i;
    return 0; // Si todas están ocupadas, reemplaza la primera
}

// Obtener línea específica de una caché
CacheLine *cache_get_line(Cache *cache, int addr)
{
    int set_index = (addr / LINE_SIZE) % SETS; // Corregido
    unsigned long tag = addr / (SETS * LINE_SIZE); // Corregido
    CacheSet *set = &cache->sets[set_index];

    for (int i = 0; i < WAYS; i++)
    {
        if (set->lines[i].valid && set->lines[i].tag == tag)
            return &set->lines[i];
    }
    return NULL;
}

double cache_read(Cache *cache, int addr, int pe_id)
{
    pthread_mutex_lock(&cache_mutex);

    int set_index = (addr / LINE_SIZE) % SETS; // Corregido
    unsigned long tag = addr / (SETS * LINE_SIZE); // Corregido
    CacheSet *set = &cache->sets[set_index];

    // Revisar hits
    for (int i = 0; i < WAYS; i++)
    {
        CacheLine *line = &set->lines[i];
        if (line->valid && line->tag == tag)
        {
            printf("[PE%d] HIT en set %d (way %d)\n", pe_id, set_index, i);

            // Verificar si la línea ya está en estado Shared o Exclusive
            if (line->state == S || line->state == E || line->state == M)
            {
                pthread_mutex_unlock(&cache_mutex);
                return line->data[0];
            }
        }
    }

    // Miss → pedir BusRd
    printf("[PE%d] MISS en set %d, pidiendo BusRd\n", pe_id, set_index);
    bus_broadcast(cache->bus, BUS_RD, addr, pe_id);

    // Leer desde memoria principal
    double val = mem_read(addr);

    // Reemplazar línea
    int way = find_line_to_replace(set);
    CacheLine *line = &set->lines[way];
    line->valid = 1;
    line->tag = tag; // Corregido
    line->state = S; // Shared
    line->data[0] = val;
    printf("[CACHE] Transición de estado: Invalid -> Shared\n");

    pthread_mutex_unlock(&cache_mutex);
    return val;
}

// Escribir en caché
void cache_write(Cache *cache, int addr, double value, int pe_id)
{
    pthread_mutex_lock(&cache_mutex);

    int set_index = (addr / LINE_SIZE) % SETS; // Corregido
    unsigned long tag = addr / (SETS * LINE_SIZE); // Corregido
    CacheSet *set = &cache->sets[set_index];

    CacheLine *line = cache_get_line(cache, addr);
    if (line)
    {
        printf("[PE%d] WRITE hit en set %d\n", pe_id, set_index);

        if (line->state == S || line->state == E)
        {
            line->state = M; // S/E -> M
            printf("[CACHE] Transición de estado: %s -> Modified\n", mesi_state_to_str(S));
        }

        line->data[0] = value;
    }
    else
    {
        printf("[PE%d] WRITE miss en set %d, enviando BusRdX\n", pe_id, set_index);
        bus_broadcast(cache->bus, BUS_RDX, addr, pe_id);

        int way = find_line_to_replace(set);
        line = &set->lines[way];
        line->valid = 1;
        line->tag = tag; // Corregido
        line->state = M;
        line->data[0] = value;

        // Invalidar otras cachés
        for (int i = 0; i < NUM_PES; i++)
        {
            if (cache->bus->caches[i] != cache)
            {
                CacheLine *other_line = cache_get_line(cache->bus->caches[i], addr);
                if (other_line && other_line->state != I)
                {
                    other_line->state = I;
                    printf("[CACHE] Invalidando PE%d para addr=%d\n", i, addr);
                }
            }
        }
    }

    pthread_mutex_unlock(&cache_mutex);
}