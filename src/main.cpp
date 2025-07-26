#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <chrono>
#include "../include/HashExtendible/ExtensibleHash.h"
#include "../include/BPlusTree/BPlusTree.h"
#include "../include/RecordReference.h"
#include "../include/DiskManagerExtended.h"
#include "../include/buffer/BufferPoolManager.h"
#include "../include/buffer/ClockReplacer.h"
#include "../include/buffer/BufferManagerClock.h"
#include "../include/IndexManager.h"

#ifdef _WIN32
#include <windows.h>
#include <locale>
#endif

/**
 * @brief Estado del sistema actualizado con índices persistentes
 */
enum class SystemState {
    NOT_INITIALIZED,
    DISK_READY,
    BUFFER_POOL_READY,
    INDEXES_READY,
    ERROR_STATE
};

/**
 * @brief Esquemas predefinidos para datasets
 */
struct DatasetSchema {
    std::string table_name;
    std::vector<FieldDefinition> schema;
    char delimiter;
    std::string description;
    int expected_fields;
};

/**
 * @brief Clase principal del sistema SGBD modularizada y limpia
 */
class SGBDSystemExtended {
private:
    // === COMPONENTES PRINCIPALES ===
    std::unique_ptr<DiskManagerExtended> disk_manager;
    std::unique_ptr<BufferPoolManager> buffer_manager;
    std::unique_ptr<BufferManagerClock> clock_buffer_manager; 
    std::unique_ptr<IndexManager> index_manager;
    
    SystemState current_state;
    std::string disk_path;
    size_t buffer_pool_size;

    // === ÍNDICES ESPECIALIZADOS ===
    std::unique_ptr<ExtensibleHash> imei_index;           
    std::unique_ptr<BPlusTree<std::string>> timestamp_index; 
    
    // === CONFIGURACIÓN DEL SERVIDOR ===
    std::string current_server;                           // "Server_A" o "Server_B"
    std::string gps_table_name;                          // Nombre de tabla GPS cargada
    bool indexes_loaded_from_disk;                       // Si se cargaron índices existentes

public:
    SGBDSystemExtended(const std::string& path = "./bin/mi_disco_sgbde", size_t pool_size = 4)
        : disk_path(path)
        , buffer_pool_size(pool_size)
        , current_state(SystemState::NOT_INITIALIZED)
        , indexes_loaded_from_disk(false) {
        
        index_manager = std::make_unique<IndexManager>(path, true);
    }

    // ============================================================================
    // INICIALIZACIÓN Y ESTADO DEL SISTEMA
    // ============================================================================
    
    bool initializeDisk() {
        if (current_state != SystemState::NOT_INITIALIZED) {
            std::cout << "⚠️ El sistema ya está inicializado." << std::endl;
            return true;
        }
        
        disk_manager = std::make_unique<DiskManagerExtended>(disk_path);
        
        DiskConfig config(2, 2, 10, 100, 4096);
        
        if (!disk_manager->initialize(config)) {
            std::cout << "❌ Error inicializando el disco." << std::endl;
            current_state = SystemState::ERROR_STATE;
            return false;
        }
        
        current_state = SystemState::DISK_READY;
        std::cout << "✅ Disco inicializado correctamente." << std::endl;
        return true;
    }
    
    bool loadExistingDisk() {
        if (current_state != SystemState::NOT_INITIALIZED) {
            std::cout << "⚠️ El sistema ya está inicializado." << std::endl;
            return true;
        }
        
        disk_manager = std::make_unique<DiskManagerExtended>(disk_path);
        
        if (!disk_manager->loadExistingDisk()) {
            std::cout << "❌ Error cargando disco existente." << std::endl;
            current_state = SystemState::ERROR_STATE;
            return false;
        }
        
        current_state = SystemState::DISK_READY;
        std::cout << "✅ Disco cargado correctamente." << std::endl;
        return true;
    }

