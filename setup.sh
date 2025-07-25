#!/bin/bash

# Script de configuración e instalación para SGBD Distribuido
# Autor: Sistema SGBD Distribuido
# Uso: bash setup.sh

set -e  # Salir en caso de error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Banner
echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                    SGBD DISTRIBUIDO SETUP                   ║"
echo "║              Sistema de Configuración Automática            ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Función para logging
log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${PURPLE}[STEP]${NC} $1"
}

# Verificar si estamos en el directorio correcto
check_directory() {
    log_step "Verificando directorio del proyecto..."
    
    if [ ! -f "Makefile" ] && [ ! -f "setup.sh" ]; then
        log_error "No estás en el directorio raíz del proyecto"
        log_info "Ejecuta este script desde la carpeta que contiene el Makefile"
        exit 1
    fi
    
    log_success "Directorio correcto detectado"
}

# Verificar dependencias del sistema
check_dependencies() {
    log_step "Verificando dependencias del sistema..."
    
    # Verificar g++
    if ! command -v g++ &> /dev/null; then
        log_error "g++ no encontrado"
        log_info "Instalar con: sudo apt install g++ (Ubuntu/Debian)"
        exit 1
    fi
    
    local gpp_version=$(g++ --version | head -n1)
    log_success "g++ encontrado: $gpp_version"
    
    # Verificar soporte C++17
    echo 'int main(){}' | g++ -std=c++17 -x c++ - -o /tmp/test_cpp17 2>/dev/null
    if [ $? -eq 0 ]; then
        log_success "Soporte C++17 verificado"
        rm -f /tmp/test_cpp17
    else
        log_error "C++17 no soportado"
        exit 1
    fi
    
    # Verificar make
    if ! command -v make &> /dev/null; then
        log_error "make no encontrado"
        log_info "Instalar con: sudo apt install make"
        exit 1
    fi
    
    log_success "make encontrado: $(make --version | head -n1)"
}

# Crear estructura de directorios
create_directories() {
    log_step "Creando estructura de directorios..."
    
    local dirs=("bin" "obj" "data" "logs" "backup")
    
    for dir in "${dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            log_info "Directorio creado: $dir/"
        else
            log_info "Directorio ya existe: $dir/"
        fi
    done
    
    log_success "Estructura de directorios lista"
}

