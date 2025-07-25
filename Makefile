# Makefile para SGBD Completo con Sistema Distribuido Modularizado
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -Wno-sign-compare -Wno-unused-variable
INCLUDES = -I./include
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Targets principales
TARGET_MAIN = sgbd_main
TARGET_DISTRIBUTED = sgbd_distributed
TARGET_INTERACTIVE = sgbd_interactive

# Archivos fuente
MAIN_SRC = $(SRCDIR)/main_extended.cpp
DISTRIBUTED_MAIN_SRC = $(SRCDIR)/main_distributed.cpp
DISTRIBUTED_SRC = $(SRCDIR)/SGBDDistributed.cpp $(SRCDIR)/SGBDDistributed_Interface.cpp
INTERACTIVE_SRC = sgbd_distribuido_gps.cpp

# Headers requeridos - ESTRUCTURA COMPLETA
CORE_HEADERS = include/Record.h \
               include/PhysicalAddress.h \
               include/Block.h \
               include/DiskManager.h \
               include/DiskManagerExtended.h \
               include/RecordReference.h

HASH_HEADERS = include/HashExtendible/ExtensibleHash.h \
               include/HashExtendible/Directory.h \
               include/HashExtendible/Bucket.h \
               include/HashExtendible/HashFunction.h \
               include/HashExtendible/HashEntry.h

BTREE_HEADERS = include/BPlusTree/BPlusTree.h \
                include/BPlusTree/BPlusNode.h \
                include/BPlusTree/LeafNode.h \
                include/BPlusTree/InternalNode.h \
                include/BPlusTree/KeyComparator.h

BUFFER_HEADERS = include/buffer/BufferManagerClock.h \
                 include/buffer/BufferPoolManager.h \
                 include/buffer/PageTable.h \
                 include/buffer/ClockReplacer.h \
                 include/buffer/LRUReplacer.h \
                 include/buffer/PageDirectory.h

DISTRIBUTED_HEADERS = include/SGBDDistributed.h

ALL_HEADERS = $(CORE_HEADERS) $(HASH_HEADERS) $(BTREE_HEADERS) $(BUFFER_HEADERS) $(DISTRIBUTED_HEADERS)

# Objetos compilados
DISTRIBUTED_OBJS = $(OBJDIR)/SGBDDistributed.o $(OBJDIR)/SGBDDistributed_Interface.o

# Crear directorios si no existen
$(shell mkdir -p $(OBJDIR) $(BINDIR) data)

.PHONY: all clean run demo test help check-headers check-data distributed main interactive

# Verificar headers antes de compilar
check-headers:
	@echo "🔍 Verificando headers requeridos..."
	@missing_headers=""; \
	for header in $(ALL_HEADERS); do \
		if [ ! -f "$$header" ]; then \
			echo "❌ Header faltante: $$header"; \
			missing_headers="$$missing_headers $$header"; \
		fi \
	done; \
	if [ -n "$$missing_headers" ]; then \
		echo ""; \
		echo "🚨 HEADERS FALTANTES DETECTADOS:"; \
		echo "$$missing_headers"; \
		echo ""; \
		echo "📋 SOLUCIÓN:"; \
		echo "   1. Crear include/RecordReference.h (código proporcionado)"; \
		echo "   2. Crear include/SGBDDistributed.h (código proporcionado)"; \
		echo "   3. Verificar estructura completa de headers"; \
		echo ""; \
		exit 1; \
	else \
		echo "✅ Todos los headers encontrados"; \
	fi

# Verificar datos
check-data:
	@echo "📁 Verificando archivos de datos..."
	@if [ ! -f "data/data-GPS.csv" ]; then \
		echo "⚠️  Archivo data/data-GPS.csv no encontrado"; \
		echo "   Creando directorio data/ y archivo de muestra..."; \
		mkdir -p data; \
		echo '"id","imei","commandId","timestamp","latitude","longitude","recordIndex","timestampExtension","recordExtension","priority","altitude","angle","satellites","speed","hdop","eventId","punto","ioElements","processedAt","createdAt","updatedAt"' > data/data-GPS.csv; \
		echo '"1","868018070237402","68","2025-06-25 00:47:02+00","-16.4103100","-71.5309216","0","0","0","0","2345.8","55.4","5","0","2.0","7","POINT","{}","2025-06-25 00:47:48+00","2025-06-25 00:47:48+00","2025-06-25 00:47:48+00"' >> data/data-GPS.csv; \
		echo "✅ Archivo de muestra creado"; \
	else \
		echo "✅ data/data-GPS.csv encontrado"; \
	fi

# Objetivo principal - compilar todo
all: check-headers check-data $(BINDIR)/$(TARGET_MAIN) $(BINDIR)/$(TARGET_DISTRIBUTED) $(BINDIR)/$(TARGET_INTERACTIVE)

# Compilar sistema principal integrado
main: check-headers $(BINDIR)/$(TARGET_MAIN)

