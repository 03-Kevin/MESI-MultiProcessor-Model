#ifndef MEMORY_H
#define MEMORY_H

#include <pthread.h>

#define MEM_SIZE 512

extern double main_memory[MEM_SIZE];
extern pthread_mutex_t mem_lock;

void mem_init();
double mem_read(int addr);
void mem_write(int addr, double value);

#endif
