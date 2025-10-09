#include "pe.h"
#include <stdio.h>
#include <unistd.h>

void* pe_run(void* arg) {
    PE* pe = (PE*)arg;
    printf("[PE%d] Starting thread...\n", pe->id);
    cache_write(&pe->cache, pe->id * 4, 3.14 * (pe->id + 1), pe->id);
    cache_read(&pe->cache, pe->id * 4, pe->id);
    sleep(1);
    printf("[PE%d] Finished.\n", pe->id);
    return NULL;
}
