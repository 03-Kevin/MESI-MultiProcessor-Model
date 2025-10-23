// src/pe/pe.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe.h"
#include "../cache/cache.h"    // cache_read / cache_write
#include "../asm/program_loader.h"
#include "../include/config.h"  // VECTOR_SIZE, VECTOR_A_ADDR, VECTOR_B_ADDR, SUMS_ADDR, NUM_PES
#include <errno.h>

#ifndef MAX_REGS
#define MAX_REGS 8
#endif

// Forward: primitivas ISA (las implementamos aquí para claridad)
void LOAD(PE *pe, int dest, int addr);
void STORE(PE *pe, int src, int addr);
void FMUL(PE *pe, int dest, int src1, int src2);
void FADD(PE *pe, int dest, int src1, int src2);
void INC(PE *pe, int reg);
void DEC(PE *pe, int reg);
int JNZ(PE *pe, int reg);

// Helper de validación
static int valid_reg(int r) {
    return (r >= 0 && r < MAX_REGS);
}

// Validación de dirección (por si quieres doble chequeo)
static void validate_addr(int addr) {
    if (addr < 0 || addr >= MEM_SIZE) {
        fprintf(stderr, "[PE] ERROR: acceso fuera de rango addr=%d (MEM_SIZE=%d). Abortando.\n", addr, MEM_SIZE);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
}

// --------------------------------------------------
// Función principal del hilo PE
// --------------------------------------------------
void *pe_run(void *arg) {
    PE *pe = (PE *)arg;
    if (!pe) return NULL;

    int N = VECTOR_SIZE;
    int pe_id = pe->id;

    // Ajustar la segmentación para manejar casos donde VECTOR_SIZE no sea divisible por NUM_PES
    int base_chunk = N / NUM_PES;  // Tamaño base del segmento
    int remainder = N % NUM_PES;  // Elementos sobrantes

    // Asignar los elementos sobrantes a los primeros PEs
    int start = pe_id * base_chunk + (pe_id < remainder ? pe_id : remainder);
    int end = start + base_chunk + (pe_id < remainder ? 1 : 0);

    // Validar que los límites de segmentación estén dentro de los rangos permitidos
    if (start < 0 || start >= VECTOR_SIZE || end < 0 || end > VECTOR_SIZE) {
        fprintf(stderr, "[PE%d] Error: Segmentación fuera de rango start=%d end=%d VECTOR_SIZE=%d\n", pe_id, start, end, VECTOR_SIZE);
        return NULL;
    }

    printf("[PE%d] start=%d end=%d chunk=%d\n", pe_id, start, end, end - start);

    // Inicializar registros (8 regs, doubles)
    for (int i = 0; i < MAX_REGS; ++i) pe->regs[i] = 0.0;
    pe->regs[0] = (double)(VECTOR_A_ADDR + start); // puntero a A[i]
    pe->regs[1] = (double)(VECTOR_B_ADDR + start); // puntero a B[i]
    pe->regs[2] = (double)(SUMS_ADDR + pe_id);     // puntero a SUMS[pe_id]
    pe->regs[3] = (double)(end - start);           // contador
    pe->regs[4] = 0.0;                             // acumulador parcial

    // Cargar valor inicial de SUMS[pe_id] (si contienen basura)
    int sums_addr = (int)pe->regs[2];
    validate_addr(sums_addr);
    LOAD(pe, 4, sums_addr); // pone el valor en REG4

    // Cargar programa ASM (archivo editable)
    Program program;
    int ret = load_program("src/asm/program.asm", &program);
    if (ret < 0) {
        fprintf(stderr, "[PE%d] Error cargando programa ASM (archivo src/asm/program.asm)\n", pe_id);
        return NULL;
    }

    // Ejecutar programa
    int pc = 0;
    while (pc < program.num_instructions) {
        Instruction *instr = &program.instructions[pc];
        // Imprime traza corta: PC y algunos registros
        printf("[PE%d] PC=%d OPCODE=%s ARGS=%d,%d,%d | REG0=%f REG1=%f REG3=%f REG4=%f\n",
               pe_id, pc, instr->opcode,
               instr->args[0], instr->args[1], instr->args[2],
               pe->regs[0], pe->regs[1], pe->regs[3], pe->regs[4]);

        if (strcmp(instr->opcode, "LOAD") == 0) {
            int dest = instr->args[0];
            int src_reg = instr->args[1];
            if (!valid_reg(dest) || !valid_reg(src_reg)) {
                fprintf(stderr, "[PE%d] LOAD: argumento de registro inválido dest=%d src=%d\n", pe_id, dest, src_reg);
                return NULL;
            }
            int addr = (int)pe->regs[src_reg];
            validate_addr(addr);
            LOAD(pe, dest, addr);
        }
        else if (strcmp(instr->opcode, "STORE") == 0) {
            int src = instr->args[0];
            int addr_reg = instr->args[1];
            if (!valid_reg(src) || !valid_reg(addr_reg)) {
                fprintf(stderr, "[PE%d] STORE: argumento de registro inválido src=%d addr_reg=%d\n", pe_id, src, addr_reg);
                return NULL;
            }
            int addr = (int)pe->regs[addr_reg];
            validate_addr(addr);
            STORE(pe, src, addr);
        }
        else if (strcmp(instr->opcode, "FMUL") == 0) {
            int dest = instr->args[0], s1 = instr->args[1], s2 = instr->args[2];
            if (!valid_reg(dest) || !valid_reg(s1) || !valid_reg(s2)) {
                fprintf(stderr, "[PE%d] FMUL: registro inválido\n", pe_id);
                return NULL;
            }
            FMUL(pe, dest, s1, s2);
        }
        else if (strcmp(instr->opcode, "FADD") == 0) {
            int dest = instr->args[0], s1 = instr->args[1], s2 = instr->args[2];
            if (!valid_reg(dest) || !valid_reg(s1) || !valid_reg(s2)) {
                fprintf(stderr, "[PE%d] FADD: registro inválido\n", pe_id);
                return NULL;
            }
            FADD(pe, dest, s1, s2);
        }
        else if (strcmp(instr->opcode, "INC") == 0) {
            int r = instr->args[0];
            if (!valid_reg(r)) {
                fprintf(stderr, "[PE%d] INC: reg inválido\n", pe_id);
                return NULL;
            }
            INC(pe, r);
        }
        else if (strcmp(instr->opcode, "DEC") == 0) {
            int r = instr->args[0];
            if (!valid_reg(r)) {
                fprintf(stderr, "[PE%d] DEC: reg inválido\n", pe_id);
                return NULL;
            }
            DEC(pe, r);
        }
        else if (strcmp(instr->opcode, "JNZ") == 0) {
            int r = instr->args[0];
            int target = instr->args[1]; // **IMPORTANTE**: en tu archivo asm JNZ debe tener dos args: reg target_pc
            if (!valid_reg(r)) {
                fprintf(stderr, "[PE%d] JNZ: reg inválido\n", pe_id);
                return NULL;
            }
            if (target < 0 || target >= program.num_instructions) {
                fprintf(stderr, "[PE%d] JNZ: target fuera de rango target=%d\n", pe_id, target);
                return NULL;
            }
            if (JNZ(pe, r)) {
                // salto
                pc = target;
                continue;
            }
        }
        else {
            fprintf(stderr, "[PE%d] Instrucción desconocida: %s (PC=%d)\n", pe_id, instr->opcode, pc);
            return NULL;
        }

        pc++;
    }

    printf("[PE%d] Fin de ejecución\n", pe_id);
    return NULL;
}

// ----------------------
// Implementación ISA
// ----------------------
void LOAD(PE *pe, int dest, int addr) {
    validate_addr(addr);  // Validar dirección antes de leer
    double value = cache_read(pe->cache, addr, pe->id);
    pe->regs[dest] = value;
    printf("[PE%d] LOAD(REG%d) <- CACHE[Addr=%d] = %f\n", pe->id, dest, addr, value);
}

void STORE(PE *pe, int src, int addr) {
    validate_addr(addr);  // Validar dirección antes de escribir
    double value = pe->regs[src];
    cache_write(pe->cache, addr, value, pe->id);
    printf("[PE%d] STORE CACHE[Addr=%d] <- REG%d = %f\n", pe->id, addr, src, value);
}

void FMUL(PE *pe, int dest, int src1, int src2)
{
    double result = pe->regs[src1] * pe->regs[src2];
    pe->regs[dest] = result;
    printf("[PE%d] FMUL REG%d <- REG%d * REG%d = %f\n", pe->id, dest, src1, src2, result);
}

void FADD(PE *pe, int dest, int src1, int src2)
{
    double result = pe->regs[src1] + pe->regs[src2];
    pe->regs[dest] = result;
    printf("[PE%d] FADD REG%d <- REG%d + REG%d = %f\n", pe->id, dest, src1, src2, result);
}

void INC(PE *pe, int reg)
{
    pe->regs[reg] += 1.0;
    printf("[PE%d] INC REG%d = %f\n", pe->id, reg, pe->regs[reg]);
}

void DEC(PE *pe, int reg)
{
    pe->regs[reg] -= 1.0;
    printf("[PE%d] DEC REG%d = %f\n", pe->id, reg, pe->regs[reg]);
}

int JNZ(PE *pe, int reg) {
    if (!valid_reg(reg)) {
        fprintf(stderr, "[PE%d] JNZ: registro inválido REG=%d\n", pe->id, reg);
        exit(EXIT_FAILURE);
    }
    if (pe->regs[reg] != 0.0) {
        printf("[PE%d] JNZ: REG%d != 0, saltando...\n", pe->id, reg);
        return 1;
    } else {
        printf("[PE%d] JNZ: REG%d == 0, no salta.\n", pe->id, reg);
        return 0;
    }
}