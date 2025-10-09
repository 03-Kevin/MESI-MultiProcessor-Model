#ifndef PE_H
#define PE_H

#include "cache/cache.h"
#include <pthread.h>

typedef struct {
    int id;
    double regs[8];
    Cache cache;
} PE;

void* pe_run(void* arg);

#endif
