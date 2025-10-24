# ============================
# CONFIGURACIÓN GENERAL
# ============================
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = mp_mesi

# Directorios de código
SRC_DIR = src
OBJ_DIR = obj

# Incluir subcarpetas
INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/bus \
           -I$(SRC_DIR)/cache \
           -I$(SRC_DIR)/memory \
           -I$(SRC_DIR)/pe \
           -I$(SRC_DIR)/mesi

# Buscar todos los archivos .c en src/ (excluyendo tests por defecto)
SRC = $(shell find $(SRC_DIR) -name "*.c" -not -path "$(SRC_DIR)/tests/*")
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Archivos de tests (compilados/linked por separado)
TEST_SRC = $(shell find $(SRC_DIR)/tests -name "*.c" 2>/dev/null)
TEST_OBJS = $(TEST_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Lista de objetos sin main.o (útil para linkear tests que tienen su propio main)
NO_MAIN = $(filter-out $(OBJ_DIR)/main.o, $(OBJ))

# ============================
# COLORES
# ============================
GREEN  = \033[0;32m
YELLOW = \033[1;33m
RED    = \033[0;31m
RESET  = \033[0m

# ============================
# REGLAS PRINCIPALES
# ============================
all: $(TARGET)

# Crear ejecutable principal
$(TARGET): $(OBJ)
	@echo "$(YELLOW) Enlazando...$(RESET)"
	@$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "$(GREEN)Compilación completa: $(TARGET)$(RESET)"

# Target para compilar test mesi_stress (si existe)
# genera ejecutable 'mesi_stress' usando todos los objetos normales menos main.o
# y el/los objeto(s) del test.
mesi_stress: $(NO_MAIN) $(OBJ_DIR)/tests/mesi_stress.o
	@echo "$(YELLOW)Enlazando test mesi_stress...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) $(NO_MAIN) $(OBJ_DIR)/tests/mesi_stress.o -o mesi_stress
	@echo "$(GREEN)Compilación completa: mesi_stress$(RESET)"

# Compilar cada archivo .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compilando $< ...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================
# REGLAS DE EJECUCIÓN
# ============================
run: $(TARGET)
	@echo "$(GREEN) Ejecutando...$(RESET)"
	@./$(TARGET)

debug: $(TARGET)
	@echo "$(GREEN) Iniciando gdb...$(RESET)"
	@gdb ./$(TARGET)

stress: mesi_stress
	@echo "$(GREEN)Ejecutando stress test...$(RESET)"
	@./mesi_stress

# ============================
# LIMPIEZA
# ============================
clean:
	@echo "$(RED) Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJ_DIR) $(TARGET) mesi_stress

# ============================
# EXTRA
# ============================
.PHONY: all clean run debug mesi_stress stress
