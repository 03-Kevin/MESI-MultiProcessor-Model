#ifndef CACHE_H
#define CACHE_H

#include "mesi.h"
#include "bus.h"
#include "memory.h"

#define SETS 16
#define WAYS 2
#define BLOCK_SIZE 4 // 4 doubles (32 bytes)

typedef struct {
    unsigned long tag;
    MESI_State state;
    double data[BLOCK_SIZE];
    int valid;
} CacheLine;

typedef struct {
    CacheLine lines[WAYS];
} CacheSet;

typedef struct {
    CacheSet sets[SETS];
} Cache;

void cache_init(Cache* cache);
double cache_read(Cache* cache, int addr, int pe_id);
void cache_write(Cache* cache, int addr, double value, int pe_id);

#endif
