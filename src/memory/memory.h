#ifndef MEMORY_H
#define MEMORY_H

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

// Carga datos en memoria con alineación
void mem_load_data(double *memory, size_t segment, size_t base_offset, double *array, size_t size, size_t alignment);


#endif // MEMORY_H