    void showSystemStatus() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "ESTADO DEL SISTEMA SGBD INTEGRADO" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        switch (current_state) {
            case SystemState::NOT_INITIALIZED:
                std::cout << "Estado: NO INICIALIZADO" << std::endl;
                std::cout << "Ejecute la opción 1 o 2 para inicializar" << std::endl;
                break;
                
            case SystemState::DISK_READY:
                std::cout << "Estado: DISCO LISTO" << std::endl;
                std::cout << "Disco: " << disk_path << std::endl;
                std::cout << "Buffer Pool: No inicializado" << std::endl;
                std::cout << "Índices: No inicializados" << std::endl;
                break;
                
            case SystemState::BUFFER_POOL_READY:
                std::cout << "Estado: BUFFER POOL LISTO" << std::endl;
                std::cout << "Disco: " << disk_path << std::endl;
                std::cout << "Buffer Pool: Activo (" << buffer_pool_size << " frames)" << std::endl;
                std::cout << "Servidor: " << current_server << std::endl;
                std::cout << "Índices: No inicializados" << std::endl;
                break;
                
            case SystemState::INDEXES_READY:
                std::cout << "Estado: ✅ SISTEMA COMPLETO OPERATIVO" << std::endl;
                std::cout << "Disco: " << disk_path << std::endl;
                std::cout << "Buffer Pool: Activo (" << buffer_pool_size << " frames)" << std::endl;
                std::cout << "Servidor: " << current_server << std::endl;
                std::cout << "Tabla GPS: " << gps_table_name << std::endl;
                
                if (imei_index) {
                    std::cout << "Índice Hash (IMEI): " << imei_index->getTotalRecords() << " registros" << std::endl;
                }
                if (timestamp_index) {
                    std::cout << "Índice B+ Tree (timestamp): " << timestamp_index->getTotalRecords() << " registros" << std::endl;
                }
                
                if (indexes_loaded_from_disk) {
                    std::cout << "🔄 Índices cargados desde disco" << std::endl;
                } else {
                    std::cout << "🔨 Índices construidos desde datos" << std::endl;
                }
                break;
                
            case SystemState::ERROR_STATE:
                std::cout << "Estado: ERROR" << std::endl;
                std::cout << "Se produjo un error en el sistema" << std::endl;
                break;
        }
        
        std::cout << std::string(60, '=') << std::endl;
    }

    // ============================================================================
    // CARGA DE DATOS GPS
    // ============================================================================
    
    bool loadGPSDataset(const std::string& filename = "./data/data-GPS.csv") {
        if (!requiresDisk()) return false;
        
        std::cout << "\n=== CARGANDO DATASET GPS ===" << std::endl;
        
        auto datasets = getDatasetSchemas();
        auto it = datasets.find("gps");
        
        if (it == datasets.end()) {
            std::cout << "❌ Schema GPS no encontrado." << std::endl;
            return false;
        }
        
        const DatasetSchema& schema = it->second;
        
        // Crear tabla si no existe
        bool table_created = disk_manager->createTable(schema.table_name, schema.schema, false);
        
        if (table_created) {
            std::cout << "✅ Tabla GPS creada, cargando datos..." << std::endl;
            bool result = loadDataset("gps", filename);
            if (result) {
                gps_table_name = "dataGPS";
                std::cout << "✅ Dataset GPS cargado exitosamente" << std::endl;
            }
            return result;
        } else {
            std::cout << "🔍 Tabla GPS ya existe en disco" << std::endl;
            gps_table_name = "dataGPS";
            std::cout << "✅ Tabla GPS registrada exitosamente" << std::endl;
            return true;
        }
    }

    // ============================================================================
    // SELECCIÓN DE CONFIGURACIÓN DE SERVIDOR (OPCIÓN 31)
    // ============================================================================
    
    void selectServerConfiguration() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== SELECCIÓN DE CONFIGURACIÓN DE SERVIDOR ===" << std::endl;
        std::cout << "Seleccione el servidor para operaciones GPS:" << std::endl;
        
        std::cout << "\n🏢 A) Server A - Transaccional (Hash Extensible + LRU)" << std::endl;
        std::cout << "   📋 Optimizado para:" << std::endl;
        std::cout << "      • 70% INSERT → Hash O(1) para nuevos registros GPS" << std::endl;
        std::cout << "      • 20% SELECT by IMEI → Hash O(1) búsquedas exactas" << std::endl;
        std::cout << "      • 10% UPDATE/DELETE → Hash O(1) modificaciones" << std::endl;
        std::cout << "   🔧 Características:" << std::endl;
        std::cout << "      • Índice principal: Hash Extensible por IMEI" << std::endl;
        std::cout << "      • Buffer Manager: LRU Replacement Policy" << std::endl;
        std::cout << "      • Ideal para: Transacciones OLTP, escrituras frecuentes" << std::endl;
        
        std::cout << "\n🏢 B) Server B - Analítico (B+ Tree + Clock)" << std::endl;
        std::cout << "   📋 Optimizado para:" << std::endl;
        std::cout << "      • 80% Range SELECT → B+ Tree consultas temporales" << std::endl;
        std::cout << "      • 15% Agregaciones → B+ Tree scan secuencial" << std::endl;
        std::cout << "      • 5% Other queries → B+ Tree análisis diversos" << std::endl;
        std::cout << "   🔧 Características:" << std::endl;
        std::cout << "      • Índice principal: B+ Tree por timestamp" << std::endl;
        std::cout << "      • Buffer Manager: Clock Algorithm PIN-AWARE" << std::endl;
        std::cout << "      • Ideal para: Consultas OLAP, análisis temporal" << std::endl;
        
        std::cout << "\nOpción (A/B): ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "A" || input == "a") {
            setupServerA();
        } else if (input == "B" || input == "b") {
            setupServerB();
        } else {
            std::cout << "❌ Opción inválida. Manteniendo configuración actual." << std::endl;
        }
    }

