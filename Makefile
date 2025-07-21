# Makefile para SGBD con B+ Tree integrado
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -I./include
TARGET = demo_sgbd
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Archivos fuente principales
MAIN_SRC = demo_flujo_completo.cpp

# Headers que necesitan ser incluidos
HEADERS = include/QueryExecutor.h \
          include/RecordReference.h \
          include/BPlusTree/BPlusTree.h \
          include/BPlusTree/BPlusNode.h \
          include/BPlusTree/LeafNode.h \
          include/BPlusTree/KeyComparator.h \
          include/buffer/BufferManagerClock.h \
          include/DiskManagerExtended.h \
          include/Record.h \
          include/Block.h \
          include/PhysicalAddress.h

# Crear directorios si no existen
$(shell mkdir -p $(OBJDIR) $(BINDIR))

.PHONY: all clean run demo test help

# Objetivo principal
all: $(BINDIR)/$(TARGET)

# Compilar el demo principal
$(BINDIR)/$(TARGET): $(MAIN_SRC) $(HEADERS)
	@echo "[*] Compilando demo integrado del SGBD..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MAIN_SRC)
	@echo "[OK] Compilacion exitosa: $@"

# Ejecutar demo
run: $(BINDIR)/$(TARGET)
	@echo "[*] Ejecutando demo del SGBD integrado..."
	@echo "================================================"
	./$(BINDIR)/$(TARGET)

# Demo específico con parámetros
demo: $(BINDIR)/$(TARGET)
	@echo "🎯 Ejecutando demostración completa..."
	@echo "================================================"
	@echo "📋 Configuración:"
	@echo "   - B+ Tree Order: 4"
	@echo "   - Buffer Pool: 16 frames"
	@echo "   - Registros: Empleados (longitud fija)"
	@echo "   - Algoritmo: Clock PIN-AWARE"
	@echo "================================================"
	./$(BINDIR)/$(TARGET)

# Compilar solo para verificar sintaxis
test: $(MAIN_SRC) $(HEADERS)
	@echo "🧪 Verificando sintaxis..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(MAIN_SRC)
	@echo "✅ Sintaxis correcta"

# Compilar con información de debug
debug: CXXFLAGS += -DDEBUG -g3
debug: $(BINDIR)/$(TARGET)
	@echo "🐛 Versión debug compilada"

# Generar diagrama PlantUML (requiere plantuml instalado)
diagram:
	@echo "📊 Generando diagrama del flujo..."
	@if command -v plantuml >/dev/null 2>&1; then \
		plantuml diagrama_flujo_completo.puml; \
		echo "✅ Diagrama generado: diagrama_flujo_completo.png"; \
	else \
		echo "❌ PlantUML no encontrado. Instalar con: apt install plantuml"; \
	fi

# Limpiar archivos compilados
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -rf $(OBJDIR)/* $(BINDIR)/*
	@echo "✅ Limpieza completada"

# Información de ayuda
help:
	@echo "🛠️  MAKEFILE DEL SGBD INTEGRADO"
	@echo "=================================="
	@echo ""
	@echo "Objetivos disponibles:"
	@echo "  make all      - Compilar todo"
	@echo "  make run      - Ejecutar demo"
	@echo "  make demo     - Ejecutar demostración completa"
	@echo "  make test     - Verificar sintaxis"
	@echo "  make debug    - Compilar versión debug"
	@echo "  make diagram  - Generar diagrama PlantUML"
	@echo "  make clean    - Limpiar archivos"
	@echo "  make help     - Mostrar esta ayuda"
	@echo ""
	@echo "Estructura del proyecto:"
	@echo "  📁 include/          - Headers (.h)"
	@echo "  📁 include/BPlusTree/- B+ Tree headers"
	@echo "  📁 include/buffer/   - Buffer Manager headers"
	@echo "  📁 bin/             - Ejecutables"
	@echo "  📁 obj/             - Objetos compilados"
	@echo "  📄 demo_flujo_completo.cpp - Demo principal"
	@echo ""
	@echo "Componentes integrados:"
	@echo "  🌳 B+ Tree          - Indexación"
	@echo "  💾 BufferManager    - Gestión de memoria"
	@echo "  💿 DiskManager      - Almacenamiento"
	@echo "  🔍 QueryExecutor    - Coordinador de consultas"
	@echo ""

# Información del proyecto
info:
	@echo "📋 INFORMACIÓN DEL PROYECTO"
	@echo "============================"
	@echo "Proyecto: SGBD con B+ Tree integrado"
	@echo "Lenguaje: C++17"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo ""
	@echo "Características principales:"
	@echo "  ✅ B+ Tree como método de acceso"
	@echo "  ✅ Buffer Manager con algoritmo Clock PIN-AWARE"
	@echo "  ✅ Soporte para registros fijos y variables"
	@echo "  ✅ Integración completa disco-memoria-índice"
	@echo "  ✅ Simulación realista de SGBD"
	@echo ""
	@echo "Flujo de consulta:"
	@echo "  1️⃣  SELECT -> B+ Tree (localizar referencia)"
	@echo "  2️⃣  B+ Tree -> BufferManager (verificar memoria)"
	@echo "  3️⃣  BufferManager -> DiskManager (cargar si necesario)"
	@echo "  4️⃣  Retornar registro completo"

# Objetivo por defecto
.DEFAULT_GOAL := help
