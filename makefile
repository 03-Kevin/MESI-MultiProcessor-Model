# Makefile completo para MESI-MultiProcessor-Model (CORREGIDO)
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS =
TARGET = mp_mesi

SRC_DIR = src
OBJ_DIR = obj

INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/bus \
           -I$(SRC_DIR)/cache \
           -I$(SRC_DIR)/memory \
           -I$(SRC_DIR)/pe \
           -I$(SRC_DIR)/mesi \
           -I$(SRC_DIR)/step_control

# Si pasas STEP_CONTROL=1 en make, definimos la macro que busca el código
ifneq ($(STEP_CONTROL),)
CFLAGS += -DSTEP_CONTROL_AVAILABLE
endif

# Fuentes y objetos
SRC = $(shell find $(SRC_DIR) -name "*.c" -not -path "$(SRC_DIR)/tests/*")
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

TEST_SRC = $(shell find $(SRC_DIR)/tests -name "*.c" 2>/dev/null)
TEST_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

NO_MAIN = $(filter-out $(OBJ_DIR)/main.o, $(OBJ))

GREEN  = \033[0;32m
YELLOW = \033[1;33m
RED    = \033[0;31m
RESET  = \033[0m

.PHONY: all clean run debug mesi_stress stress sources objects

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "$(YELLOW) Enlazando...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "$(GREEN)Compilación completa: $(TARGET)$(RESET)"

mesi_stress: $(NO_MAIN) $(OBJ_DIR)/tests/mesi_stress.o
	@echo "$(YELLOW)Enlazando test mesi_stress...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) $(NO_MAIN) $(OBJ_DIR)/tests/mesi_stress.o -o mesi_stress $(LDFLAGS)
	@echo "$(GREEN)Compilación completa: mesi_stress$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compilando $< ...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	@echo "$(GREEN) Ejecutando $(TARGET)...$(RESET)"
	@./$(TARGET)

debug: $(TARGET)
	@echo "$(GREEN) Iniciando gdb...$(RESET)"
	@gdb ./$(TARGET)

stress: mesi_stress
	@echo "$(GREEN)Ejecutando stress test...$(RESET)"
	@./mesi_stress

clean:
	@echo "$(RED) Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJ_DIR) $(TARGET) mesi_stress

sources:
	@printf "%s\n" $(SRC)

objects:
	@printf "%s\n" $(OBJ)