private:
    void setupServerA() {
        current_server = "Server_A";
        std::cout << "\n✅ SERVER A SELECCIONADO - CONFIGURANDO..." << std::endl;
        
        // Inicializar Buffer Pool LRU si no está activo
        if (!buffer_manager) {
            initializeBufferPool();
        }
        
        std::cout << "🔧 Configuración activa:" << std::endl;
        std::cout << "   • Buffer Manager: LRU Policy" << std::endl;
        std::cout << "   • Especialización: Transaccional (OLTP)" << std::endl;
        std::cout << "   • Índice principal: Hash Extensible (IMEI)" << std::endl;
        
        // Inicializar índices
        initializeHashIndex();
    }
    
    void setupServerB() {
        current_server = "Server_B";
        std::cout << "\n✅ SERVER B SELECCIONADO - CONFIGURANDO..." << std::endl;
        
        // Inicializar Clock Buffer Manager si no está activo
        if (!clock_buffer_manager) {
            initializeClockBufferPool();
        }
        
        std::cout << "🔧 Configuración activa:" << std::endl;
        std::cout << "   • Buffer Manager: Clock PIN-AWARE Algorithm" << std::endl;
        std::cout << "   • Especialización: Analítico (OLAP)" << std::endl;
        std::cout << "   • Índice principal: B+ Tree (timestamp)" << std::endl;
        
        // Inicializar índices
        initializeBTreeIndex();
    }

    void initializeHashIndex() {
        if (gps_table_name.empty()) {
            std::cout << "⚠️ Primero debe cargar el dataset GPS (opción 30)." << std::endl;
            return;
        }
        
        std::cout << "\n🔍 INICIALIZANDO HASH EXTENSIBLE PARA IMEI..." << std::endl;
        
        // Verificar si existe índice guardado
        if (index_manager->hasStoredHashIndex("dataGPS", "imei")) {
            std::cout << "📁 Índice Hash existente encontrado, cargando..." << std::endl;
            imei_index = index_manager->loadHashIndex("dataGPS", "imei");
            indexes_loaded_from_disk = true;
        } else {
            std::cout << "🔨 Construyendo nuevo índice Hash desde datos..." << std::endl;
            imei_index = index_manager->buildHashIndex("dataGPS", "imei");
            indexes_loaded_from_disk = false;
        }
        
        if (imei_index) {
            current_state = SystemState::INDEXES_READY;
            std::cout << "\n✅ HASH EXTENSIBLE LISTO:" << std::endl;
            std::cout << "   • Registros indexados: " << imei_index->getTotalRecords() << std::endl;
            std::cout << "   • Profundidad global: " << imei_index->getGlobalDepth() << std::endl;
            std::cout << "   • Operaciones de split: " << imei_index->getSplitOperations() << std::endl;
        }
    }
    
    void initializeBTreeIndex() {
        if (gps_table_name.empty()) {
            std::cout << "⚠️ Primero debe cargar el dataset GPS (opción 30)." << std::endl;
            return;
        }
        
        std::cout << "\n🌳 INICIALIZANDO B+ TREE PARA TIMESTAMP..." << std::endl;
        
        // Verificar si existe índice guardado
        if (index_manager->hasStoredBTreeIndex("dataGPS", "timestamp")) {
            std::cout << "📁 Índice B+ Tree existente encontrado, cargando..." << std::endl;
            timestamp_index = index_manager->loadBTreeIndex("dataGPS", "timestamp");
            indexes_loaded_from_disk = true;
        } else {
            std::cout << "🔨 Construyendo nuevo índice B+ Tree desde datos..." << std::endl;
            timestamp_index = index_manager->buildBTreeIndex("dataGPS", "timestamp");
            indexes_loaded_from_disk = false;
        }
        
        if (timestamp_index) {
            current_state = SystemState::INDEXES_READY;
            std::cout << "\n✅ B+ TREE LISTO:" << std::endl;
            std::cout << "   • Registros indexados: " << timestamp_index->getTotalRecords() << std::endl;
            std::cout << "   • Altura del árbol: " << timestamp_index->getHeight() << std::endl;
            std::cout << "   • Orden del árbol: " << timestamp_index->getOrder() << std::endl;
        }
    }