$(BINDIR)/$(TARGET_MAIN): $(MAIN_SRC) $(ALL_HEADERS)
	@echo "🔧 Compilando sistema principal integrado..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MAIN_SRC)
	@echo "✅ Sistema principal compilado: $@"

# Compilar sistema distribuido modular
distributed: check-headers check-data $(BINDIR)/$(TARGET_DISTRIBUTED)

$(OBJDIR)/SGBDDistributed.o: $(SRCDIR)/SGBDDistributed.cpp $(ALL_HEADERS)
	@echo "🔧 Compilando SGBDDistributed.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/SGBDDistributed_Interface.o: $(SRCDIR)/SGBDDistributed_Interface.cpp $(ALL_HEADERS)
	@echo "🔧 Compilando SGBDDistributed_Interface.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/main_distributed.o: $(DISTRIBUTED_MAIN_SRC) $(ALL_HEADERS)
	@echo "🔧 Compilando main_distributed.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BINDIR)/$(TARGET_DISTRIBUTED): $(DISTRIBUTED_OBJS) $(OBJDIR)/main_distributed.o
	@echo "🔧 Enlazando sistema distribuido modular..."
	$(CXX) $(CXXFLAGS) -o $@ $(DISTRIBUTED_OBJS) $(OBJDIR)/main_distributed.o
	@echo "✅ Sistema distribuido modular compilado: $@"

# Compilar sistema interactivo monolítico (compatibilidad)
interactive: check-headers check-data $(BINDIR)/$(TARGET_INTERACTIVE)

$(BINDIR)/$(TARGET_INTERACTIVE): $(INTERACTIVE_SRC) $(ALL_HEADERS)
	@echo "🔧 Compilando sistema interactivo..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(INTERACTIVE_SRC)
	@echo "✅ Sistema interactivo compilado: $@"

# Ejecutar sistema principal
run-main: $(BINDIR)/$(TARGET_MAIN)
	@echo "🚀 Ejecutando sistema principal integrado..."
	@echo "=================================================="
	./$(BINDIR)/$(TARGET_MAIN)

# Ejecutar sistema distribuido
run-distributed: $(BINDIR)/$(TARGET_DISTRIBUTED)
	@echo "🚀 Ejecutando sistema distribuido modular..."
	@echo "=================================================="
	./$(BINDIR)/$(TARGET_DISTRIBUTED)

# Ejecutar sistema interactivo
run-interactive: $(BINDIR)/$(TARGET_INTERACTIVE)
	@echo "🚀 Ejecutando sistema interactivo..."
	@echo "=================================================="
	./$(BINDIR)/$(TARGET_INTERACTIVE)

# Demo completo - ejecutar el sistema principal
demo: main
	@echo "🎯 DEMOSTRACIÓN COMPLETA DEL SGBD INTEGRADO"
	@echo "============================================"
	@echo ""
	@echo "📋 Sistema incluye:"
	@echo "   • SGBD tradicional con Buffer Pool"
	@echo "   • Sistema distribuido (Hash + B+ Tree)"
	@echo "   • Carga de datasets (Housing, GPS)"
	@echo "   • Comparación de algoritmos LRU vs Clock"
	@echo ""
	@echo "🎯 Opciones disponibles:"
	@echo "   1-30: Sistema SGBD tradicional"
	@echo "   31-34: Sistema distribuido interactivo"
	@echo ""
	@echo "▶️  Iniciando sistema principal..."
	@echo "=================================================="
	./$(BINDIR)/$(TARGET_MAIN)

# Demo específico del sistema distribuido
demo-distributed: distributed
	@echo "🎯 DEMO DEL SISTEMA DISTRIBUIDO"
	@echo "================================"
	@echo ""
	@echo "📋 Características:"
	@echo "   • Hash Extensible para IMEI (S1)"
	@echo "   • B+ Tree para Timestamp (S2)"
	@echo "   • Routing automático vs manual"
	@echo "   • Consultas SQL interactivas"
	@echo "   • Dataset GPS completo"
	@echo ""
	@echo "▶️  Iniciando sistema distribuido..."
	@echo "=================================="
	./$(BINDIR)/$(TARGET_DISTRIBUTED)

# Verificar sintaxis
test: $(MAIN_SRC) $(DISTRIBUTED_SRC) $(INTERACTIVE_SRC) $(ALL_HEADERS)
	@echo "🧪 Verificando sintaxis de todos los archivos..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(MAIN_SRC)
	@echo "✅ main_extended.cpp - OK"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(SRCDIR)/SGBDDistributed.cpp
	@echo "✅ SGBDDistributed.cpp - OK"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(SRCDIR)/SGBDDistributed_Interface.cpp
	@echo "✅ SGBDDistributed_Interface.cpp - OK"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(INTERACTIVE_SRC)
	@echo "✅ sgbd_distribuido_gps.cpp - OK"
	@echo "✅ Sintaxis correcta en todos los archivos"

# Compilar versión debug
debug: CXXFLAGS += -DDEBUG -g3 -fsanitize=address
debug: all
	@echo "🐛 Versión debug compilada con AddressSanitizer"

