#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <chrono>
#include <thread>
#include <unordered_set>

// ✅ HEADERS CORREGIDOS
#include "../include/RecordReference.h"
#include "../include/HashExtendible/ExtensibleHash.h"
#include "../include/BPlusTree/BPlusTree.h"
#include "../include/DiskManager.h"
#include "../include/DiskManagerExtended.h"
#include "../include/buffer/BufferPoolManager.h"
#include "../include/buffer/BufferManagerClock.h"
#include "../include/IndexManager.h"

#ifdef _WIN32
#include <windows.h>
#include <locale>
#endif

/**
 * @brief Estado del sistema
 */
enum class SystemState {
    NOT_INITIALIZED,
    DISK_READY,
    BUFFER_POOL_READY,
    GPS_LOADED,
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
 * @brief SGBD FÍSICO EDUCATIVO COMPLETO - VERSIÓN FINAL CORREGIDA
 * 
 * ✅ TODAS LAS CORRECCIONES FINALES APLICADAS:
 * - Esquema GPS real integrado correctamente
 * - DiskConfig configurado apropiadamente
 * - parseCSVLine con parámetros correctos
 * - insertRecordFromValues compatible con DiskManager base
 * - displayExtendedSystemInfo sin override
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
    std::string current_server;
    std::string gps_table_name;
    bool indexes_loaded_from_disk;

    // === ESTADÍSTICAS GLOBALES ===
    size_t total_gps_records;
    std::chrono::steady_clock::time_point system_start_time;

public:
    /**
     * @brief Constructor
     */
    SGBDSystemExtended(const std::string& path, size_t pool_size = 8) 
        : current_state(SystemState::NOT_INITIALIZED)
        , disk_path(path)
        , buffer_pool_size(pool_size)
        , current_server("")
        , gps_table_name("")
        , indexes_loaded_from_disk(false)
        , total_gps_records(0)
    {
        system_start_time = std::chrono::steady_clock::now();
        
        try {
            disk_manager = std::make_unique<DiskManagerExtended>(path);
            
            std::filesystem::create_directories(path + "/metadata");
            index_manager = std::make_unique<IndexManager>(path, true, disk_manager.get());
            
            std::cout << "🚀 SGBD Físico Educativo Inicializado:" << std::endl;
            std::cout << "   📁 Ruta: " << path << std::endl;
            std::cout << "   💾 Buffer Pool: " << pool_size << " frames" << std::endl;
            std::cout << "   🔗 IndexManager: Conectado a DiskManager ✅" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error inicializando sistema: " << e.what() << std::endl;
            current_state = SystemState::ERROR_STATE;
        }
    }

    // ============================================================================
    // OPERACIONES BÁSICAS DEL SISTEMA
    // ============================================================================
    
