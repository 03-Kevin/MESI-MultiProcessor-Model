// src/memory/memory.c
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include "include/config.h"
#include <stdlib.h>

// Definición de la memoria principal y el mutex
double main_memory[MEM_SIZE];
pthread_mutex_t mem_lock;

// Tamaños de cada segmento (opcional, para control)
size_t segment_sizes[NUM_SEGMENTS] = {
    VECTOR_SIZE,   // VECTOR_A
    VECTOR_SIZE,   // VECTOR_B
    NUM_PES,       // SUMS
    NUM_PES,       // DONE
    1              // RESULT
};

// Dirección base de cada segmento
size_t segment_bases[NUM_SEGMENTS] = {
    VECTOR_A_ADDR,
    VECTOR_B_ADDR,
    SUMS_ADDR,
    DONE_ADDR,
    RESULT_ADDR
};

// Inicializa la memoria principal y el mutex
void mem_init() {
    pthread_mutex_init(&mem_lock, NULL);
    for (int i = 0; i < MEM_SIZE; i++) {
        main_memory[i] = 0.0;
    }
}

// Calcula la dirección física a partir del segmento y offset
static inline size_t get_phys_addr(Segment seg, int offset) {
    if (seg < 0 || seg >= NUM_SEGMENTS) {
        fprintf(stderr, "Error: segmento inválido en get_phys_addr (%d)\n", seg);
        exit(EXIT_FAILURE);
    }
    if (offset < 0 || (size_t)offset >= segment_sizes[seg]) {
        fprintf(stderr, "Error: Offset fuera de rango en el segmento %d (offset=%d seg_size=%zu)\n", seg, offset, segment_sizes[seg]);
        exit(EXIT_FAILURE);
    }
    size_t addr = segment_bases[seg] + (size_t)offset;
    if (addr >= MEM_SIZE) {
        fprintf(stderr, "Error: Dirección física fuera de los límites de la memoria (addr=%zu MEM_SIZE=%d)\n", addr, MEM_SIZE);
        exit(EXIT_FAILURE);
    }
    return addr;
}

// Convierte una dirección global (phys addr) en un segmento de memoria
Segment addr_to_segment(int addr) {
    if (addr < 0 || addr >= MEM_SIZE) {
        fprintf(stderr, "addr_to_segment: dirección fuera de rango addr=%d (MEM_SIZE=%d)\n", addr, MEM_SIZE);
        exit(EXIT_FAILURE);
    }

    if ((size_t)addr >= segment_bases[RESULT] && (size_t)addr < segment_bases[RESULT] + segment_sizes[RESULT])
        return RESULT;
    if ((size_t)addr >= segment_bases[DONE] && (size_t)addr < segment_bases[DONE] + segment_sizes[DONE])
        return DONE;
    if ((size_t)addr >= segment_bases[SUMS] && (size_t)addr < segment_bases[SUMS] + segment_sizes[SUMS])
        return SUMS;
    if ((size_t)addr >= segment_bases[VECTOR_B] && (size_t)addr < segment_bases[VECTOR_B] + segment_sizes[VECTOR_B])
        return VECTOR_B;
    if ((size_t)addr >= segment_bases[VECTOR_A] && (size_t)addr < segment_bases[VECTOR_A] + segment_sizes[VECTOR_A])
        return VECTOR_A;

    // Fallback seguro (no debería pasar)
    fprintf(stderr, "addr_to_segment: no se pudo mapear addr=%d a ningún segmento\n", addr);
    exit(EXIT_FAILURE);
}

// Lee un valor de la memoria principal
double mem_read(Segment seg, int offset) {
    size_t addr = get_phys_addr(seg, offset);
    pthread_mutex_lock(&mem_lock);
    double val = main_memory[addr];
    pthread_mutex_unlock(&mem_lock);
    printf("[DEBUG] Leyendo de memoria: Segmento=%d, Offset=%d, Addr=%zu, Valor=%f\n", seg, offset, addr, val);
    return val;
}

// Escribe un valor en la memoria principal
void mem_write(Segment seg, int offset, double value) {
    size_t addr = get_phys_addr(seg, offset);
    pthread_mutex_lock(&mem_lock);
    main_memory[addr] = value;
    pthread_mutex_unlock(&mem_lock);
    printf("[DEBUG] Escribiendo en memoria: Segmento=%d, Offset=%d, Addr=%zu, Valor=%f\n", seg, offset, addr, value);
}

// Carga datos en memoria con alineación (segment = enum Segment)
void mem_load_data(double *memory, size_t segment, size_t base_offset, double *array, size_t size, size_t alignment) {
    if (segment >= NUM_SEGMENTS) {
        fprintf(stderr, "mem_load_data: segmento inválido %zu\n", segment);
        exit(EXIT_FAILURE);
    }

    // calcular offset alineado *dentro del segmento*
    size_t aligned_base = (base_offset + alignment - 1) & ~(alignment - 1);

    // asegurar que la copia cabe en el segmento
    if (aligned_base + size > segment_sizes[segment]) {
        fprintf(stderr, "Error: Carga de datos fuera del segmento %zu (aligned_base=%zu size=%zu seg_size=%zu).\n",
                segment, aligned_base, size, segment_sizes[segment]);
        exit(EXIT_FAILURE);
    }

    // dirección física de inicio
    size_t phys_base = segment_bases[segment] + aligned_base;
    if (phys_base + size > MEM_SIZE) {
        fprintf(stderr, "Error: Carga de datos fuera de los límites físicos (phys_base=%zu size=%zu MEM_SIZE=%d).\n",
                phys_base, size, MEM_SIZE);
        exit(EXIT_FAILURE);
    }

    memcpy(&memory[phys_base], array, size * sizeof(double));
    printf("[MEMORY] Datos cargados en segmento %zu, offset alineado %zu (phys %zu), tamaño %zu, alineación %zu\n",
           segment, aligned_base, phys_base, size, alignment);
}
