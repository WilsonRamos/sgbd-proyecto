# Makefile para SGBD Físico Simple
# Compilación alternativa sin CMake

# Configuración del compilador
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -I./include
LIBS = -lstdc++fs

# Directorios
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

# Archivos
TARGET = sgbd_fisico
MAIN_SRC = $(SRC_DIR)/main.cpp
TEST_TARGET = test_runner
TEST_SRC = $(TEST_DIR)/test_basic.cpp

# Headers (para dependencias)
HEADERS = $(INCLUDE_DIR)/PhysicalAddress.h \
          $(INCLUDE_DIR)/DiskConfig.h \
          $(INCLUDE_DIR)/Record.h \
          $(INCLUDE_DIR)/Block.h \
          $(INCLUDE_DIR)/FileSystemSimulator.h \
          $(INCLUDE_DIR)/DiskManager.h

# Detectar sistema operativo
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS += -pthread
endif
ifeq ($(UNAME_S),Darwin)
    LIBS += -pthread
endif

# Targets principales
.PHONY: all clean test install help run setup

all: setup $(TARGET)

# Compilar programa principal
$(TARGET): $(MAIN_SRC) $(HEADERS)
	@echo "Compilando SGBD Físico..."
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(MAIN_SRC) -o $(BIN_DIR)/$(TARGET) $(LIBS)
	@echo "Compilación exitosa: $(BIN_DIR)/$(TARGET)"

# Compilar tests
test: $(TEST_TARGET)
	@echo "Ejecutando tests..."
	@./$(BIN_DIR)/$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(HEADERS)
	@echo "Compilando tests..."
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRC) -o $(BIN_DIR)/$(TEST_TARGET) $(LIBS)

# Ejecutar el programa
run: $(TARGET)
	@echo "Ejecutando SGBD Físico..."
	@cd $(BIN_DIR) && ./$(TARGET)

# Configurar directorios y datos de prueba
setup:
	@echo "Configurando proyecto..."
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(TEST_DIR)/sample_data
	@echo "Creando datos de prueba..."
	@echo "Juan Perez,30,Ingeniero,75000" > $(TEST_DIR)/sample_data/empleados.csv
	@echo "Maria Garcia,28,Analista,65000" >> $(TEST_DIR)/sample_data/empleados.csv
	@echo "Carlos Rodriguez,35,Gerente,85000" >> $(TEST_DIR)/sample_data/empleados.csv
	@echo "Ana Martinez,32,Desarrolladora,70000" >> $(TEST_DIR)/sample_data/empleados.csv
	@echo "Luis Gonzalez,29,Tester,60000" >> $(TEST_DIR)/sample_data/empleados.csv
	@echo "Laptop HP,1200.50,Computadoras,20" > $(TEST_DIR)/sample_data/productos.csv
	@echo "Mouse Logitech,25.99,Accesorios,100" >> $(TEST_DIR)/sample_data/productos.csv
	@echo "Monitor Dell,300.00,Pantallas,15" >> $(TEST_DIR)/sample_data/productos.csv
	@echo "Teclado Mecánico,89.99,Accesorios,50" >> $(TEST_DIR)/sample_data/productos.csv
	@echo "Impresora Canon,150.00,Oficina,8" >> $(TEST_DIR)/sample_data/productos.csv

# Instalar en el sistema
install: $(TARGET)
	@echo "Instalando en /usr/local/bin..."
	@sudo cp $(BIN_DIR)/$(TARGET) /usr/local/bin/
	@echo "Instalación completa. Ejecuta: sgbd_fisico"

# Limpiar archivos generados
clean:
	@echo "Limpiando archivos generados..."
	@rm -rf $(BIN_DIR)
	@rm -rf $(BUILD_DIR)
	@rm -rf ./mi_disco_sgbd
	@rm -rf ./disk_simulation
	@rm -f empleados.csv productos.csv
	@echo "Limpieza completa."

# Limpiar solo ejecutables
clean-bin:
	@echo "Limpiando ejecutables..."
	@rm -rf $(BIN_DIR)

# Demo completo
demo: setup $(TARGET)
	@echo "=== DEMO DEL SGBD FÍSICO ==="
	@echo "1. Creando datos de prueba..."
	@cp $(TEST_DIR)/sample_data/*.csv .
	@echo "2. Iniciando programa (usa los archivos CSV creados)"
	@echo "3. Archivos disponibles: empleados.csv, productos.csv"
	@echo "4. Ejecutando..."
	@cd $(BIN_DIR) && ./$(TARGET)

# Verificar dependencias
check-deps:
	@echo "Verificando dependencias..."
	@which $(CXX) > /dev/null || (echo "Error: g++ no encontrado" && exit 1)
	@echo "Compilador: $(shell $(CXX) --version | head -n1)"
	@echo "Estándar C++17: $(shell $(CXX) -dumpversion)"
	@echo "Sistema: $(UNAME_S)"
	@echo "Dependencias OK"

# Información del proyecto
info:
	@echo "=== INFORMACIÓN DEL PROYECTO ==="
	@echo "Nombre: SGBD Físico Simple"
	@echo "Versión: 1.0"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Sistema: $(UNAME_S)"
	@echo "Directorio de construcción: $(BIN_DIR)"
	@echo "=== ARCHIVOS ==="
	@echo "Ejecutable: $(BIN_DIR)/$(TARGET)"
	@echo "Headers: $(words $(HEADERS)) archivos"
	@echo "=== USO ==="
	@echo "make        - Compilar proyecto"
	@echo "make run    - Compilar y ejecutar"
	@echo "make test   - Ejecutar tests"
	@echo "make demo   - Demo completo"
	@echo "make clean  - Limpiar todo"

# Ayuda
help:
	@echo "=== SGBD FÍSICO - SISTEMA DE COMPILACIÓN ==="
	@echo ""
	@echo "Targets disponibles:"
	@echo "  all          - Compilar todo (default)"
	@echo "  run          - Compilar y ejecutar programa"
	@echo "  test         - Compilar y ejecutar tests"
	@echo "  demo         - Demo completo con datos de prueba"
	@echo "  setup        - Configurar directorios y datos"
	@echo "  install      - Instalar en el sistema"
	@echo "  clean        - Limpiar archivos generados"
	@echo "  clean-bin    - Limpiar solo ejecutables"
	@echo "  check-deps   - Verificar dependencias"
	@echo "  info         - Información del proyecto"
	@echo "  help         - Mostrar esta ayuda"
	@echo ""
	@echo "Ejemplos de uso:"
	@echo "  make run              # Compilar y ejecutar"
	@echo "  make demo             # Demo completo"
	@echo "  make clean && make    # Recompilar desde cero"
	@echo ""
	@echo "Requisitos:"
	@echo "  - g++ con soporte C++17"
	@echo "  - Sistema de archivos (std::filesystem)"
	@echo "  - Linux/macOS/Windows (con MinGW)"

# Debug build
debug: CXXFLAGS += -g -DDEBUG -O0
debug: $(TARGET)
	@echo "Compilación debug completada"

# Release optimizada
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)
	@echo "Compilación release completada"

# Análisis estático
static-analysis:
	@echo "Ejecutando análisis estático..."
	@which cppcheck > /dev/null && cppcheck --enable=all $(INCLUDE_DIR) $(SRC_DIR) || echo "cppcheck no disponible"

# Formatear código
format:
	@echo "Formateando código..."
	@which clang-format > /dev/null && find $(INCLUDE_DIR) $(SRC_DIR) -name "*.h" -o -name "*.cpp" | xargs clang-format -i || echo "clang-format no disponible"

# Target por defecto
.DEFAULT_GOAL := all