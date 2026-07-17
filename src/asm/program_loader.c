// src/asm/program_loader.c
#include "program_loader.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 128
#endif

#ifndef MAX_LABELS
#define MAX_LABELS 64
#endif

#ifndef MAX_LABEL_LEN
#define MAX_LABEL_LEN 64
#endif

// Estructura interna para tablas de labels
typedef struct {
    char name[MAX_LABEL_LEN];
    int target_pc;
} LabelEntry;

// Helper: trim leading/trailing whitespace in place
static char *strip(char *s) {
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

int load_program(const char *filename, Program *program) {
    if (!filename || !program) {
        fprintf(stderr, "[program_loader] Parámetros inválidos.\n");
        return -1;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("[program_loader] Error abriendo archivo");
        return -1;
    }

    // Buffer de líneas (leer todo el archivo para facilitar la segunda pasada)
    char lines[MAX_INSTRUCTIONS][MAX_LINE_LENGTH];
    int line_count = 0;

    while (fgets(lines[line_count], MAX_LINE_LENGTH, f)) {
        // Remove trailing newline
        char *p = lines[line_count];
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';

        // Trim
        char *t = strip(p);

        // Skip blank lines but keep position (helps with labels)
        if (t[0] == '\0') {
            lines[line_count][0] = '\0';
        } else {
            // Support comments starting with '#' or '//' -> truncate
            char *hash = strchr(t, '#');
            char *slash = strstr(t, "//");
            char *comment_pos = NULL;
            if (hash && slash) comment_pos = (hash < slash) ? hash : slash;
            else if (hash) comment_pos = hash;
            else if (slash) comment_pos = slash;

            if (comment_pos) {
                *comment_pos = '\0';
                char *trimmed = strip(t);
                memmove(lines[line_count], trimmed, MAX_LINE_LENGTH - 1);
                lines[line_count][MAX_LINE_LENGTH - 1] = '\0';
            } else {
                // copy trimmed
                char *trimmed = strip(t);
                memmove(lines[line_count], trimmed, MAX_LINE_LENGTH - 1);
                lines[line_count][MAX_LINE_LENGTH - 1] = '\0';
            }
        }

        line_count++;
        if (line_count >= MAX_INSTRUCTIONS) break;
    }

    fclose(f);

    // First pass: collect labels and their target PC
    LabelEntry labels[MAX_LABELS];
    int label_count = 0;
    int pc = 0; // PC numbering for instructions (not file line number)

    for (int i = 0; i < line_count; ++i) {
        char *line = lines[i];
        if (!line || line[0] == '\0') continue;

        char tmp[MAX_LINE_LENGTH];
        strncpy(tmp, line, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';

        char *token = strtok(tmp, " \t");
        if (!token) continue;

        size_t len = strlen(token);
        if (len > 0 && token[len - 1] == ':') {
            // label found
            if (label_count >= MAX_LABELS) {
                fprintf(stderr, "[program_loader] demasiadas etiquetas (MAX_LABELS=%d)\n", MAX_LABELS);
                return -1;
            }
            char name[MAX_LABEL_LEN];
            size_t nlen = (len - 1) < (MAX_LABEL_LEN - 1) ? (len - 1) : (MAX_LABEL_LEN - 1);
            strncpy(name, token, nlen);
            name[nlen] = '\0';
            strncpy(labels[label_count].name, name, MAX_LABEL_LEN - 1);
            labels[label_count].name[MAX_LABEL_LEN - 1] = '\0';
            labels[label_count].target_pc = pc;
            label_count++;

            // Check if there's an instruction after the label on same line
            char *rest = line + (token - tmp) + len;
            rest = strip(rest);
            if (rest && rest[0] != '\0') {
                pc++; // this line also contains an instruction
            }
        } else {
            pc++;
        }
    }

    // Second pass: parse instructions and resolve labels to instruction PCs
    program->num_instructions = 0;

    for (int i = 0; i < line_count; ++i) {
        char *line = lines[i];
        if (!line || line[0] == '\0') continue;

        char copy[MAX_LINE_LENGTH];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';
        char *cur = strip(copy);
        if (!cur || cur[0] == '\0') continue;

        // Handle label at start of line (e.g., "LOOP: LOAD 5 0")
        char *tok = strtok(cur, " \t");
        if (!tok) continue;

        if (tok[strlen(tok) - 1] == ':') {
            // move to rest of the line after label
            char *rest = cur + (tok - copy) + strlen(tok);
            rest = strip(rest);
            if (!rest || rest[0] == '\0') continue;
            cur = rest;
            tok = strtok(cur, " \t");
            if (!tok) continue;
        }

        // Build Instruction
        Instruction instr;
        // zero entire instruction (opcode and args)
        memset(&instr, 0, sizeof(Instruction));
        // safe copy of opcode
        strncpy(instr.opcode, tok, sizeof(instr.opcode) - 1);
        instr.opcode[sizeof(instr.opcode) - 1] = '\0';

        // Initialize args to zero (important to avoid garbage when fewer args given)
        for (int a = 0; a < MAX_ARGS; ++a) instr.args[a] = 0;

        // Grab the rest of the line as arguments string
        char *args_start = strtok(NULL, "");
        if (args_start) {
            // replace commas with spaces to allow "LOAD 5,0" or "LOAD 5 0"
            char tmpargs[MAX_LINE_LENGTH];
            strncpy(tmpargs, args_start, sizeof(tmpargs) - 1);
            tmpargs[sizeof(tmpargs) - 1] = '\0';
            for (char *p = tmpargs; *p; ++p) if (*p == ',') *p = ' ';

            // Tokenize arguments
            char *argtok = strtok(tmpargs, " \t");
            int arg_index = 0;
            while (argtok && arg_index < MAX_ARGS) {
                // Check if token is a number (optional leading + or -)
                int isnum = 1;
                char *q = argtok;
                if (*q == '+' || *q == '-') q++;
                if (!*q) isnum = 0;
                while (*q) {
                    if (!isdigit((unsigned char)*q)) { isnum = 0; break; }
                    q++;
                }

                if (isnum) {
                    instr.args[arg_index++] = atoi(argtok);
                } else {
                    // treat as label/symbol - resolve in labels table
                    int found = -1;
                    for (int L = 0; L < label_count; ++L) {
                        if (strcmp(labels[L].name, argtok) == 0) {
                            found = labels[L].target_pc;
                            break;
                        }
                    }
                    if (found == -1) {
                        fprintf(stderr, "[program_loader] Label no encontrada: '%s' en %s\n", argtok, filename);
                        return -1;
                    }
                    instr.args[arg_index++] = found;
                }

                argtok = strtok(NULL, " \t");
            }
        }

        // Store instruction in program
        if (program->num_instructions >= MAX_INSTRUCTIONS) {
            fprintf(stderr, "[program_loader] demasiadas instrucciones (MAX_INSTRUCTIONS=%d)\n", MAX_INSTRUCTIONS);
            return -1;
        }
        program->instructions[program->num_instructions++] = instr;
    }

    return program->num_instructions;
}