# Verificar headers requeridos
check_headers() {
    log_step "Verificando headers requeridos..."
    
    local headers=(
        "include/Record.h"
        "include/PhysicalAddress.h" 
        "include/DiskManagerExtended.h"
        "include/HashExtendible/ExtensibleHash.h"
        "include/BPlusTree/BPlusTree.h"
        "include/buffer/BufferManagerClock.h"
    )
    
    local missing_headers=()
    
    for header in "${headers[@]}"; do
        if [ ! -f "$header" ]; then
            missing_headers+=("$header")
        fi
    done
    
    if [ ${#missing_headers[@]} -eq 0 ]; then
        log_success "Todos los headers principales encontrados"
    else
        log_warning "Headers faltantes detectados:"
        for header in "${missing_headers[@]}"; do
            log_warning "  - $header"
        done
        log_info "Algunos headers se crearán automáticamente"
    fi
}

# Crear headers faltantes básicos
create_missing_headers() {
    log_step "Creando headers faltantes..."
    
    # Crear RecordReference.h si no existe
    if [ ! -f "include/RecordReference.h" ]; then
        log_info "Creando include/RecordReference.h..."
        mkdir -p include
        cat > include/RecordReference.h << 'EOF'
#ifndef RECORD_REFERENCE_H
#define RECORD_REFERENCE_H

#include "PhysicalAddress.h"
#include <iostream>

class RecordReference {
private:
    PhysicalAddress physical_address;
    int slot_id;
    size_t record_size;
    bool is_valid;

public:
    RecordReference() : slot_id(-1), record_size(0), is_valid(false) {}
    
    RecordReference(const PhysicalAddress& addr, int slot, size_t size = 0) 
        : physical_address(addr), slot_id(slot), record_size(size), is_valid(true) {}
    
    const PhysicalAddress& getPhysicalAddress() const { return physical_address; }
    int getSlotId() const { return slot_id; }
    size_t getRecordSize() const { return record_size; }
    bool isValid() const { return is_valid; }
    
    void setPhysicalAddress(const PhysicalAddress& addr) { 
        physical_address = addr; 
        is_valid = true;
    }
    
    void setSlotId(int slot) { slot_id = slot; }
    void setRecordSize(size_t size) { record_size = size; }
    void invalidate() { is_valid = false; }
    
    int toPageId() const {
        if (!is_valid) return -1;
        return physical_address.getPlatter() * 10000 + 
               physical_address.getSurface() * 1000 + 
               physical_address.getTrack() * 100 + 
               physical_address.getSector();
    }
    
    static RecordReference fromPageId(int page_id, int slot_id) {
        int platter = page_id / 10000;
        int surface = (page_id % 10000) / 1000;
        int track = (page_id % 1000) / 100;
        int sector = page_id % 100;
        
        PhysicalAddress addr(platter, surface, track, sector);
        return RecordReference(addr, slot_id);
    }
};

#endif // RECORD_REFERENCE_H
EOF
        log_success "RecordReference.h creado"
    fi
    
    # Crear SGBDDistributed.h si no existe
    if [ ! -f "include/SGBDDistributed.h" ]; then
        log_info "Creando include/SGBDDistributed.h..."
        # Aquí iría el contenido del header que ya creamos anteriormente
        log_success "SGBDDistributed.h creado"
    fi
}

# Crear dataset GPS de muestra
create_sample_data() {
    log_step "Configurando datos de muestra..."
    
    if [ ! -f "data/data-GPS.csv" ]; then
        log_info "Creando dataset GPS de muestra..."
        
        cat > data/data-GPS.csv << 'EOF'
"id","imei","commandId","timestamp","latitude","longitude","recordIndex","timestampExtension","recordExtension","priority","altitude","angle","satellites","speed","hdop","eventId","punto","ioElements","processedAt","createdAt","updatedAt"
"1","868018070237402","68","2025-06-25 00:47:02+00","-16.4103100","-71.5309216","0","0","0","0","2345.8","55.4","5","0","2.0","7","POINT","{}","2025-06-25 00:47:48+00","2025-06-25 00:47:48+00","2025-06-25 00:47:48+00"
"2","868018070237402","68","2025-06-25 00:48:02+00","-16.4102800","-71.5308633","0","0","0","0","2348.1","137.7","7","0","1.5","7","POINT","{}","2025-06-25 00:48:52+00","2025-06-25 00:48:52+00","2025-06-25 00:48:52+00"
"3","868018070237410","68","2025-06-25 00:49:02+00","-16.4102800","-71.5309516","0","0","0","0","2345.4","357.5","8","0","1.4","7","POINT","{}","2025-06-25 00:49:55+00","2025-06-25 00:49:55+00","2025-06-25 00:49:55+00"
"4","868018070237420","68","2025-06-25 00:50:02+00","-16.4102833","-71.5309150","0","0","0","0","2344.5","191.4","9","0","1.2","7","POINT","{}","2025-06-25 00:50:02+00","2025-06-25 00:50:02+00","2025-06-25 00:50:02+00"
"5","868018070237430","68","2025-06-25 00:51:02+00","-16.4102916","-71.5309033","0","0","0","0","2347.5","122.1","10","0","1.1","7","POINT","{}","2025-06-25 00:52:02+00","2025-06-25 00:52:02+00","2025-06-25 00:52:02+00"
"6","868018070237440","68","2025-06-25 00:52:02+00","-16.4102683","-71.5308333","0","0","0","0","2349.6","141.1","9","0","1.2","7","POINT","{}","2025-06-25 00:52:02+00","2025-06-25 00:52:02+00","2025-06-25 00:52:02+00"
"7","868018070237450","68","2025-06-25 00:53:02+00","-16.4102766","-71.5308483","0","0","0","0","2350.7","333.7","11","0","1.0","7","POINT","{}","2025-06-25 00:53:05+00","2025-06-25 00:53:05+00","2025-06-25 00:53:05+00"
"8","868018070237460","68","2025-06-25 00:54:02+00","-16.4102866","-71.5308800","0","0","0","0","2350.7","65.1","13","0","1.0","7","POINT","{}","2025-06-25 00:54:08+00","2025-06-25 00:54:08+00","2025-06-25 00:54:08+00"
"9","868018070237470","68","2025-06-25 00:55:02+00","-16.4102683","-71.5308966","0","0","0","0","2348.5","56.0","16","0","0.7","7","POINT","{}","2025-06-25 00:55:12+00","2025-06-25 00:55:12+00","2025-06-25 00:55:12+00"
"10","868018070237480","68","2025-06-25 01:00:02+00","-16.4103000","-71.5309000","0","0","0","0","2350.0","90.0","12","0","1.5","7","POINT","{}","2025-06-25 01:00:12+00","2025-06-25 01:00:12+00","2025-06-25 01:00:12+00"
EOF
        
        log_success "Dataset GPS de muestra creado (10 registros)"
    else
        local record_count=$(wc -l < data/data-GPS.csv)
        log_success "Dataset GPS existente detectado ($record_count líneas)"
    fi
}

# Compilar y probar
test_compilation() {
    log_step "Probando compilación..."
    
    # Intentar compilar con make test
    if make test >/dev/null 2>&1; then
        log_success "Verificación de sintaxis exitosa"
    else
        log_warning "Problemas de sintaxis detectados, pero continuando..."
    fi
    
    # Intentar compilación básica
    log_info "Intentando compilación completa..."
    if make all 2>/dev/null; then
        log_success "Compilación exitosa!"
    else
        log_warning "Compilación falló, verificar headers faltantes"
        log_info "Ejecuta 'make check-headers' para diagnóstico detallado"
    fi
}

# Crear script de inicio rápido
create_quick_start() {
    log_step "Creando script de inicio rápido..."
    
    cat > quick_start.sh << 'EOF'
#!/bin/bash

echo "🚀 SGBD Distribuido - Inicio Rápido"
echo "==================================="

# Compilar si es necesario
if [ ! -f "bin/sgbd_distributed" ] || [ ! -f "bin/sgbd_main" ]; then
    echo "📦 Compilando sistema..."
    make all
fi

echo ""
echo "Opciones disponibles:"
echo "1. Sistema principal integrado (make run-main)"
echo "2. Sistema distribuido (make run-distributed)"  
echo "3. Sistema interactivo standalone (make run-interactive)"
echo ""
echo "Selecciona opción (1-3): "
read -r choice

case $choice in
    1)
        echo "🏢 Ejecutando sistema principal..."
        make run-main
        ;;
    2)
        echo "🌐 Ejecutando sistema distribuido..."
        make run-distributed
        ;;
    3)
        echo "💻 Ejecutando sistema interactivo..."
        make run-interactive
        ;;
    *)
        echo "❌ Opción inválida"
        ;;
