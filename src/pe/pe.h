#ifndef PE_H
#define PE_H

#include "cache/cache.h"
#include <pthread.h>
#include "include/config.h"      // Necesario para las constantes globales como NUM_PES


typedef struct {
    int id;
    double regs[8];
    Cache* cache;
} PE;

void* pe_run(void* arg);

// Declaración de las instrucciones del ISA
void LOAD(PE* pe, int dest, int addr);       // Cargar desde memoria a un registro
void STORE(PE* pe, int src, int addr);      // Almacenar desde un registro a memoria
void FMUL(PE* pe, int dest, int src1, int src2); // Multiplicación de punto flotante
void FADD(PE* pe, int dest, int src1, int src2); // Suma de punto flotante
void INC(PE* pe, int reg);                  // Incrementar un registro
void DEC(PE* pe, int reg);                  // Decrementar un registro
int JNZ(PE* pe, int reg);                   // Salto condicional si el registro no es 0



#endif
