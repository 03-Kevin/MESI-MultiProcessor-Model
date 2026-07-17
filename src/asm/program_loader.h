#ifndef PROGRAM_LOADER_H
#define PROGRAM_LOADER_H

#include <stddef.h>

#define MAX_INSTRUCTIONS 1024
#define MAX_ARGS 3
#define MAX_OPCODE_LEN 16
#define MAX_LABELS 256
#define MAX_LABEL_LEN 64
#define MAX_LINE_LENGTH 128

typedef struct {
    char opcode[MAX_OPCODE_LEN];   // e.g. "LOAD", "FMUL"
    int args[MAX_ARGS];            // argumentos resueltos (reg indices o PC targets)
    int argc;                      // número de argumentos efectivamente parseados
    char raw_args[MAX_LINE_LENGTH]; // copia cruda (opcional, para debug)
} Instruction;

typedef struct {
    Instruction instructions[MAX_INSTRUCTIONS];
    int num_instructions;
} Program;

/**
 * Carga un programa ASM desde filename en program.
 * - Soporta etiquetas: "LOOP:" en su propia línea o con instrucción.
 * - Soporta comentarios que empiezan con '#' o '//' (resto de la línea ignorado).
 * - JNZ usa: JNZ reg label   (label será resuelto a PC por el loader)
 *
 * Devuelve número de instrucciones cargadas (>=0) o -1 en caso de error.
 */
int load_program(const char *filename, Program *program);

#endif // PROGRAM_LOADER_H
