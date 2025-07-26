# Makefile para SGBD Completo con Índices Especializados Integrados
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -Wno-sign-compare -Wno-unused-variable -Wno-unused-parameter
INCLUDES = -I./include
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Target principal
TARGET_MAIN = sgbd_indices_completo

# Archivos fuente
MAIN_SRC = $(SRCDIR)/main.cpp

# Headers requeridos - ESTRUCTURA COMPLETA CON ÍNDICES
CORE_HEADERS = include/Record.h \
               include/PhysicalAddress.h \
               include/Block.h \
               include/DiskManager.h \
               include/DiskManagerExtended.h \
               include/RecordReference.h \
               include/IndexManager.h

HASH_HEADERS = include/HashExtendible/ExtensibleHash.h \
               include/HashExtendible/Directory.h \
               include/HashExtendible/Bucket.h \
               include/HashExtendible/HashFunction.h

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

ALL_HEADERS = $(CORE_HEADERS) $(HASH_HEADERS) $(BTREE_HEADERS) $(BUFFER_HEADERS)

# Crear directorios si no existen
$(shell mkdir -p $(OBJDIR) $(BINDIR) data)

.PHONY: all clean run demo test help check-headers check-data info

# Verificar headers antes de compilar
check-headers:
	@echo "🔍 Verificando headers requeridos para sistema integrado..."
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
		echo "   1. Crear include/RecordReference.h"; \
		echo "   2. Crear include/IndexManager.h"; \
		echo "   3. Crear todos los headers de HashExtendible/"; \
		echo "   4. Crear todos los headers de BPlusTree/"; \
		echo "   5. Verificar estructura completa de headers"; \
		echo ""; \
		exit 1; \
	else \
		echo "✅ Todos los headers encontrados ($(words $(ALL_HEADERS)) archivos)"; \
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
		for i in $$(seq 2 100); do \
			imei_suffix=$$(printf "%02d" $$((i % 100))); \
			timestamp="2025-06-25 0$$(printf "%01d" $$((i % 24))):$$(printf "%02d" $$((i % 60))):$$(printf "%02d" $$((i % 60)))+00"; \
			lat="-16.41$$(printf "%02d" $$((i % 100)))"; \
			lon="-71.53$$(printf "%02d" $$((i % 100)))"; \
			echo "\"$$i\",\"8680180702374$$imei_suffix\",\"68\",\"$$timestamp\",\"$$lat\",\"$$lon\",\"0\",\"0\",\"0\",\"0\",\"234$$i\",\"$$((i*10))\",\"$$((5 + i % 10))\",\"0\",\"$$((i % 5))\",\"7\",\"POINT\",\"{}\",\"$$timestamp\",\"$$timestamp\",\"$$timestamp\"" >> data/data-GPS.csv; \
		done; \
		echo "✅ Archivo de muestra creado con 100 registros GPS"; \
	else \
		record_count=$$(wc -l < data/data-GPS.csv); \
		echo "✅ data/data-GPS.csv encontrado ($$record_count líneas)"; \
	fi

# Crear estructura de directorios necesaria
setup-dirs:
	@echo "📁 Creando estructura de directorios..."
	@mkdir -p $(OBJDIR) $(BINDIR) data
	@mkdir -p bin/mi_disco_sgbde/metadata
	@mkdir -p bin/mi_disco_sgbde/platter_0/surface_0/track_0
	@mkdir -p bin/mi_disco_sgbde/platter_0/surface_1/track_0
	@echo "✅ Estructura de directorios creada"

# Objetivo principal - compilar sistema integrado
all: check-headers check-data setup-dirs $(BINDIR)/$(TARGET_MAIN)

$(BINDIR)/$(TARGET_MAIN): $(MAIN_SRC) $(ALL_HEADERS)
	@echo "🔧 Compilando sistema SGBD integrado con índices..."
	@echo "   Source: $(MAIN_SRC)"
	@echo "   Headers: $(words $(ALL_HEADERS)) archivos"
	@echo "   Flags: $(CXXFLAGS)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MAIN_SRC)
	@echo "✅ Sistema compilado exitosamente: $@"
	@echo ""
	@echo "🎯 PARA EJECUTAR:"
	@echo "   make run    - Ejecutar sistema interactivo"
	@echo "   make demo   - Ejecutar demo educativo"

