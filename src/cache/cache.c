#include "cache.h"
#include <stdio.h>

void cache_init(Cache* cache) {
    for (int i = 0; i < SETS; i++)
        for (int j = 0; j < WAYS; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].state = I;
        }
}

double cache_read(Cache* cache, int addr, int pe_id) {
    int set_index = addr % SETS;
    unsigned long tag = addr / SETS;
    CacheSet* set = &cache->sets[set_index];

    // Buscar hit
    for (int i = 0; i < WAYS; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            printf("[PE%d] HIT en set %d (way %d)\n", pe_id, set_index, i);
            return set->lines[i].data[0]; // simplificación, es necesario manejar el offset
        }
    }

    // Miss → contactar bus
    printf("[PE%d] MISS en set %d, pidiendo BusRd\n", pe_id, set_index);
    bus_broadcast(BUS_RD, addr, pe_id);

    // Simular lectura desde memoria principal
    double val = mem_read(addr);
    set->lines[0].valid = 1;
    set->lines[0].tag = tag;
    set->lines[0].state = E;
    set->lines[0].data[0] = val;
    return val;
}

void cache_write(Cache* cache, int addr, double value, int pe_id) {
    int set_index = addr % SETS;
    unsigned long tag = addr / SETS;
    CacheSet* set = &cache->sets[set_index];

    // Buscar si ya está en caché
    int found = 0;
    for (int i = 0; i < WAYS; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            printf("[PE%d] WRITE hit en set %d\n", pe_id, set_index);
            set->lines[i].data[0] = value;
            set->lines[i].state = M;
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[PE%d] WRITE miss en set %d, enviando BusRdX\n", pe_id, set_index);
        bus_broadcast(BUS_RDX, addr, pe_id);
        set->lines[0].valid = 1;
        set->lines[0].tag = tag;
        set->lines[0].state = M;
        set->lines[0].data[0] = value;
    }

    // Política write-back: actualizar memoria solo si está en M al reemplazar
    mem_write(addr, value);
}
