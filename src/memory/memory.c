// src/memory/memory.c
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/config.h"

// Definición de la memoria principal y el mutex
double main_memory[MEM_SIZE];
pthread_mutex_t mem_lock;

// Tamaños de cada segmento (nº de doubles)
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

/* helper local: devuelve capacidad (nº de doubles) del segmento */
static int segment_capacity_in_doubles(Segment seg) {
    switch (seg) {
        case VECTOR_A: return (int)(segment_bases[VECTOR_B] - segment_bases[VECTOR_A]);
        case VECTOR_B: return (int)(segment_bases[SUMS] - segment_bases[VECTOR_B]);
        case SUMS:     return (int)(segment_bases[DONE] - segment_bases[SUMS]);
        case DONE:     return (int)(segment_bases[RESULT] - segment_bases[DONE]);
        case RESULT:   return (int)(MEM_SIZE - segment_bases[RESULT]);
        default:       return 0;
    }
}

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
    int seg_capacity = segment_capacity_in_doubles(seg);
    if (offset < 0 || offset >= seg_capacity) {
        fprintf(stderr, "Error: Offset fuera de rango en el segmento %d (offset=%d seg_size=%d)\n",
                seg, offset, seg_capacity);
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

    fprintf(stderr, "addr_to_segment: no se pudo mapear addr=%d a ningún segmento\n", addr);
    exit(EXIT_FAILURE);
}

// Lee un valor de la memoria principal
double mem_read(Segment seg, int offset) {
    size_t addr = get_phys_addr(seg, offset);
    pthread_mutex_lock(&mem_lock);
    double val = main_memory[addr];
    pthread_mutex_unlock(&mem_lock);
    printf("[DEBUG] Leyendo en memoria: Segmento=%d, Offset=%d, Addr=%zu, Valor=%f\n",
           seg, offset, addr, val);
    return val;
}

// Escribe un valor en la memoria principal
void mem_write(Segment seg, int offset, double value) {
    size_t addr = get_phys_addr(seg, offset);
    pthread_mutex_lock(&mem_lock);
    main_memory[addr] = value;
    pthread_mutex_unlock(&mem_lock);
    printf("[DEBUG] Escribiendo en memoria: Segmento=%d, Offset=%d, Addr=%zu, Valor=%f\n",
           seg, offset, addr, value);
}

/*
 * mem_load_data
 *  - seg: segmento destino (enum Segment)
 *  - offset: offset deseado dentro del segmento (en número de doubles)
 *  - data: puntero a datos fuente (double*)
 *  - count: número de doubles a copiar
 *  - alignment: alineamiento deseado en "nº de doubles" (>=1).
 *
 * Devuelve 0 en éxito, -1 en error (p. ej. overflow, parámetros inválidos).
 */
int mem_load_data(Segment seg, int offset, const double *data, size_t count, int alignment) {
    if (!data) {
        fprintf(stderr, "[mem_load_data] puntero data NULL\n");
        return -1;
    }
    if (count == 0) return 0;
    if (alignment <= 0) alignment = 1;

    int seg_capacity = segment_capacity_in_doubles(seg);
    if (seg_capacity <= 0) {
        fprintf(stderr, "[mem_load_data] segmento inválido o tamaño 0\n");
        return -1;
    }

    if (offset < 0) {
        fprintf(stderr, "[mem_load_data] offset negativo solicitado: %d\n", offset);
        return -1;
    }

    /* calcular offset alineado */
    int aligned_offset = ((offset + alignment - 1) / alignment) * alignment;
    if ((size_t)aligned_offset + count > (size_t)seg_capacity) {
        fprintf(stderr, "[mem_load_data] overflow: aligned_offset=%d + count=%zu > seg_capacity=%d\n",
                aligned_offset, count, seg_capacity);
        return -1;
    }

    /* Copiar atómicamente usando mem_write (ya protegido por mem_lock) */
    for (size_t i = 0; i < count; ++i) {
        mem_write(seg, aligned_offset + (int)i, data[i]);
    }

    printf("[mem_load_data] seg=%d offset_req=%d aligned_offset=%d count=%zu alignment(doubles)=%d\n",
           seg, offset, aligned_offset, count, alignment);

    return 0;
}
