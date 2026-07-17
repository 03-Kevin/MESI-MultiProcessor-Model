# Programa ejemplo: producto punto parcial
# Registros: REG0 = ptr A, REG1 = ptr B, REG2 = ptr SUMS[pe], REG3 = contador, REG4 = acc

LOOP:
LOAD 5 0      # Carga A[i] en REG5
LOAD 6 1      # Carga B[i] en REG6
FMUL 7 5 6    # REG7 = REG5 * REG6
FADD 4 4 7    # REG4 += REG7
INC 0         # Incrementa REG0 (puntero A[i])
INC 1         # Incrementa REG1 (puntero B[i])
DEC 3         # Decrementa REG3 (contador)
JNZ 3 LOOP    # Si REG3 != 0, salta a LOOP
STORE 4 2     # Almacena REG4 en SUMS[pe_id]
