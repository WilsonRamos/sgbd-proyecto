# 🔥 SGBD Físico con Índices Especializados

## Sistema de Gestión de Base de Datos Educativo con Hash Extensible y B+ Tree

### 📋 Descripción General

Este proyecto implementa un **Sistema de Gestión de Base de Datos (SGBD) físico educativo** que demuestra conceptos fundamentales de bases de datos con énfasis en **métodos de acceso especializados**. El sistema incluye:

- **Hash Extensible** para búsquedas exactas O(1)
- **B+ Tree** para búsquedas por rango O(log n + k)
- **Buffer Pool Manager** con políticas LRU y Clock
- **Persistencia de índices** automática
- **Flujo educativo** paso a paso

### 🎯 Características Principales

#### 🔍 Métodos de Acceso
- **Hash Extensible (IMEI)**
  - Directorio dinámico con profundidad global
  - Buckets con capacidad configurable
  - Splits automáticos con redistribución
  - Optimizado para `SELECT WHERE imei = 'valor'`

- **B+ Tree (Timestamp)**
  - Nodos internos y hojas separados
  - Enlaces horizontales entre hojas
  - Búsquedas por rango eficientes
  - Optimizado para `SELECT WHERE timestamp BETWEEN x AND y`

#### 💾 Gestión de Almacenamiento
- **DiskManager** con simulación de sectores físicos
- **Buffer Pool** con algoritmos LRU y Clock PIN-AWARE
- **Page Directory** persistente para mapeo físico
- **Record Management** con registros fijos y variables

#### 🔄 Persistencia
- **IndexManager** para guardar/cargar índices
- Metadatos de estructura en `metadata/`
- Reconstrucción automática desde datos originales
- Optimización de arranque del sistema

### 🏗️ Arquitectura del Sistema

```
SGBD Físico Integrado
├── 🏢 Servidor A (Transaccional)
│   ├── Hash Extensible (IMEI)
│   ├── Buffer Pool LRU
│   └── Optimizado para OLTP
│
├── 🏢 Servidor B (Analítico)
│   ├── B+ Tree (Timestamp)
│   ├── Buffer Pool Clock
│   └── Optimizado para OLAP
│
├── 💾 Capa de Almacenamiento
│   ├── DiskManager + FileSystemSimulator
│   ├── Page Directory persistente
│   └── Buffer Pool Manager
│
└── 📊 Capa de Datos
    ├── Dataset GPS (5000+ registros)
    ├── 21 campos por registro
    └── Formato CSV con parsing robusto
```

### 📁 Estructura de Archivos

```
proyecto/
├── src/
│   └── main.cpp                    # Programa principal integrado
├── include/
│   ├── IndexManager.h              # Gestor de persistencia
│   ├── RecordReference.h           # Referencias ligeras a registros
│   ├── HashExtendible/
│   │   ├── ExtensibleHash.h        # Hash Extensible principal
│   │   ├── Directory.h             # Directorio dinámico
│   │   ├── Bucket.h                # Buckets con capacidad
│   │   └── HashFunction.h          # Funciones hash optimizadas
│   ├── BPlusTree/
│   │   ├── BPlusTree.h             # B+ Tree principal
│   │   ├── BPlusNode.h             # Clase base para nodos
│   │   ├── LeafNode.h              # Nodos hoja con enlaces
│   │   ├── InternalNode.h          # Nodos internos de navegación
│   │   └── KeyComparator.h         # Comparador de claves
│   └── buffer/                     # Sistema de buffer existente
├── data/
│   └── data-GPS.csv               # Dataset GPS (generado automáticamente)
├── bin/
│   └── mi_disco_sgbde/            # Simulación de disco
│       ├── metadata/              # Índices persistentes
│       └── platter_*/             # Estructura física
├── Makefile                       # Sistema de build completo
└── README.md                      # Esta documentación
```

### 🚀 Instalación y Compilación

#### Prerrequisitos
```bash
# Compilador C++17
sudo apt-get install g++ make

# Para análisis de memoria (opcional)
sudo apt-get install valgrind
```

#### Compilación
```bash
# Verificar estructura
make check-headers
make check-data

# Compilar sistema completo
make all

# O compilar con debug
make debug

# O versión optimizada
make release
```

### 🎮 Uso del Sistema

#### Inicio Rápido
```bash
# Ejecutar sistema interactivo
make run

# O ejecutar demo educativo
make demo
```

#### Flujo de Trabajo Recomendado

1. **Inicializar Sistema** (Opción 1 o 2)
   ```
   1. Inicializar nuevo disco
   2. Cargar disco existente
   ```

2. **Cargar Dataset GPS** (Opción 30)
   ```
   30. Cargar dataset GPS (Data-GPS.csv)
   ```

3. **Configurar Servidor** (Opción 31) 🔥 **NUEVA FUNCIONALIDAD**
   ```
   31. Seleccionar configuración de servidor (A/B)
   ```
   - **Servidor A**: Hash Extensible + LRU (Transaccional)
   - **Servidor B**: B+ Tree + Clock (Analítico)

4. **Ejecutar Consultas** (Opciones 32-35)
   ```
   32. SELECT * FROM dataGPS
   33. SELECT WHERE imei = ? (Hash Extensible)
   34. SELECT WHERE timestamp BETWEEN ? AND ? (B+ Tree)
   35. INSERT INTO dataGPS
   ```

