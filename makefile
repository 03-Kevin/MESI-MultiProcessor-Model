# ============================
# 🔧 CONFIGURACIÓN GENERAL
# ============================
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = mp_mesi

# 🧩 Directorios de código
SRC_DIR = src
OBJ_DIR = obj

# 🧠 Incluir subcarpetas
INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/bus \
           -I$(SRC_DIR)/cache \
           -I$(SRC_DIR)/memory \
           -I$(SRC_DIR)/pe \
           -I$(SRC_DIR)/mesi

# 🔍 Buscar todos los archivos .c en src/ y subcarpetas
SRC = $(shell find $(SRC_DIR) -name "*.c")
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# ============================
# 🎨 COLORES
# ============================
GREEN  = \033[0;32m
YELLOW = \033[1;33m
RED    = \033[0;31m
RESET  = \033[0m

# ============================
# 🧱 REGLAS PRINCIPALES
# ============================
all: $(TARGET)

# Crear ejecutable
$(TARGET): $(OBJ)
	@echo "$(YELLOW)🔧 Enlazando...$(RESET)"
	@$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "$(GREEN)✅ Compilación completa: $(TARGET)$(RESET)"

# Compilar cada archivo .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)📦 Compilando $< ...$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================
# ▶️ REGLAS DE EJECUCIÓN
# ============================
run: $(TARGET)
	@echo "$(GREEN)🚀 Ejecutando...$(RESET)"
	@./$(TARGET)

debug: $(TARGET)
	@echo "$(GREEN)🐞 Iniciando gdb...$(RESET)"
	@gdb ./$(TARGET)

# ============================
# 🧹 LIMPIEZA
# ============================
clean:
	@echo "$(RED)🧹 Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJ_DIR) $(TARGET)

# ============================
# 📦 EXTRA
# ============================
.PHONY: all clean run debug
