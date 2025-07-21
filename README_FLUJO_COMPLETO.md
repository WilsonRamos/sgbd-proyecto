# 🏢 SGBD Integrado con B+ Tree - Documentación Completa

## 📋 Resumen Ejecutivo

Este proyecto implementa un **Sistema de Gestión de Base de Datos (SGBD) completo e integrado** que utiliza **B+ Tree como método de acceso principal**. El sistema coordina eficientemente la interacción entre indexación, gestión de memoria y almacenamiento persistente.

## 🎯 Objetivos Cumplidos

### ✅ Flujo Completo de Consultas
- **SELECT**: Localización eficiente de registros individuales
- **INSERT**: Almacenamiento coordinado con actualización de índices
- **RANGE SELECT**: Consultas por rango optimizadas con lista enlazada de hojas

### ✅ Integración Modular
- **B+ Tree**: Indexación rápida con referencias a registros
- **BufferManager**: Gestión inteligente de memoria con algoritmo Clock PIN-AWARE
- **DiskManager**: Almacenamiento persistente con Page Directory integrado
- **QueryExecutor**: Coordinador central que orquesta todas las operaciones

### ✅ Soporte para Registros Diversos
- **Longitud Fija**: Registros con tamaño predeterminado (ej: empleados)
- **Longitud Variable**: Registros con campos de tamaño dinámico
- **Metadatos**: Offset tables, headers, timestamps

## 🏗️ Arquitectura del Sistema

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   USUARIO       │    │  QueryExecutor  │    │   B+ Tree       │
│                 │◄──►│                 │◄──►│                 │
│ SELECT/INSERT   │    │   Coordinador   │    │   Indexación    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                               │                        │
                               ▼                        ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ BufferManager   │    │  DiskManager    │    │ RecordReference │
│                 │◄──►│                 │    │                 │
│ Gestión Memoria │    │ Almacenamiento  │    │ Punteros Disco  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                       │
        ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│  Page Table     │    │ Page Directory  │
│  (Memoria)      │    │ (Disco)         │
└─────────────────┘    └─────────────────┘
```

## 🔄 Flujo Detallado de Operaciones

### 📖 SELECT (Consulta Individual)

```cpp
// 1. Usuario solicita registro
auto record = query_executor.selectRecord("12345678");

// 2. B+ Tree localiza referencia
RecordReference ref;
btree->search("12345678", ref);  // Retorna: PhysicalAddress + slot_id

// 3. BufferManager verifica memoria
auto block = buffer_manager->fetchPage(ref.toPageId());
// 3a. Si está en memoria: CACHE HIT
// 3b. Si no está: CACHE MISS -> cargar desde disco

// 4. DiskManager carga página (si es necesario)
block = disk_manager->readBlock(ref.getPhysicalAddress());

// 5. Extraer registro específico
auto record = block->getRecord(ref.getSlotId());

// 6. Liberar recursos
buffer_manager->unpinPage(page_id, false);
```

### 📝 INSERT (Inserción de Registro)

```cpp
// 1. Usuario inserta nuevo registro
auto record = std::make_unique<EmpleadoRecord>();
query_executor.insertRecord("12345678", std::move(record));

// 2. BufferManager obtiene página disponible
auto block = buffer_manager->newPage(page_id);

// 3. Añadir registro al bloque
int slot_id = block->addRecord(record);

// 4. Crear referencia
RecordReference ref(block->getAddress(), slot_id);

// 5. Insertar referencia en B+ Tree
btree->insert("12345678", ref);

// 6. Confirmar cambios (marcar dirty)
buffer_manager->unpinPage(page_id, true);
```

### 📊 RANGE SELECT (Consulta por Rango)

```cpp
// 1. Usuario solicita rango
auto records = query_executor.selectRange("10000000", "50000000");

// 2. B+ Tree retorna referencias en rango
auto refs = btree->rangeSearch("10000000", "50000000");