### 📝 Ejemplos de Consultas

#### Búsqueda por IMEI (Hash Extensible)
```sql
-- Sistema muestra flujo completo:
-- 1. Hash calculado
-- 2. Directory lookup
-- 3. Bucket search
-- 4. RecordReference obtenido
-- 5. Buffer Pool verificado
-- 6. Acceso a disco
-- 7. Registro recuperado

SELECT * FROM dataGPS WHERE imei = '868018070237402'
```

#### Búsqueda por Rango de Tiempo (B+ Tree)
```sql
-- Sistema muestra flujo completo:
-- 1. Navegación por el árbol
-- 2. Localización de nodo hoja
-- 3. Recorrido horizontal
-- 4. Múltiples RecordReference
-- 5. Accesos optimizados al disco
-- 6. Registros en rango

SELECT * FROM dataGPS 
WHERE timestamp BETWEEN '2025-06-25 00:47:00' AND '2025-06-25 01:00:00'
```

### 🔧 Configuración Avanzada

#### Servidores Especializados

**Servidor A - Transaccional**
- **Índice**: Hash Extensible por IMEI
- **Buffer**: LRU Replacement Policy
- **Uso típico**: 70% INSERT, 20% SELECT exacto, 10% UPDATE/DELETE
- **Complejidad**: O(1) para búsquedas exactas

**Servidor B - Analítico**
- **Índice**: B+ Tree por Timestamp
- **Buffer**: Clock Algorithm PIN-AWARE
- **Uso típico**: 80% Range SELECT, 15% Agregaciones, 5% Otras
- **Complejidad**: O(log n + k) para rangos

#### Parámetros de Índices

```cpp
// Hash Extensible
bucket_capacity = 4        // Registros por bucket
global_depth = dinámico    // Profundidad del directorio
split_threshold = 100%     // Divide cuando está lleno

// B+ Tree
tree_order = 3            // Orden del árbol
leaf_capacity = 2         // Claves por hoja
internal_capacity = 2     // Claves por nodo interno
```

### 💾 Persistencia de Índices

#### Archivos de Metadatos
```
bin/mi_disco_sgbde/metadata/
├── indicehash_imei_dataGPS.txt      # Hash Extensible
├── indicebtree_timestamp_dataGPS.txt # B+ Tree
└── page_directory.txt               # Mapeo físico
```

#### Flujo de Persistencia
1. **Al cerrar**: Índices se guardan automáticamente
2. **Al iniciar**: Sistema detecta índices existentes
3. **Opción**: Cargar índices o reconstruir desde datos
4. **Optimización**: Metadatos + reconstrucción inteligente

### 📊 Monitoreo y Estadísticas

#### Información de Índices
```bash
# Opción 36: Mostrar estadísticas de índices
- Registros indexados
- Operaciones realizadas
- Eficiencia de distribución
- Factores de carga
- Tiempo de construcción
```

### 🎓 Aspectos Educativos

#### Conceptos Demostrados

1. **Hash Extensible**
   - Directorio dinámico vs. hashing estático
   - Splits locales vs. globales
   - Distribución uniforme de claves
   - Complejidad O(1) amortizada

2. **B+ Tree**
   - Separación de índice y datos
   - Navegación por nodos internos
   - Recorrido secuencial en hojas
   - Búsquedas por rango eficientes

3. **Buffer Management**
   - Políticas de reemplazo (LRU vs Clock)
   - Pin counts para páginas activas
   - Dirty pages y write-back
   - Hit ratios y performance

4. **Persistencia**
   - Metadatos de estructura
   - Reconstrucción vs. serialización
   - Mapeo físico-lógico
   - Recovery de índices

#### Flujo Educativo Paso a Paso

Cada operación muestra:
- **Entrada**: Consulta SQL o comando
- **Proceso**: Pasos internos detallados
- **Estructuras**: Estado de índices y buffer
- **Salida**: Resultados y estadísticas
- **Análisis**: Complejidad y optimizaciones

### 🔬 Extensiones Futuras

#### Funcionalidades Planeadas
- [ ] Transacciones ACID básicas
- [ ] Write-Ahead Logging (WAL)
- [ ] Concurrencia con locks
- [ ] Compresión de índices
- [ ] Índices compuestos
- [ ] Query optimizer básico

#### Mejoras Técnicas
- [ ] Serialización binaria de índices
- [ ] Compactación automática de buckets
- [ ] B+ Tree con bulk loading
- [ ] Buffer pool adaptativo
- [ ] Métricas de performance en tiempo real

### 🐛 Troubleshooting

#### Problemas Comunes

**Error de compilación - Headers faltantes**
```bash
make check-headers  # Verificar archivos requeridos
```

**Error de ejecución - Dataset no encontrado**
```bash
make check-data     # Crear dataset de muestra
```

**Performance lenta - Buffer pool pequeño**
```cpp
// En main.cpp, aumentar buffer_pool_size
SGBDSystemExtended sistema("./bin/mi_disco_sgbde", 16); // 16 frames
```

**Índices no persisten**
```bash
# Verificar permisos de escritura
ls -la bin/mi_disco_sgbde/metadata/
```