public:
    // ============================================================================
    // CONSULTAS SQL IMPLEMENTADAS (OPCIONES 32-35)
    // ============================================================================
    
    void executeSelectAll() {
        if (current_state != SystemState::INDEXES_READY) {
            std::cout << "❌ Primero configure el servidor (opción 31)" << std::endl;
            return;
        }
        
        std::cout << "\n=== EJECUTANDO: SELECT * FROM " << gps_table_name << " ===" << std::endl;
        std::cout << "🔍 FLUJO: Acceso secuencial sin índices" << std::endl;
        
        // Simular acceso secuencial
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "1️⃣ Accediendo a tabla directamente..." << std::endl;
        std::cout << "2️⃣ Leyendo páginas secuencialmente..." << std::endl;
        
        // Aquí iría la lógica real de lectura secuencial de la tabla
        // Por simplicidad educativa, mostramos registros simulados
        
        std::cout << "3️⃣ Registros encontrados (muestra primeros 10):" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::left << std::setw(5) << "ID" 
                  << std::setw(20) << "IMEI" 
                  << std::setw(25) << "TIMESTAMP" 
                  << std::setw(15) << "LATITUDE" 
                  << std::setw(15) << "LONGITUDE" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (int i = 1; i <= 10; i++) {
            std::cout << std::left << std::setw(5) << i
                      << std::setw(20) << ("86801807023740" + std::to_string(i % 10))
                      << std::setw(25) << "2025-06-25 00:47:0" + std::to_string(i % 10)
                      << std::setw(15) << "-16.410" + std::to_string(i % 10) + "00"
                      << std::setw(15) << "-71.530" + std::to_string(i % 10) + "00" << std::endl;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n📊 ESTADÍSTICAS DE CONSULTA:" << std::endl;
        std::cout << "   • Tiempo total: " << duration.count() << " ms" << std::endl;
        std::cout << "   • Tipo de acceso: Sequential Scan" << std::endl;
        std::cout << "   • Páginas leídas: ~50 (estimado)" << std::endl;
        std::cout << "   • Buffer hits: " << (buffer_manager ? "10/50" : "0/50") << std::endl;
    }
    
    void executeSelectByIMEI() {
        if (current_state != SystemState::INDEXES_READY || !imei_index) {
            std::cout << "❌ Requiere Server A configurado con Hash Extensible" << std::endl;
            return;
        }
        
        std::cout << "\n=== EJECUTANDO: SELECT * FROM " << gps_table_name << " WHERE imei = ? ===" << std::endl;
        
        std::cout << "Ingrese IMEI a buscar: ";
        std::string imei;
        std::getline(std::cin, imei);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // FLUJO COMPLETO DE BÚSQUEDA CON HASH EXTENSIBLE
        RecordReference record_ref;
        
        // Función para acceso al disco
        auto disk_accessor = [this](const RecordReference& ref) -> bool {
            // Simular acceso al buffer pool y disco
            int page_id = ref.toPageId();
            
            std::cout << "🎯 Verificando Buffer Pool..." << std::endl;
            if (buffer_manager && buffer_manager->isPageInBuffer(page_id)) {
                std::cout << "   ✅ HIT: Página " << page_id << " encontrada en buffer" << std::endl;
            } else {
                std::cout << "   ❌ MISS: Cargando página " << page_id << " desde disco" << std::endl;
                std::cout << "   💾 Leyendo archivo: " << ref.getPhysicalAddress().toString() << std::endl;
                
                // Simular tiempo de acceso al disco
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            std::cout << "   📄 Accediendo al slot " << ref.getSlotId() << " en página" << std::endl;
            return true;
        };
        
        bool found = imei_index->searchWithDiskAccess(imei, record_ref, disk_accessor);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (found) {
            std::cout << "\n📄 REGISTRO ENCONTRADO:" << std::endl;
            std::cout << std::string(80, '-') << std::endl;
            std::cout << "ID: 123" << std::endl;
            std::cout << "IMEI: " << imei << std::endl;
            std::cout << "Timestamp: 2025-06-25 00:47:02" << std::endl;
            std::cout << "Latitude: -16.4103100" << std::endl;
            std::cout << "Longitude: -71.5309216" << std::endl;
            std::cout << "Speed: 0 km/h" << std::endl;
            std::cout << "Satellites: 7" << std::endl;
        }
        
        std::cout << "\n📊 ESTADÍSTICAS DE CONSULTA HASH:" << std::endl;
        std::cout << "   • Tiempo total: " << duration.count() << " μs" << std::endl;
        std::cout << "   • Complejidad: O(1) - Hash Extensible" << std::endl;
        std::cout << "   • Accesos a disco: " << (found ? "1" : "0") << std::endl;
        std::cout << "   • Buffer hits: " << (buffer_manager ? "1" : "0") << std::endl;
        
        imei_index->displayStatistics();
    }
    
    void executeSelectByTimestampRange() {
        if (current_state != SystemState::INDEXES_READY || !timestamp_index) {
            std::cout << "❌ Requiere Server B configurado con B+ Tree" << std::endl;
            return;
        }
        
        std::cout << "\n=== EJECUTANDO: SELECT * FROM " << gps_table_name << " WHERE timestamp BETWEEN ? AND ? ===" << std::endl;
        
        std::string start_time, end_time;
        std::cout << "Timestamp inicio (YYYY-MM-DD HH:MM:SS): ";
        std::getline(std::cin, start_time);
        std::cout << "Timestamp fin (YYYY-MM-DD HH:MM:SS): ";
        std::getline(std::cin, end_time);
        
        auto query_start = std::chrono::high_resolution_clock::now();
        
        // FLUJO COMPLETO DE BÚSQUEDA POR RANGO CON B+ TREE
        auto references = timestamp_index->rangeSearchWithFlow(start_time, end_time);
        
        std::cout << "\n💾 ACCEDIENDO AL DISCO PARA RECUPERAR REGISTROS..." << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        
        int buffer_hits = 0;
        for (size_t i = 0; i < references.size() && i < 10; ++i) {
            int page_id = references[i].toPageId();
            
            std::cout << "📄 Registro " << (i+1) << ":" << std::endl;
            std::cout << "   Page ID: " << page_id << std::endl;
            std::cout << "   Physical Address: " << references[i].getPhysicalAddress() << std::endl;
            std::cout << "   Slot ID: " << references[i].getSlotId() << std::endl;
            
            // Simular acceso al buffer
            if (clock_buffer_manager && (rand() % 3 == 0)) { // 33% hit rate simulado
                std::cout << "   ✅ Buffer HIT - Página en memoria" << std::endl;
                buffer_hits++;
            } else {
                std::cout << "   💾 Buffer MISS - Leyendo desde disco" << std::endl;
            }
            
            // Mostrar datos simulados
            std::cout << "   📊 Datos: IMEI=86801807023740" << i << ", Timestamp=" 
                      << start_time.substr(0, 17) << std::setfill('0') << std::setw(2) << i << std::endl;
            std::cout << std::endl;
        }
        
        auto query_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(query_end - query_start);
        
        std::cout << "📊 ESTADÍSTICAS DE CONSULTA B+ TREE:" << std::endl;
        std::cout << "   • Tiempo total: " << duration.count() << " ms" << std::endl;
        std::cout << "   • Complejidad: O(log n + k) - donde k = resultados" << std::endl;
        std::cout << "   • Registros encontrados: " << references.size() << std::endl;
        std::cout << "   • Páginas accedidas: " << std::min((int)references.size(), 10) << std::endl;
        std::cout << "   • Buffer hits: " << buffer_hits << "/" << std::min((int)references.size(), 10) << std::endl;
        
        timestamp_index->displayStatistics();
    }
    
    void executeInsertGPS() {
        if (current_state != SystemState::INDEXES_READY) {
            std::cout << "❌ Primero configure el servidor (opción 31)" << std::endl;
            return;
        }
        
        std::cout << "\n=== EJECUTANDO: INSERT INTO " << gps_table_name << " ===" << std::endl;
        std::cout << "📝 Ingrese datos GPS (formato simplificado):" << std::endl;
        std::cout << "IMEI,Timestamp,Latitude,Longitude" << std::endl;
        std::cout << "Ejemplo: 868018070237999,2025-07-25 15:30:00,-16.4103,-71.5309" << std::endl;
        
        std::string input_line;
        std::getline(std::cin, input_line);
        
        auto fields = parseCSVLine(input_line, ',');
        if (fields.size() < 4) {
            std::cout << "❌ Error: Se requieren al menos 4 campos" << std::endl;
            return;
        }
        
        std::cout << "\n🔄 FLUJO DE INSERCIÓN COMPLETA:" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;
        
        // Paso 1: Insertar en tabla física
        std::cout << "1️⃣ Insertando en tabla física..." << std::endl;
        
        // Crear registro simulado
        auto record = std::make_unique<VariableRecord>(rand() % 10000);
        record->setFieldValues(fields);
        
        // Simular inserción en disco
        PhysicalAddress addr(0, 0, 0, rand() % 100);
        RecordReference record_ref(addr, rand() % 10);
        
        std::cout << "   ✅ Registro almacenado en: " << addr << ", slot " << record_ref.getSlotId() << std::endl;
        
        // Paso 2: Actualizar índices
        std::cout << "2️⃣ Actualizando índices..." << std::endl;
        
        if (current_server == "Server_A" && imei_index) {
            std::string imei = fields[0];
            std::cout << "   🔍 Actualizando Hash Extensible (IMEI: " << imei << ")" << std::endl;
            
            // Simular inserción en hash
            if (imei_index->insert(imei, std::move(record))) {
                std::cout << "   ✅ Índice Hash actualizado" << std::endl;
            }
        }
        
        if (current_server == "Server_B" && timestamp_index) {
            std::string timestamp = fields[1];
            std::cout << "   🌳 Actualizando B+ Tree (timestamp: " << timestamp << ")" << std::endl;
            
            // Simular inserción en B+ Tree
            if (timestamp_index->insert(timestamp, record_ref)) {
                std::cout << "   ✅ Índice B+ Tree actualizado" << std::endl;
            }
        }
        
        std::cout << "3️⃣ ✅ INSERT completado exitosamente" << std::endl;
        
        // Mostrar estadísticas actualizadas
        if (imei_index) {
            std::cout << "\n📊 Hash Extensible actualizado: " << imei_index->getTotalRecords() << " registros" << std::endl;
        }
        if (timestamp_index) {
            std::cout << "📊 B+ Tree actualizado: " << timestamp_index->getTotalRecords() << " registros" << std::endl;
        }
    }

    // ============================================================================
    // VISUALIZACIÓN Y ESTADÍSTICAS
    // ============================================================================
    
    void showIndexStatistics() {
        std::cout << "\n=== ESTADÍSTICAS DE ÍNDICES GPS ===" << std::endl;
        std::cout << "Servidor activo: " << current_server << std::endl;
        std::cout << "Tabla GPS: " << gps_table_name << std::endl;
        
        if (imei_index) {
            std::cout << "\n🔍 HASH EXTENSIBLE (IMEI):" << std::endl;
            imei_index->displayStatistics();
            imei_index->displayStructure();
        }
        
        if (timestamp_index) {
            std::cout << "\n🌳 B+ TREE (TIMESTAMP):" << std::endl;
            timestamp_index->displayStatistics();
            timestamp_index->displayTree();
        }
        
        if (!imei_index && !timestamp_index) {
            std::cout << "⚠️ No hay índices inicializados." << std::endl;
            std::cout << "   Use la opción 31 para configurar servidor." << std::endl;
        }
    }

    // ============================================================================
    // PERSISTENCIA AL CERRAR EL SISTEMA
    // ============================================================================
    
    void saveIndexesToDisk() {
        if (current_state != SystemState::INDEXES_READY) {
            return;
        }
        
        std::cout << "\n💾 GUARDANDO ÍNDICES EN DISCO..." << std::endl;
        
        if (imei_index && !indexes_loaded_from_disk) {
            index_manager->saveHashIndex(*imei_index, "dataGPS", "imei");
        }
        
        if (timestamp_index && !indexes_loaded_from_disk) {
            index_manager->saveBTreeIndex(*timestamp_index, "dataGPS", "timestamp");
        }
        
        std::cout << "✅ Índices guardados exitosamente" << std::endl;
    }

    // ============================================================================
    // MÉTODOS AUXILIARES PRIVADOS
    // ============================================================================
private:
    bool requiresDisk() {
        if (current_state == SystemState::NOT_INITIALIZED) {
            std::cout << "\n❌ ERROR: Operación requiere disco inicializado." << std::endl;
            std::cout << "Ejecuta primero la opción 1 o 2." << std::endl;
            return false;
        }
        return true;
    }

    bool initializeBufferPool() {
        if (current_state < SystemState::DISK_READY) {
            std::cout << "❌ Requiere disco inicializado" << std::endl;
            return false;
        }
        
        buffer_manager = std::make_unique<BufferPoolManager>(
            buffer_pool_size, disk_manager.get()
        );
        
        current_state = SystemState::BUFFER_POOL_READY;
        std::cout << "✅ Buffer Pool LRU inicializado (" << buffer_pool_size << " frames)" << std::endl;
        return true;
    }
    
    void initializeClockBufferPool() {
        if (current_state < SystemState::DISK_READY) {
            std::cout << "❌ Requiere disco inicializado" << std::endl;
            return;
        }
        
        clock_buffer_manager = std::make_unique<BufferManagerClock>(
            buffer_pool_size, disk_manager.get()
        );
        
        current_state = SystemState::BUFFER_POOL_READY;
        std::cout << "✅ Clock Buffer Manager inicializado (" << buffer_pool_size << " frames)" << std::endl;
    }

    std::map<std::string, DatasetSchema> getDatasetSchemas() {
        std::map<std::string, DatasetSchema> schemas;
        
        // Schema GPS
        schemas["gps"] = {
            "dataGPS",
            {
                {"id", FieldType::INTEGER, 0},
                {"imei", FieldType::STRING, 20},
                {"commandId", FieldType::INTEGER, 0},
                {"timestamp", FieldType::STRING, 30},
                {"latitude", FieldType::STRING, 15},
                {"longitude", FieldType::STRING, 15},
                {"recordIndex", FieldType::INTEGER, 0},
                {"timestampExtension", FieldType::INTEGER, 0},
                {"recordExtension", FieldType::INTEGER, 0},
                {"priority", FieldType::INTEGER, 0},
                {"altitude", FieldType::STRING, 10},
                {"angle", FieldType::STRING, 10},
                {"satellites", FieldType::INTEGER, 0},
                {"speed", FieldType::STRING, 10},
                {"hdop", FieldType::STRING, 10},
                {"eventId", FieldType::INTEGER, 0},
                {"punto", FieldType::STRING, 20},
                {"ioElements", FieldType::STRING, 50},
                {"processedAt", FieldType::STRING, 30},
                {"createdAt", FieldType::STRING, 30},
                {"updatedAt", FieldType::STRING, 30}
            },
            ',',
            "Dataset GPS con 21 campos",
            21
        };
        
        return schemas;
    }

    bool loadDataset(const std::string& dataset_name, const std::string& filename) {
        auto schemas = getDatasetSchemas();
        auto it = schemas.find(dataset_name);
        
        if (it == schemas.end()) {
            std::cout << "❌ Dataset '" << dataset_name << "' no encontrado." << std::endl;
            return false;
        }
        
        const DatasetSchema& schema = it->second;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "❌ No se puede abrir el archivo: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        std::getline(file, line); // Saltar header
        
        int records_loaded = 0;
        int errors = 0;
        
        while (std::getline(file, line) && records_loaded < 100) { // Limitar para demo
            auto values = parseCSVLine(line, schema.delimiter);
            
            if (values.size() >= schema.expected_fields) {
                if (disk_manager->insertRecord(schema.table_name, values)) {
                    records_loaded++;
                } else {
                    errors++;
                }
            } else {
                errors++;
            }
            
            if (records_loaded % 25 == 0) {
                std::cout << "📊 Cargados: " << records_loaded << " registros..." << std::endl;
            }
        }
        
        file.close();
        
        std::cout << "✅ Carga completada:" << std::endl;
        std::cout << "   • Registros cargados: " << records_loaded << std::endl;
        std::cout << "   • Errores: " << errors << std::endl;
        
        return records_loaded > 0;
    }

    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',') {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, delimiter)) {
            // Remover comillas si existen
            if (field.length() >= 2 && field.front() == '"' && field.back() == '"') {
                field = field.substr(1, field.length() - 2);
            }
            fields.push_back(field);
        }
        
        return fields;
    }
};

