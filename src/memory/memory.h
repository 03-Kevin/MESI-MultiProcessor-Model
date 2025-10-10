#ifndef MEMORY_H
#define MEMORY_H

#include "include/config.h"
#include <pthread.h>

extern double main_memory[MEM_SIZE];
extern pthread_mutex_t mem_lock;

void mem_init();
double mem_read(int addr);
void mem_write(int addr, double value);

#endif
