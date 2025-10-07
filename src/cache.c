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
    printf("[PE%d] READ from set %d\n", pe_id, set_index);
    return 0.0; // por ahora retorna dummy
}

void cache_write(Cache* cache, int addr, double value, int pe_id) {
    int set_index = addr % SETS;
    printf("[PE%d] WRITE to set %d value %.2f\n", pe_id, set_index, value);
}
