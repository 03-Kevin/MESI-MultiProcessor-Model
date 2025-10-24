#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>

#include "include/config.h"
#include <pthread.h>

// Memoria principal compartida
extern double main_memory[MEM_SIZE];

// Mutex para proteger el acceso a la memoria
extern pthread_mutex_t mem_lock;

// Inicializa la memoria principal y el mutex
void mem_init();

// Lee un valor de la memoria principal dado un segmento y un offset
double mem_read(Segment seg, int offset);

// Escribe un valor en la memoria principal dado un segmento y un offset
void mem_write(Segment seg, int offset, double value);

// Convierte una dirección global en un segmento de memoria
Segment addr_to_segment(int addr);

// Carga datos en memoria con alineación (definición pública unificada)
// Devuelve 0 en éxito, -1 en error.
int mem_load_data(Segment seg, int offset, const double *data, size_t count, int alignment);

#endif // MEMORY_H
