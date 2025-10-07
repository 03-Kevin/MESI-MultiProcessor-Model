#include "memory.h"
#include <stdio.h>

double main_memory[MEM_SIZE];
pthread_mutex_t mem_lock;

void mem_init() {
    pthread_mutex_init(&mem_lock, NULL);
    for (int i = 0; i < MEM_SIZE; i++)
        main_memory[i] = 0.0;
}

double mem_read(int addr) {
    pthread_mutex_lock(&mem_lock);
    double val = main_memory[addr];
    pthread_mutex_unlock(&mem_lock);
    return val;
}

void mem_write(int addr, double value) {
    pthread_mutex_lock(&mem_lock);
    main_memory[addr] = value;
    pthread_mutex_unlock(&mem_lock);
}
