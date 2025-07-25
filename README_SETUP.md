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