    /**
     * @brief ✅ Inicializa nuevo disco - CORREGIDO con DiskConfig apropiado
     */
    bool initializeNewDisk() {
        try {
            std::cout << "\n🔧 INICIALIZANDO NUEVO DISCO..." << std::endl;
            
            // ✅ Crear configuración usando constructor o métodos públicos apropiados
            DiskConfig config(2,    // num_platters (número de platos)
                         2,    // surfaces_per_platter (superficies por plato, generalmente 2)
                         100,  // tracks_per_surface (pistas por superficie)
                         64,   // sectors_per_track (sectores por pista)
                         4096); // bytes_per_sector (bytes por sector)
            
            if (!disk_manager->initialize(config)) {
                std::cout << "❌ Error inicializando DiskManager" << std::endl;
                return false;
            }
            
            current_state = SystemState::DISK_READY;
            std::cout << "✅ Disco inicializado correctamente" << std::endl;
            
            return initializeBufferPool();
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Carga disco existente
     */
    bool loadExistingDisk() {
        try {
            std::cout << "\n📂 CARGANDO DISCO EXISTENTE..." << std::endl;
            
            if (!std::filesystem::exists(disk_path)) {
                std::cout << "❌ Ruta no existe: " << disk_path << std::endl;
                return false;
            }
            
            current_state = SystemState::DISK_READY;
            std::cout << "✅ Disco cargado correctamente" << std::endl;
            
            return initializeBufferPool();
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Inicializa Buffer Pool
     */
    bool initializeBufferPool() {
        try {
            std::cout << "\n💾 INICIALIZANDO BUFFER POOL..." << std::endl;
            
            buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
            clock_buffer_manager = std::make_unique<BufferManagerClock>(buffer_pool_size, disk_manager.get());
            
            current_state = SystemState::BUFFER_POOL_READY;
            std::cout << "✅ Buffer Pool inicializado (" << buffer_pool_size << " frames)" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error: " << e.what() << std::endl;
            return false;
        }
    }

    // ============================================================================
    // CARGA DE DATOS GPS CORREGIDA
    // ============================================================================
    
    /**
     * @brief ✅ Carga dataset GPS usando esquema real y DiskManager corregido
     */
    bool loadGPSDataset() {
        if (current_state < SystemState::BUFFER_POOL_READY) {
            std::cout << "❌ Sistema no inicializado correctamente" << std::endl;
            return false;
        }

        std::cout << "\n📡 CARGANDO DATASET GPS..." << std::endl;
        
        std::string csv_path = "./data/Data-GPS.csv";
        if (!std::filesystem::exists(csv_path)) {
            std::cout << "❌ Archivo no encontrado: " << csv_path << std::endl;
            return false;
        }

        try {
            // ✅ Obtener esquema GPS REAL
            auto schemas = getDatasetSchemas();
            auto gps_schema = schemas["gps"];
            
            // ✅ USAR DISKMANAGER PARA CREAR TABLA
            if (!disk_manager->createTable(gps_schema.table_name, gps_schema.schema)) {
                std::cout << "❌ Error creando tabla GPS" << std::endl;
                return false;
            }
            
            // Cargar datos del CSV al DiskManager
            std::ifstream file(csv_path);
            std::string line;
            std::getline(file, line); // Saltar header
            
            int records_loaded = 0;
            std::unordered_set<std::string> processed_imeis; // Prevenir duplicados
            
            while (std::getline(file, line) && records_loaded < 2000) {
                if (line.empty()) continue;
                
                // ✅ CORREGIDO: parseCSVLine con 2 parámetros
                auto values = parseCSVLine(line, ',');
                if (values.size() >= 21) {
                    std::string imei = values[1]; // IMEI en posición 1
                    
                    // Verificar duplicados
                    if (processed_imeis.find(imei) != processed_imeis.end()) {
                        continue;
                    }
                    processed_imeis.insert(imei);
                    
                    // ✅ INSERTAR VIA DISKMANAGER usando vector<string>
                    if (disk_manager->insertRecordFromValues(gps_schema.table_name, values)) {
                        records_loaded++;
                        
                        if (records_loaded % 500 == 0) {
                            std::cout << "📈 Cargados: " << records_loaded << " registros únicos" << std::endl;
                        }
                    }
                }
            }
            
            file.close();
            total_gps_records = records_loaded;
            gps_table_name = gps_schema.table_name;
            current_state = SystemState::GPS_LOADED;
            
            std::cout << "✅ GPS Dataset cargado:" << std::endl;
            std::cout << "   📊 Registros únicos: " << records_loaded << std::endl;
            std::cout << "   📋 Tabla: " << gps_table_name << std::endl;
            std::cout << "   🔍 IMEIs únicos: " << processed_imeis.size() << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error cargando GPS: " << e.what() << std::endl;
            return false;
        }
    }

    // ============================================================================
    // CONSTRUCCIÓN DE ÍNDICES
    // ============================================================================
    
    /**
     * @brief Inicializa índices usando DiskManager
     */
    bool initializeIndexes() {
        if (current_state < SystemState::GPS_LOADED) {
            std::cout << "❌ Datos GPS no cargados" << std::endl;
            return false;
        }
        
        if (current_server.empty()) {
            std::cout << "❌ Servidor no seleccionado" << std::endl;
            return false;
        }

        std::cout << "\n🔨 INICIALIZANDO ÍNDICES DESDE DISKMANAGER..." << std::endl;
        std::cout << "Servidor: " << current_server << std::endl;
        std::cout << "Tabla: " << gps_table_name << std::endl;

        try {
            if (current_server == "Server_A") {
                std::cout << "\n🔗 Construyendo Hash Extensible (IMEI)..." << std::endl;
                imei_index = index_manager->buildHashIndexFromDisk(gps_table_name, "imei", 1500);
                
                if (imei_index && imei_index->getTotalRecords() > 0) {
                    std::cout << "✅ Hash Extensible construido: " << imei_index->getTotalRecords() << " registros" << std::endl;
                } else {
                    std::cout << "⚠️ Hash Extensible vacío o error" << std::endl;
                }
                
            } else if (current_server == "Server_B") {
                std::cout << "\n🌲 Construyendo B+ Tree (Timestamp)..." << std::endl;
                timestamp_index = index_manager->buildBTreeIndexFromDisk(gps_table_name, "timestamp", 1500);
                
                if (timestamp_index && timestamp_index->size() > 0) {
                    std::cout << "✅ B+ Tree construido: " << timestamp_index->size() << " registros" << std::endl;
                } else {
                    std::cout << "⚠️ B+ Tree vacío o error" << std::endl;
                }
            }

            current_state = SystemState::INDEXES_READY;
            std::cout << "\n🎯 ÍNDICES LISTOS PARA CONSULTAS" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error construyendo índices: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Carga índices desde disco
     */
    bool loadIndexesFromDisk() {
        if (!index_manager) {
            std::cout << "❌ IndexManager no disponible" << std::endl;
            return false;
        }
        
        std::cout << "\n📂 CARGANDO ÍNDICES DESDE DISCO..." << std::endl;
        
        bool found_indexes = false;
        
        if (current_server == "Server_A") {
            imei_index = index_manager->loadHashIndex("imei_index");
            if (imei_index) {
                std::cout << "✅ Hash Extensible cargado desde disco" << std::endl;
                found_indexes = true;
            }
        } else if (current_server == "Server_B") {
            timestamp_index = index_manager->loadBTreeIndex("timestamp_index");
            if (timestamp_index) {
                std::cout << "✅ B+ Tree cargado desde disco" << std::endl;
                found_indexes = true;
            }
        }
        
        if (found_indexes) {
            current_state = SystemState::INDEXES_READY;
            indexes_loaded_from_disk = true;
        }
        
        return found_indexes;
    }

    /**
     * @brief Guarda índices en disco
     */
    void saveIndexes() {
        if (!index_manager) return;
        
        std::cout << "\n💾 GUARDANDO ÍNDICES..." << std::endl;
        
        if (imei_index) {
            index_manager->saveHashIndex(*imei_index, "imei_index");
        }
        
        if (timestamp_index) {
            index_manager->saveBTreeIndex(*timestamp_index, "timestamp_index");
        }
    }

    // ============================================================================
    // CONSULTAS SQL ESPECIALIZADAS
    // ============================================================================
    
    /**
     * @brief SELECT por IMEI usando Hash Extensible O(1)
     */
    void executeSelectByIMEI() {
        if (!imei_index) {
            std::cout << "❌ Hash Extensible no disponible" << std::endl;
            return;
        }

        std::cout << "\n🔍 CONSULTA POR IMEI (Hash Extensible O(1)):" << std::endl;
        std::cout << "Ingrese IMEI a buscar: ";
        std::string imei;
        std::getline(std::cin, imei);

        if (imei.empty()) {
            std::cout << "❌ IMEI vacío" << std::endl;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();
        
        RecordReference record_ref;
        bool found = imei_index->search(imei, record_ref);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (found) {
            std::cout << "✅ REGISTRO ENCONTRADO:" << std::endl;
            std::cout << "   IMEI: " << imei << std::endl;
            std::cout << "   RecordReference: " << record_ref.toString() << std::endl;
            
            if (disk_manager) {
                Block block(record_ref.getPhysicalAddress(), 4096);
                if (disk_manager->resolveRecordReference(record_ref, block)) {
                    auto records = block.getActiveRecords();
                    for (const auto& record : records) {
                        if (record->getId() == record_ref.getSlotId()) {
                            if (auto var_record = std::dynamic_pointer_cast<VariableRecord>(record)) {
                                auto values = var_record->getFieldValues();
                                if (values.size() >= 5) {
                                    std::cout << "   Timestamp: " << values[3] << std::endl;
                                    std::cout << "   Latitude: " << values[4] << std::endl;
                                    std::cout << "   Longitude: " << values[5] << std::endl;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        } else {
            std::cout << "❌ IMEI no encontrado: " << imei << std::endl;
        }

        std::cout << "⏱️ Tiempo: " << duration.count() << " microsegundos" << std::endl;
        std::cout << "🎯 Complejidad: O(1) - acceso directo por hash" << std::endl;
    }

    /**
     * @brief SELECT por rango de timestamp usando B+ Tree O(log n + k)
     */
    void executeSelectByTimestampRange() {
        if (!timestamp_index) {
            std::cout << "❌ B+ Tree no disponible" << std::endl;
            return;
        }

        std::cout << "\n📅 CONSULTA POR RANGO DE TIMESTAMP (B+ Tree O(log n + k)):" << std::endl;
        std::cout << "Formato: YYYY-MM-DD HH:MM:SS" << std::endl;
        
        std::cout << "Timestamp inicio: ";
        std::string start_time;
        std::getline(std::cin, start_time);
        
        std::cout << "Timestamp fin: ";
        std::string end_time;
        std::getline(std::cin, end_time);

        if (start_time.empty() || end_time.empty()) {
            std::cout << "❌ Timestamps vacíos" << std::endl;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto results = timestamp_index->rangeSearch(start_time, end_time);
        auto end_search = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_search - start);

        std::cout << "\n📊 RESULTADOS:" << std::endl;
        std::cout << "   Registros encontrados: " << results.size() << std::endl;
        std::cout << "   ⏱️ Tiempo: " << duration.count() << " microsegundos" << std::endl;
        std::cout << "   🎯 Complejidad: O(log n + k) donde k=" << results.size() << std::endl;

        if (!results.empty()) {
            std::cout << "\n📋 MUESTRA DE RESULTADOS:" << std::endl;
            size_t sample_size = std::min(static_cast<size_t>(5), results.size());
            
            for (size_t i = 0; i < sample_size; i++) {
                std::cout << "   [" << i << "] " << results[i].toString() << std::endl;
            }
            
            if (results.size() > sample_size) {
                std::cout << "   ... y " << (results.size() - sample_size) << " más" << std::endl;
            }
        }
    }

    // ============================================================================
    // INFORMACIÓN Y ESTADÍSTICAS
    // ============================================================================
    
    /**
     * @brief Muestra estadísticas de índices
     */
    void showIndexStatistics() {
        std::cout << "\n📊 ESTADÍSTICAS DE ÍNDICES:" << std::endl;
        std::cout << "=========================" << std::endl;
        
        if (current_server == "Server_A" && imei_index) {
            std::cout << "\n🔗 HASH EXTENSIBLE (IMEI):" << std::endl;
            imei_index->displayStatistics();
        } else if (current_server == "Server_B" && timestamp_index) {
            std::cout << "\n🌲 B+ TREE (TIMESTAMP):" << std::endl;
            timestamp_index->displayStatistics();
        } else {
            std::cout << "❌ No hay índices cargados para " << current_server << std::endl;
        }
        
        std::cout << "\n🏢 SISTEMA:" << std::endl;
        std::cout << "   Servidor activo: " << current_server << std::endl;
        std::cout << "   Estado: " << getStateString() << std::endl;
        std::cout << "   Registros GPS: " << total_gps_records << std::endl;
        
        if (buffer_manager) {
            std::cout << "\n💾 BUFFER POOL:" << std::endl;
            std::cout << buffer_manager->getStatistics() << std::endl;
        }
    }

    // ============================================================================
    // CONFIGURACIÓN DE SERVIDOR
    // ============================================================================
    
    /**
     * @brief Selecciona configuración de servidor
     */
    bool selectServerConfiguration() {
        std::cout << "\n🏢 CONFIGURACIÓN DE SERVIDOR:" << std::endl;
        std::cout << "=============================" << std::endl;
        std::cout << "A. Server A - Hash Extensible + LRU (Transaccional)" << std::endl;
        std::cout << "B. Server B - B+ Tree + Clock (Analítico)" << std::endl;
        std::cout << "Seleccione (A/B): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "A" || choice == "a") {
            current_server = "Server_A";
            std::cout << "✅ Server A seleccionado (Hash Extensible + LRU)" << std::endl;
            return true;
        } else if (choice == "B" || choice == "b") {
            current_server = "Server_B";
            std::cout << "✅ Server B seleccionado (B+ Tree + Clock)" << std::endl;
            return true;
        } else {
            std::cout << "❌ Selección inválida" << std::endl;
            return false;
        }
    }

    // ============================================================================
    // MENÚ PRINCIPAL
    // ============================================================================
    
    void runMainMenu() {
        while (true) {
            showMainMenu();
            
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "0") {
                saveIndexes();
                std::cout << "👋 Saliendo del sistema..." << std::endl;
                break;
            }
            
            executeMenuChoice(choice);
        }
    }

private:
    void showMainMenu() const {
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - system_start_time
        );
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "🔥 SGBD FÍSICO EDUCATIVO - ÍNDICES ESPECIALIZADOS" << std::endl;
        std::cout << "Estado: " << getStateString() << " | Servidor: " << current_server 
                  << " | Uptime: " << uptime.count() << "s" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "📁 INICIALIZACIÓN:" << std::endl;
        std::cout << " 1. Inicializar nuevo disco" << std::endl;
        std::cout << " 2. Cargar disco existente" << std::endl;
        std::cout << std::endl;
        
        std::cout << "📡 DATOS:" << std::endl;
        std::cout << "30. Cargar dataset GPS (Data-GPS.csv)" << std::endl;
        std::cout << "31. Seleccionar configuración servidor (A/B)" << std::endl;
        std::cout << std::endl;
        
        std::cout << "🔨 ÍNDICES:" << std::endl;
        std::cout << "32. Inicializar índices desde DiskManager" << std::endl;
        std::cout << "33. Cargar índices desde disco" << std::endl;
        std::cout << "34. Guardar índices en disco" << std::endl;
        std::cout << std::endl;
        
        std::cout << "🔍 CONSULTAS:" << std::endl;
        std::cout << "40. SELECT por IMEI (Hash O(1))" << std::endl;
        std::cout << "41. SELECT por rango timestamp (B+ Tree O(log n+k))" << std::endl;
        std::cout << std::endl;
        
        std::cout << "📊 INFORMACIÓN:" << std::endl;
        std::cout << "50. Estadísticas de índices" << std::endl;
        std::cout << "51. Estructura del sistema" << std::endl;
        std::cout << "52. Estado del disco" << std::endl;
        std::cout << std::endl;
        
        std::cout << " 0. Salir (auto-guarda índices)" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Opción: ";
    }
    
    void executeMenuChoice(const std::string& choice) {
        try {
            if (choice == "1") {
                initializeNewDisk();
            } else if (choice == "2") {
                loadExistingDisk();
            } else if (choice == "30") {
                loadGPSDataset();
            } else if (choice == "31") {
                selectServerConfiguration();
            } else if (choice == "32") {
                initializeIndexes();
            } else if (choice == "33") {
                loadIndexesFromDisk();
            } else if (choice == "34") {
                saveIndexes();
            } else if (choice == "40") {
                executeSelectByIMEI();
            } else if (choice == "41") {
                executeSelectByTimestampRange();
            } else if (choice == "50") {
                showIndexStatistics();
            } else if (choice == "51") {
                showSystemArchitecture();
            } else if (choice == "52") {
                showDiskStructure();
            } else if (!choice.empty()) {
                std::cout << "❌ Opción inválida: " << choice << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ Error ejecutando opción: " << e.what() << std::endl;
        }
        
        if (!choice.empty() && choice != "0") {
            std::cout << "\nPresione Enter para continuar...";
            std::cin.get();
        }
    }

    // ============================================================================
    // MÉTODOS AUXILIARES IMPLEMENTADOS
    // ============================================================================
    
    std::string getStateString() const {
        switch (current_state) {
            case SystemState::NOT_INITIALIZED: return "No inicializado";
            case SystemState::DISK_READY: return "Disco listo";
            case SystemState::BUFFER_POOL_READY: return "Buffer Pool listo";
            case SystemState::GPS_LOADED: return "GPS cargado";
            case SystemState::INDEXES_READY: return "Índices listos";
            case SystemState::ERROR_STATE: return "Error";
            default: return "Desconocido";
        }
    }

    /**
     * @brief ✅ ESQUEMA GPS REAL - CORREGIDO según tu formato
     */
    std::map<std::string, DatasetSchema> getDatasetSchemas() {
        std::map<std::string, DatasetSchema> datasets;
        
        // ✅ ESQUEMA GPS REAL que funciona
        DatasetSchema gps_schema;
        gps_schema.table_name = "dataGPS";
        gps_schema.delimiter = ',';
        gps_schema.description = "Dataset GPS con tracking de dispositivos";
        gps_schema.expected_fields = 21;
        
        // ✅ ESQUEMA CORRECTO con FieldType::INTEGER y FieldType::STRING
        gps_schema.schema = {
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
            {"speed", FieldType::INTEGER, 0},
            {"hdop", FieldType::STRING, 10},
            {"eventId", FieldType::INTEGER, 0},
            {"punto", FieldType::STRING, 50},
            {"ioElements", FieldType::STRING, 100},
            {"processedAt", FieldType::STRING, 30},
            {"createdAt", FieldType::STRING, 30},
            {"updatedAt", FieldType::STRING, 30}
        };
        
        datasets["gps"] = gps_schema;
        return datasets;
    }
    
    /**
     * @brief ✅ parseCSVLine CORREGIDO con 2 parámetros
     */
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;
        
        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];
            
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == delimiter && !in_quotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
        
        fields.push_back(field);
        return fields;
    }
    
    void showSystemArchitecture() {
        std::cout << "\n🏗️ ARQUITECTURA DEL SISTEMA:" << std::endl;
        std::cout << "============================" << std::endl;
        std::cout << "┌─────────────────────────────────────────────┐" << std::endl;
        std::cout << "│               APLICACIÓN                    │" << std::endl;
        std::cout << "├─────────────────┬───────────────────────────┤" << std::endl;
        std::cout << "│ Hash Extensible │       B+ Tree             │" << std::endl;
        std::cout << "│ (IMEI O(1))     │   (Timestamp O(log n+k))  │" << std::endl;
        std::cout << "├─────────────────┴───────────────────────────┤" << std::endl;
        std::cout << "│            IndexManager                     │" << std::endl;
        std::cout << "├─────────────────────────────────────────────┤" << std::endl;
        std::cout << "│            Buffer Pool Manager              │" << std::endl;
        std::cout << "├─────────────────────────────────────────────┤" << std::endl;
        std::cout << "│          DiskManagerExtended                │" << std::endl;
        std::cout << "└─────────────────────────────────────────────┘" << std::endl;
        
        std::cout << "\n🔧 COMPONENTES ACTIVOS:" << std::endl;
        std::cout << "   • DiskManager: " << (disk_manager ? "✅" : "❌") << std::endl;
        std::cout << "   • IndexManager: " << (index_manager ? "✅" : "❌") << std::endl;
        std::cout << "   • Hash Index: " << (imei_index ? "✅" : "❌") << std::endl;
        std::cout << "   • BTree Index: " << (timestamp_index ? "✅" : "❌") << std::endl;
    }
    
    /**
     * @brief ✅ showDiskStructure CORREGIDO sin override
     */
    void showDiskStructure() {
        if (disk_manager) {
            disk_manager->displayExtendedSystemInfo(); // ✅ Método correcto sin override
        } else {
            std::cout << "❌ DiskManager no inicializado" << std::endl;
        }
    }
};

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

int main() {
    try {
        #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8); 
        //std::locale::global(std::locale(""));
        #endif

        std::cout << "🔥 SGBD FÍSICO EDUCATIVO - ÍNDICES ESPECIALIZADOS" << std::endl;
        std::cout << "=================================================" << std::endl;
        std::cout << "✅ Hash Extensible (IMEI) - O(1)" << std::endl;
        std::cout << "✅ B+ Tree (Timestamp) - O(log n + k)" << std::endl;
        std::cout << "✅ Buffer Pool + Page Directory" << std::endl;
        std::cout << "✅ RecordReference Bridge" << std::endl;
        std::cout << "✅ DiskManager Integration" << std::endl;
        std::cout << std::endl;

        SGBDSystemExtended sistema("./bin/mi_disco_sgbde", 16);
        sistema.runMainMenu();
        
        std::cout << "\n🎯 Sistema finalizado correctamente" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "\n❌ Error fatal: " << e.what() << std::endl;
        return 1;
    }
}