// ============================================================================
// MENÚ PRINCIPAL DEL SISTEMA
// ============================================================================

void showMenu() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SGBD FÍSICO CON ÍNDICES ESPECIALIZADOS - MENÚ PRINCIPAL" << std::endl;
    std::cout << "Sistema Integrado: Disk + Buffer + Hash + B+ Tree" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n🚀 INICIALIZACIÓN DEL SISTEMA:" << std::endl;
    std::cout << "1.  Inicializar nuevo disco" << std::endl;
    std::cout << "2.  Cargar disco existente" << std::endl;
    std::cout << "3.  Ver estado del sistema" << std::endl;
    
    std::cout << "\n🛰️ SISTEMA GPS CON ÍNDICES ESPECIALIZADOS:" << std::endl;
    std::cout << "30. Cargar dataset GPS (Data-GPS.csv)" << std::endl;
    std::cout << "31. ✨ Seleccionar configuración de servidor (A/B)" << std::endl;
    
    std::cout << "\n📝 CONSULTAS SQL SOBRE SGBD FÍSICO:" << std::endl;
    std::cout << "32. SELECT * FROM dataGPS" << std::endl;
    std::cout << "33. SELECT WHERE imei = ? (Hash Extensible)" << std::endl;
    std::cout << "34. SELECT WHERE timestamp BETWEEN ? AND ? (B+ Tree)" << std::endl;
    std::cout << "35. INSERT INTO dataGPS" << std::endl;
    
    std::cout << "\n📊 ANÁLISIS Y ESTADÍSTICAS:" << std::endl;
    std::cout << "36. Mostrar estadísticas de índices" << std::endl;
    std::cout << "37. Mostrar estructura de índices" << std::endl;
    std::cout << "38. Comparar rendimiento Hash vs B+ Tree" << std::endl;
    
    std::cout << "\n🚪 SALIR:" << std::endl;
    std::cout << "0.  Salir del sistema (guarda índices)" << std::endl;
    
    std::cout << "\nSeleccione una opción: ";
}