# Ejecutar el sistema
run: $(BINDIR)/$(TARGET_MAIN)
	@echo "🚀 INICIANDO SGBD CON ÍNDICES ESPECIALIZADOS"
	@echo "=============================================="
	@echo ""
	@echo "📋 Sistema disponible:"
	@echo "   • Hash Extensible para IMEI (Server A)"
	@echo "   • B+ Tree para Timestamp (Server B)"
	@echo "   • Persistencia de índices"
	@echo "   • Buffer Pool Manager (LRU/Clock)"
	@echo "   • Consultas SQL educativas"
	@echo ""
	@echo "▶️  Iniciando interfaz interactiva..."
	@echo "=============================================="
	./$(BINDIR)/$(TARGET_MAIN)

# Demo educativo
demo: $(BINDIR)/$(TARGET_MAIN)
	@echo "🎓 DEMO EDUCATIVO DEL SGBD CON ÍNDICES"
	@echo "======================================"
	@echo ""
	@echo "Este demo muestra:"
	@echo "1. Carga de datos GPS"
	@echo "2. Construcción de índices"
	@echo "3. Consultas con Hash Extensible"
	@echo "4. Consultas con B+ Tree"
	@echo "5. Persistencia de índices"
	@echo ""
	@echo "▶️  Iniciando demo..."
	@echo "======================================"
	./$(BINDIR)/$(TARGET_MAIN)

# Verificar sintaxis de todos los archivos
test: $(MAIN_SRC) $(ALL_HEADERS)
	@echo "🧪 Verificando sintaxis del sistema completo..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(MAIN_SRC)
	@echo "✅ main.cpp - Sintaxis correcta"
	@echo "✅ Verificación completa - Sin errores de sintaxis"

# Compilar versión debug con información adicional
debug: CXXFLAGS += -DDEBUG -g3 -fsanitize=address -fsanitize=undefined
debug: all
	@echo "🐛 Versión debug compilada con sanitizers"
	@echo "   AddressSanitizer: Detecta leaks de memoria"
	@echo "   UBSanitizer: Detecta comportamiento indefinido"

# Compilar versión release optimizada
release: CXXFLAGS = -std=c++17 -O3 -DNDEBUG -march=native -Wno-sign-compare -DRELEASE
release: all
	@echo "🚀 Versión release optimizada compilada"
	@echo "   Optimizaciones: -O3 -march=native"
	@echo "   Definiciones: -DNDEBUG -DRELEASE"

# Benchmark del sistema
benchmark: release
	@echo "⏱️  Ejecutando benchmark del sistema de índices..."
	@echo "Midiendo tiempos de respuesta para:"
	@echo "  • Construcción de índices"
	@echo "  • Búsquedas Hash Extensible"
	@echo "  • Búsquedas B+ Tree"
	@echo "  • Operaciones de persistencia"
	time ./$(BINDIR)/$(TARGET_MAIN)

# Análisis de memoria
memory-check: debug
	@echo "🔍 Ejecutando análisis de memoria..."
	@echo "Verificando leaks y uso de memoria..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose ./$(BINDIR)/$(TARGET_MAIN)