// 3. Cargar registros usando BufferManager
for (auto& ref : refs) {
    auto block = buffer_manager->fetchPage(ref.toPageId());
    auto record = block->getRecord(ref.getSlotId());
    results.push_back(record);
    buffer_manager->unpinPage(ref.toPageId(), false);
}
```

## 🧩 Componentes Principales

### 🌳 B+ Tree (`BPlusTree.h`)
- **Propósito**: Indexación rápida O(log n)
- **Almacena**: `RecordReference` (no registros completos)
- **Características**: Lista enlazada de hojas para rangos eficientes

```cpp
template<typename KeyType>
class BPlusTree {
    bool search(const KeyType& key, RecordReference& ref);
    std::vector<RecordReference> rangeSearch(start, end);
};
```

### 💾 BufferManager (`BufferManagerClock.h`)
- **Propósito**: Gestión de memoria con caché inteligente
- **Algoritmo**: Clock PIN-AWARE (evita evictar páginas en uso)
- **Características**: Page Table, estadísticas de hit/miss

```cpp
class BufferManagerClock {
    std::shared_ptr<Block> fetchPage(int page_id);
    bool unpinPage(int page_id, bool is_dirty);
};
```

### 💿 DiskManager (`DiskManagerExtended.h`)
- **Propósito**: Almacenamiento persistente con Page Directory
- **Características**: Mapeo automático de bloques a páginas
- **Integración**: Coordina con BufferManager para carga/descarga

```cpp
class DiskManagerExtended {
    bool readBlock(PhysicalAddress addr, Block& block);
    PageDirectory* getPageDirectory();
};
```

### 🔍 QueryExecutor (`QueryExecutor.h`)
- **Propósito**: Coordinador central de operaciones
- **Responsabilidades**: Orquestar flujo completo de consultas
- **Estadísticas**: Métricas de rendimiento del sistema

```cpp
class QueryExecutor {
    std::unique_ptr<Record> selectRecord(const std::string& key);
    bool insertRecord(const std::string& key, std::unique_ptr<Record> record);
    std::vector<std::unique_ptr<Record>> selectRange(start, end);
};
```

### 📄 RecordReference (`RecordReference.h`)
- **Propósito**: Referencia ligera a registros en disco
- **Contenido**: PhysicalAddress + slot_id + metadatos
- **Conversión**: Mapeo bidireccional con page_id del BufferManager

```cpp
class RecordReference {
    PhysicalAddress physical_address;
    int slot_id;
    int toPageId() const;  // Para BufferManager
    static RecordReference fromPageId(int page_id, int slot_id);
};
```

## 📊 Análisis de tu Almacenamiento

Basándome en la imagen que compartiste, tu sistema almacena:

```
# Header: Metadatos del sector con timestamp
# Block Header: Información estructural del bloque  
# Offset Table: Posiciones de cada registro
# Records: Datos serializados con tipo y estado

BLOCK_HEADER|P0_S0_T0_SEC0|4096|404|empleados_fijos|5
OFFSET_TABLE|64,132,200,268,336
RECORD|FIXED|1|0|P0_S0_T0_SEC0|1,Juan Perez,30,75000.50,2020-01-15
RECORD|FIXED|2|0|P0_S0_T0_SEC0|2,Maria Garcia,28,68000.00,2019-03-22
...
```

**Integración perfecta con nuestro sistema:**
- **Header**: Metadatos que `Block.h` ya maneja
- **Offset Table**: Compatible con `std::vector<size_t> offset_table`
- **Records**: Deserializables con `Record::deserialize()`
- **PhysicalAddress**: Mapeable a page_id para BufferManager

## 🚀 Ventajas de la Implementación

### ✅ Eficiencia de Memoria
- **Índice pequeño**: Solo referencias (8-16 bytes por entrada)
- **Cache-friendly**: Más claves del índice caben en memoria
- **Sin duplicación**: Registros solo almacenados en disco

### ✅ Escalabilidad
- **B+ Tree balanceado**: Altura logarítmica garantizada
- **Buffer inteligente**: Algoritmo Clock evita thrashing
- **Crecimiento dinámico**: Páginas asignadas según demanda

### ✅ Consistencia
- **ACID parcial**: Operaciones atómicas a nivel de página
- **Dirty bit tracking**: Cambios persistidos correctamente
- **Pin counting**: Evita corrupción por evicción prematura

### ✅ Flexibilidad
- **Múltiples tipos**: Soporte para diferentes esquemas de registro
- **Índices múltiples**: Varios B+ Trees sobre la misma tabla
- **Extensible**: Fácil agregar nuevos tipos de consulta

## 📈 Métricas de Rendimiento

El sistema incluye métricas comprehensivas:

```cpp
// QueryExecutor
- total_queries
- cache_hits / cache_misses  
- average_query_time

// B+ Tree
- search_operations
- insert_operations
- split_operations

// BufferManager
- page_faults
- evictions
- hit_ratio
```

## 🎯 Casos de Uso Educativos

### 1. **Demostración de Cache**
```bash
make demo  # Muestra hits/misses en tiempo real
```

### 2. **Análisis de B+ Tree**
```bash
# Visualiza estructura del árbol después de inserciones
btree->displayTree();
```

### 3. **Monitoreo de Buffer**
```bash
# Estado en tiempo real del buffer pool
buffer_manager->displayClockState();
```

## 🛠️ Compilación y Ejecución

```bash
# Compilar
make -f Makefile_integrado all

# Ejecutar demo completo
make -f Makefile_integrado demo

# Generar diagrama
make -f Makefile_integrado diagram

# Ver ayuda
make -f Makefile_integrado help
```

## 📋 Requisitos Técnicos

- **C++17** o superior
- **Headers incluidos**: Todas las dependencias en include/
- **Memoria**: ~100MB para demo completo
- **Disco**: ~50MB para archivos de simulación

## 🎓 Valor Educativo

Este proyecto demuestra:

1. **Integración real** de componentes de SGBD
2. **Coordinación** entre índices y almacenamiento  
3. **Gestión de memoria** realista con algoritmos de reemplazo
4. **Flujo completo** desde consulta hasta registro
5. **Métricas de rendimiento** como en SGBD reales

## 🔗 Archivos Clave

- `include/QueryExecutor.h` - Coordinador principal
- `include/RecordReference.h` - Referencias ligeras 
- `include/BPlusTree/BPlusTree.h` - Indexación
- `demo_flujo_completo.cpp` - Demostración integrada
- `diagrama_flujo_completo.puml` - Documentación visual

---

**✅ Este sistema implementa exitosamente el flujo completo de un SGBD real, desde la consulta del usuario hasta el acceso al registro en disco, coordinando eficientemente todos los componentes.**
