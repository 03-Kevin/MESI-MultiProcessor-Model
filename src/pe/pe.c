#include "pe.h"
#include <stdio.h>

void* pe_run(void* arg) {
    PE* pe = (PE*)arg;
    int addr = pe->id * 4; // Dirección única por PE

    printf("[PE%d] Starting thread...\n", pe->id);

    // Escribir en la dirección única
    double value_to_write = pe->id * 1.0; // Valor único por PE
    cache_write(pe->cache, addr, value_to_write, pe->id);
    printf("[PE%d] Escribió %f en MEM[%d]\n", pe->id, value_to_write, addr);

    printf("[PE%d] Finished.\n", pe->id);
    return NULL;
}