# Limpiar archivos compilados y temporales
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -rf $(OBJDIR)/*
	rm -rf $(BINDIR)/*
	@echo "✅ Archivos compilados eliminados"

# Limpiar todo incluyendo datos simulados
clean-all: clean
	@echo "🧹 Limpiando datos de simulación..."
	rm -rf bin/mi_disco_sgbde/
	rm -f data/data-GPS.csv
	@echo "✅ Limpieza completa realizada"

# Información detallada del proyecto
info:
	@echo "📋 INFORMACIÓN DEL PROYECTO SGBD CON ÍNDICES ESPECIALIZADOS"
	@echo "============================================================="
	@echo "Proyecto: SGBD Físico Educativo con Hash Extensible y B+ Tree"
	@echo "Lenguaje: C++17"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo ""
	@echo "🏗️ Arquitectura del sistema:"
	@echo "  📦 Núcleo SGBD:"
	@echo "     ├─ DiskManager + FileSystemSimulator"
	@echo "     ├─ Buffer Pool Manager (LRU/Clock)"
	@echo "     ├─ Page Directory persistente"
	@echo "     └─ Record Management (Fixed/Variable)"
	@echo ""
	@echo "  🔍 Sistema de Índices:"
	@echo "     ├─ Hash Extensible (IMEI)"
	@echo "     │  ├─ Directory dinámico"
	@echo "     │  ├─ Buckets con capacidad configurable"
	@echo "     │  └─ Splits automáticos"
	@echo "     └─ B+ Tree (Timestamp)"
	@echo "        ├─ Nodos internos y hojas"
	@echo "        ├─ Enlaces horizontales"
	@echo "        └─ Búsquedas por rango"
	@echo ""
	@echo "  💾 Persistencia:"
	@echo "     ├─ IndexManager para save/load"
	@echo "     ├─ Metadatos de índices"
	@echo "     └─ Reconstrucción automática"
	@echo ""
	@echo "📊 Componentes principales:"
	@echo "  Core Headers: $(words $(CORE_HEADERS))"
	@echo "  Hash Headers: $(words $(HASH_HEADERS))"
	@echo "  B+Tree Headers: $(words $(BTREE_HEADERS))"
	@echo "  Buffer Headers: $(words $(BUFFER_HEADERS))"
	@echo "  Total Headers: $(words $(ALL_HEADERS))"
	@echo ""
	@echo "🎯 Funcionalidades:"
	@echo "  ✅ Servidor A: Hash Extensible (Transaccional)"
	@echo "  ✅ Servidor B: B+ Tree (Analítico)"
	@echo "  ✅ Consultas SQL educativas"
	@echo "  ✅ Visualización de estructuras"
	@echo "  ✅ Persistencia automática"
	@echo "  ✅ Flujo educativo completo"
	@echo ""
	@echo "📁 Datasets soportados:"
	@echo "  📍 GPS (2025-GPS.csv) - Sistema especializado"
	@echo "     • IMEI → Hash Extensible O(1)"
	@echo "     • Timestamp → B+ Tree O(log n)"
	@echo "     • 21 campos por registro"
	@echo "     • Soporte para 5000+ registros"

# Mostrar ayuda
help:
	@echo "📚 AYUDA DEL MAKEFILE - SGBD CON ÍNDICES"
	@echo "========================================"
	@echo ""
	@echo "🎯 Objetivos principales:"
	@echo "  make all          - Compilar sistema completo"
	@echo "  make run          - Ejecutar sistema interactivo"
	@echo "  make demo         - Ejecutar demo educativo"
	@echo ""
	@echo "🔧 Compilación:"
	@echo "  make debug        - Compilar con debug + sanitizers"
	@echo "  make release      - Compilar optimizado para producción"
	@echo "  make test         - Verificar sintaxis únicamente"
	@echo ""
	@echo "🧪 Testing y análisis:"
	@echo "  make benchmark    - Benchmark de rendimiento"
	@echo "  make memory-check - Análisis de memoria con Valgrind"
	@echo ""
	@echo "🧹 Limpieza:"
	@echo "  make clean        - Limpiar archivos compilados"
	@echo "  make clean-all    - Limpieza completa (incluye datos)"
	@echo ""
	@echo "ℹ️  Información:"
	@echo "  make info         - Información detallada del proyecto"
	@echo "  make check-headers - Verificar headers requeridos"
	@echo "  make check-data   - Verificar archivos de datos"
	@echo ""
	@echo "📋 Estructura de archivos requerida:"
	@echo "  src/main.cpp                     - Programa principal"
	@echo "  include/IndexManager.h          - Gestor de persistencia"
	@echo "  include/HashExtendible/*.h       - Hash Extensible"
	@echo "  include/BPlusTree/*.h            - B+ Tree"
	@echo "  include/buffer/*.h               - Buffer Management"
	@echo "  data/data-GPS.csv               - Dataset GPS"
	@echo ""
	@echo "🚀 Para empezar:"
	@echo "  1. make check-headers"
	@echo "  2. make all"
	@echo "  3. make run"

# Objetivo por defecto
.DEFAULT_GOAL := help

# Información de compilación en tiempo real
$(BINDIR)/$(TARGET_MAIN): 
	@echo "🔨 INICIANDO COMPILACIÓN..."
	@echo "   📁 Directorio fuente: $(SRCDIR)"
	@echo "   📁 Directorio destino: $(BINDIR)"
	@echo "   🔧 Compilador: $(CXX)"
	@echo "   ⚙️  Flags: $(CXXFLAGS)"
	@echo "   📚 Headers incluidos: $(words $(ALL_HEADERS))"
	@echo ""
	@echo "🏗️ Compilando..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MAIN_SRC)
	@echo ""
	@echo "✅ COMPILACIÓN EXITOSA!"
	@echo "   📦 Ejecutable: $@"
	@echo "   📊 Tamaño: $$(du -h $@ | cut -f1)"
	@echo ""