# Compilar versión release optimizada
release: CXXFLAGS = -std=c++17 -O3 -DNDEBUG -march=native -Wno-sign-compare
release: all
	@echo "🚀 Versión release optimizada compilada"

# Benchmark del sistema
benchmark: release
	@echo "⏱️  Ejecutando benchmark del sistema..."
	@echo "Midiendo tiempos de respuesta para diferentes operaciones..."
	time ./$(BINDIR)/$(TARGET_DISTRIBUTED)

# Limpiar archivos compilados
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -rf $(OBJDIR)/* $(BINDIR)/*
	@echo "✅ Limpieza completada"

# Limpiar todo incluyendo datos
clean-all: clean
	@echo "🧹 Limpiando datos generados..."
	rm -rf data/
	@echo "✅ Limpieza completa"

# Información de ayuda
help:
	@echo "🛠️  MAKEFILE SGBD COMPLETO MODULARIZADO"
	@echo "======================================="
	@echo ""
	@echo "Objetivos principales:"
	@echo "  make all               - Compilar todo el sistema"
	@echo "  make main              - Compilar sistema principal"
	@echo "  make distributed       - Compilar sistema distribuido"
	@echo "  make interactive       - Compilar sistema interactivo"
	@echo ""
	@echo "Ejecución:"
	@echo "  make run-main          - Ejecutar sistema principal"
	@echo "  make run-distributed   - Ejecutar sistema distribuido"
	@echo "  make run-interactive   - Ejecutar sistema interactivo"
	@echo "  make demo              - Demo completo del sistema principal"
	@echo "  make demo-distributed  - Demo específico del sistema distribuido"
	@echo ""
	@echo "Desarrollo:"
	@echo "  make test              - Verificar sintaxis"
	@echo "  make debug             - Compilar versión debug"
	@echo "  make release           - Compilar versión optimizada"
	@echo "  make benchmark         - Benchmark de rendimiento"
	@echo ""
	@echo "Utilidades:"
	@echo "  make check-headers     - Verificar headers requeridos"
	@echo "  make check-data        - Verificar archivos de datos"
	@echo "  make clean             - Limpiar compilados"
	@echo "  make clean-all         - Limpiar todo + datos"
	@echo "  make help              - Mostrar esta ayuda"
	@echo ""
	@echo "Estructura del proyecto:"
	@echo "  📁 src/               - Código fuente modular"
	@echo "  📁 include/           - Headers (.h)"
	@echo "  📁 bin/               - Ejecutables"
	@echo "  📁 obj/               - Objetos compilados"
	@echo "  📁 data/              - Datasets (GPS, Housing)"
	@echo ""
	@echo "Componentes integrados:"
	@echo "  🏢 SGBD Principal     - Sistema tradicional completo"
	@echo "  🌐 Sistema Distribuido - Hash + B+ Tree especializado"
	@echo "  💾 Buffer Management  - LRU + Clock algorithms"
	@echo "  📊 Datasets          - Housing, GPS, Titanic"
	@echo ""
	@echo "Flujo recomendado:"
	@echo "  1. make check-headers  (verificar dependencias)"
	@echo "  2. make all           (compilar todo)"
	@echo "  3. make demo          (ejecutar demo principal)"
	@echo "  4. make demo-distributed (ejecutar demo distribuido)"

# Información del proyecto
info:
	@echo "📋 INFORMACIÓN DEL PROYECTO SGBD COMPLETO"
	@echo "=========================================="
	@echo "Proyecto: SGBD Físico + Sistema Distribuido Modularizado"
	@echo "Lenguaje: C++17"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo ""
	@echo "Arquitectura del sistema:"
	@echo "  🏢 Sistema Principal:"
	@echo "     ├─ DiskManager + Buffer Pool"
	@echo "     ├─ Tablas tradicionales"
	@echo "     ├─ Datasets predefinidos"
	@echo "     └─ Algoritmos LRU vs Clock"
	@echo ""
	@echo "  🌐 Sistema Distribuido:"
	@echo "     ├─ Servidor S1 (Hash Extensible - IMEI)"
	@echo "     ├─ Servidor S2 (B+ Tree - Timestamp)"
	@echo "     ├─ Query Router inteligente"
	@echo "     └─ Interfaz SQL interactiva"
	@echo ""
	@echo "Datasets soportados:"
	@echo "  📊 Housing (545 registros) - Sistema principal"
	@echo "  📍 GPS (variable) - Sistema distribuido"
	@echo "  🚢 Titanic (891 registros) - Sistema principal"
	@echo ""
	@echo "Características técnicas:"
	@echo "  ✅ Modularización completa"
	@echo "  ✅ Headers bien organizados"
	@echo "  ✅ Compilación separada"
	@echo "  ✅ Múltiples puntos de entrada"
	@echo "  ✅ Sistema de build robusto"

# Objetivo por defecto
.DEFAULT_GOAL := helps