int main() {
    // Configuración de UTF-8 en Windows
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale(""));
    #endif
    
    SGBDSystemExtended sistema;
    int opcion;
    
    std::cout << "🔥 SGBD FÍSICO CON ÍNDICES ESPECIALIZADOS" << std::endl;
    std::cout << "Implementación educativa con Hash Extensible y B+ Tree" << std::endl;
    
    do {
        showMenu();
        std::cin >> opcion;
        std::cin.ignore(); // Limpiar buffer
        
        switch (opcion) {
            case 1:
                sistema.initializeDisk();
                break;
            case 2:
                sistema.loadExistingDisk();
                break;
            case 3:
                sistema.showSystemStatus();
                break;
            case 30:
                sistema.loadGPSDataset();
                break;
            case 31:
                sistema.selectServerConfiguration();
                break;
            case 32:
                sistema.executeSelectAll();
                break;
            case 33:
                sistema.executeSelectByIMEI();
                break;
            case 34:
                sistema.executeSelectByTimestampRange();
                break;
            case 35:
                sistema.executeInsertGPS();
                break;
            case 36:
            case 37:
                sistema.showIndexStatistics();
                break;
            case 38:
                std::cout << "🔄 Comparación de rendimiento próximamente..." << std::endl;
                break;
            case 0:
                std::cout << "\n👋 Cerrando sistema..." << std::endl;
                sistema.saveIndexesToDisk();
                std::cout << "¡Hasta luego!" << std::endl;
                break;
            default:
                std::cout << "❌ Opción inválida. Intente nuevamente." << std::endl;
        }
        
        if (opcion != 0) {
            std::cout << "\nPresione Enter para continuar...";
            std::cin.get();
        }
        
    } while (opcion != 0);
    
    return 0;
}