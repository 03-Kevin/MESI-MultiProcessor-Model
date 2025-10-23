#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h> // Para size_t

// ================= Alineación =================
#define ALIGNMENT 64  // Alineación en posiciones de double

// ================= Sistema =================
#define NUM_PES 4

// ================= Memoria =================
#define MEM_SIZE 512
#define VECTOR_SIZE 251

// ================= Segmentos =================
typedef enum {
    VECTOR_A,
    VECTOR_B,
    SUMS,
    DONE,
    RESULT,
    NUM_SEGMENTS
} Segment;

// Direcciones base en memoria (offset dentro de cada segmento)
#define VECTOR_A_ADDR 0
#define VECTOR_B_ADDR (VECTOR_A_ADDR + VECTOR_SIZE)
#define SUMS_ADDR     (VECTOR_B_ADDR + VECTOR_SIZE)
#define DONE_ADDR     (SUMS_ADDR + NUM_PES)
#define RESULT_ADDR   (DONE_ADDR + 1)

// ================= Caché =================
#define WAYS 2
#define TOTAL_BLOCKS 16
#define SETS (TOTAL_BLOCKS / WAYS)
#define LINE_SIZE_BYTES 32
#define DOUBLE_SIZE sizeof(double)
#define DOUBLES_PER_LINE (LINE_SIZE_BYTES / DOUBLE_SIZE)
#define BLOCK_SIZE DOUBLES_PER_LINE

// ================= Bus =================
#define NUM_BUS_MSGS 4

#endif // CONFIG_H