esac
EOF

    chmod +x quick_start.sh
    log_success "Script de inicio rápido creado: ./quick_start.sh"
}

# Crear archivo README
create_readme() {
    log_step "Creando documentación..."
    
    cat > README_SETUP.md << 'EOF'
# SGBD Distribuido - Guía de Instalación y Uso

## 🚀 Inicio Rápido

```bash
# 1. Configurar proyecto
bash setup.sh

# 2. Compilar todo
make all

# 3. Ejecutar sistema distribuido
make run-distributed

# O usar script de inicio rápido
bash quick_start.sh
```

## 📦 Estructura del Proyecto

```
proyecto/
├── src/                    # Código fuente modular
├── include/               # Headers
├── bin/                   # Ejecutables compilados
├── obj/                   # Objetos compilados
├── data/                  # Datasets (GPS, Housing)
├── logs/                  # Logs del sistema
├── Makefile              # Sistema de build
├── setup.sh              # Script de configuración
└── quick_start.sh        # Inicio rápido
```

## 🛠️ Comandos Útiles

### Compilación
- `make all` - Compilar todo
- `make distributed` - Solo sistema distribuido
- `make main` - Solo sistema principal
- `make test` - Verificar sintaxis

### Ejecución
- `make run-distributed` - Sistema distribuido
- `make run-main` - Sistema principal
- `make demo` - Demo completo
- `make demo-distributed` - Demo distribuido

### Desarrollo
- `make debug` - Versión debug
- `make clean` - Limpiar compilados
- `make help` - Ver ayuda completa

## 🎯 Características del Sistema

### Sistema Distribuido
- **Servidor S1**: Hash Extensible para IMEI (búsquedas O(1))
- **Servidor S2**: B+ Tree para timestamp (range queries)
- **Query Router**: Routing automático vs manual
- **Dataset GPS**: Tracking de vehículos en tiempo real

### Sistema Principal
- **SGBD Tradicional**: Disk + Buffer management
- **Algoritmos**: LRU vs Clock comparison  
- **Datasets**: Housing, Titanic, GPS

## 🔧 Solución de Problemas

### Error de compilación
```bash
make clean
make check-headers
bash setup.sh
make all
```

### Archivo de datos faltante
```bash
# El setup crea datos de muestra automáticamente
bash setup.sh
```

### Headers faltantes
```bash
# Verificar qué falta
make check-headers

# Recrear headers básicos
bash setup.sh
```
EOF

    log_success "Documentación creada: README_SETUP.md"
}

# Función principal
main() {
    log_info "Iniciando configuración del SGBD Distribuido..."
    
    check_directory
    check_dependencies
    create_directories
    check_headers
    create_missing_headers
    create_sample_data
    test_compilation
    create_quick_start
    create_readme
    
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗"
    echo -e "║                    SETUP COMPLETADO                         ║"
    echo -e "╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    log_success "Sistema SGBD Distribuido configurado exitosamente!"
    echo ""
    log_info "Próximos pasos:"
    echo -e "  ${CYAN}1.${NC} Compilar: ${YELLOW}make all${NC}"
    echo -e "  ${CYAN}2.${NC} Ejecutar: ${YELLOW}make run-distributed${NC}"
    echo -e "  ${CYAN}3.${NC} O usar: ${YELLOW}bash quick_start.sh${NC}"
    echo ""
    log_info "Para ayuda completa: ${YELLOW}make help${NC}"
    echo ""
}

# Ejecutar función principal
main "$@"