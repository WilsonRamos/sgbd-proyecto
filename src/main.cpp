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
    
    // ✅ ORDEN CORREGIDO - variables miembro en orden de inicialización
    SystemState current_state;
    size_t buffer_pool_size;
    std::string disk_path;

    // === ÍNDICES ESPECIALIZADOS ===
    std::unique_ptr<ExtensibleHash> imei_index;           
    std::unique_ptr<BPlusTree<std::string>> timestamp_index; 
    
    // === CONFIGURACIÓN DEL SERVIDOR ===
    std::string current_server;                           // "Server_A" o "Server_B"
    std::string gps_table_name;                          // Nombre de tabla GPS cargada
    bool indexes_loaded_from_disk;                       // Si se cargaron índices existentes

public:
    // ✅ CONSTRUCTOR CORREGIDO - Lista de inicialización en orden correcto
    SGBDSystemExtended(const std::string& path = "./bin/mi_disco_sgbde", size_t pool_size = 4)
        : current_state(SystemState::NOT_INITIALIZED)    // Primero
        , buffer_pool_size(pool_size)                     // Segundo  
        , disk_path(path)                                 // Tercero
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
        std::cout << "✅ Sistema de disco inicializado correctamente." << std::endl;
        return true;
    }
    
    bool initializeBufferPool() {
        if (current_state != SystemState::DISK_READY) {
            std::cout << "❌ Debe inicializar el disco primero." << std::endl;
            return false;
        }
        
        buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
        clock_buffer_manager = std::make_unique<BufferManagerClock>(buffer_pool_size, disk_manager.get());
        
        current_state = SystemState::BUFFER_POOL_READY;
        std::cout << "✅ Buffer Pool inicializado correctamente." << std::endl;
        return true;
    }
    
    bool initializeIndexes() {
        if (current_state != SystemState::BUFFER_POOL_READY) {
            std::cout << "❌ Debe inicializar el buffer pool primero." << std::endl;
            return false;
        }
        
        std::cout << "\n🔍 INICIALIZANDO ÍNDICES ESPECIALIZADOS..." << std::endl;
        
        // Intentar cargar índices existentes
        imei_index = index_manager->loadHashIndex("imei_index");
        timestamp_index = index_manager->loadBTreeIndex("timestamp_index");
        
        if (!imei_index) {
            std::cout << "📝 Creando nuevo Hash Extensible para IMEI..." << std::endl;
            imei_index = std::make_unique<ExtensibleHash>(4);
        } else {
            indexes_loaded_from_disk = true;
            std::cout << "✅ Hash Extensible cargado desde disco" << std::endl;
        }
        
        if (!timestamp_index) {
            std::cout << "📝 Creando nuevo B+ Tree para Timestamp..." << std::endl;
            timestamp_index = std::make_unique<BPlusTree<std::string>>(4);
        } else {
            indexes_loaded_from_disk = true;
            std::cout << "✅ B+ Tree cargado desde disco" << std::endl;
        }
        
        current_server = "Server_A"; // Servidor por defecto
        current_state = SystemState::INDEXES_READY;
        
        std::cout << "✅ Sistema de índices inicializado correctamente." << std::endl;
        std::cout << "📊 Índices cargados desde disco: " << (indexes_loaded_from_disk ? "SÍ" : "NO") << std::endl;
        
        return true;
    }
    
    // ============================================================================
    // VERIFICACIÓN DE REQUISITOS
    // ============================================================================
    
    bool requiresDisk() {
        if (current_state == SystemState::NOT_INITIALIZED) {
            std::cout << "❌ Error: Sistema no inicializado. Use 'Inicializar Disco' primero." << std::endl;
            return false;
        }
        return true;
    }
    
    bool requiresBufferPool() {
        if (current_state < SystemState::BUFFER_POOL_READY) {
            std::cout << "❌ Error: Buffer Pool no inicializado. Use 'Inicializar Buffer Pool' primero." << std::endl;
            return false;
        }
        return true;
    }
    
    bool requiresIndexes() {
        if (current_state < SystemState::INDEXES_READY) {
            std::cout << "❌ Error: Índices no inicializados. Use 'Inicializar Índices' primero." << std::endl;
            return false;
        }
        return true;
    }
    
    // ============================================================================
    // OPERACIONES CON ÍNDICES - ✅ TODAS LAS FUNCIONES CORREGIDAS
    // ============================================================================
    
    void searchByIMEI() {
        if (!requiresIndexes()) return;
        
        std::cout << "\n=== BÚSQUEDA POR IMEI (Hash Extensible) ===" << std::endl;
        std::cout << "Servidor actual: " << current_server << std::endl;
        
        std::string imei;
        std::cout << "Ingrese IMEI a buscar: ";
        std::getline(std::cin, imei);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // ✅ VERIFICAR SI PÁGINA ESTÁ EN BUFFER
        if (buffer_manager && buffer_manager->isPageInBuffer(1)) {
            std::cout << "📋 Página en buffer pool para optimización" << std::endl;
        }
        
        // ✅ USAR VariableRecord en lugar de Record abstracto
        RecordReference record_ref;
        bool found = imei_index->searchReference(imei, record_ref);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (found) {
            std::cout << "✅ Registro encontrado:" << std::endl;
            std::cout << "   IMEI: " << imei << std::endl;
            std::cout << "   RecordReference: " << record_ref.toString() << std::endl;
        } else {
            std::cout << "❌ Registro no encontrado para IMEI: " << imei << std::endl;
        }
        
        std::cout << "⏱️ Tiempo de búsqueda: " << duration.count() << " microsegundos" << std::endl;
        std::cout << "📊 Estadísticas Hash: " << imei_index->getStatistics() << std::endl;
    }
    
    void searchByTimestampRange() {
        if (!requiresIndexes()) return;
        
        std::cout << "\n=== BÚSQUEDA POR RANGO DE TIMESTAMP (B+ Tree) ===" << std::endl;
        std::cout << "Servidor actual: " << current_server << std::endl;
        
        std::string start_time, end_time;
        std::cout << "Timestamp inicio (YYYY-MM-DD HH:MM:SS): ";
        std::getline(std::cin, start_time);
        std::cout << "Timestamp fin (YYYY-MM-DD HH:MM:SS): ";
        std::getline(std::cin, end_time);
        
        auto search_start = std::chrono::high_resolution_clock::now();
        
        auto results = timestamp_index->rangeSearch(start_time, end_time);
        
        auto search_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(search_end - search_start);
        
        std::cout << "✅ Búsqueda completada:" << std::endl;
        std::cout << "   Registros encontrados: " << results.size() << std::endl;
        std::cout << "   Rango: " << start_time << " - " << end_time << std::endl;
        
        if (!results.empty() && results.size() <= 10) {
            std::cout << "\n📋 Primeros resultados:" << std::endl;
            for (size_t i = 0; i < std::min(results.size(), size_t(10)); i++) {
                std::cout << "   [" << i+1 << "] " << results[i] << std::endl;
            }
        }
        
        std::cout << "⏱️ Tiempo de búsqueda: " << duration.count() << " microsegundos" << std::endl;
        std::cout << "📊 Estadísticas B+ Tree: " << timestamp_index->getStatistics() << std::endl;
    }
    
    void insertGPSRecord() {
        if (!requiresIndexes()) return;
        
        std::cout << "\n=== INSERTAR REGISTRO GPS ===" << std::endl;
        
        std::string imei, timestamp, latitude, longitude;
        std::cout << "IMEI: ";
        std::getline(std::cin, imei);
        std::cout << "Timestamp (YYYY-MM-DD HH:MM:SS): ";
        std::getline(std::cin, timestamp);
        std::cout << "Latitud: ";
        std::getline(std::cin, latitude);
        std::cout << "Longitud: ";
        std::getline(std::cin, longitude);
        
        // Crear registro GPS
        std::vector<std::string> gps_data = {
            "AUTO", imei, "68", timestamp, latitude, longitude,
            "0", "0", "0", "0", "0", "0", "0", "0", "0", "7",
            "POINT", "{}", timestamp, timestamp, timestamp
        };
        
        // ✅ CREAR VariableRecord CON INTERFAZ CORRECTA
        auto record = std::make_unique<VariableRecord>();
        record->setFieldValues(gps_data);
        
        auto record_copy = record->clone(); // Para el segundo índice
        
        // ✅ INSERTAR EN HASH EXTENSIBLE (usa unique_ptr<Record>)
        bool hash_inserted = imei_index->insert(imei, std::move(record));
        
        // ✅ CREAR RecordReference para B+ Tree
        PhysicalAddress addr(0, 0, 0, rand() % 1000);
        RecordReference record_ref(addr, rand() % 10);
        bool btree_inserted = timestamp_index->insert(timestamp, record_ref);
        
        if (hash_inserted && btree_inserted) {
            std::cout << "✅ Registro GPS insertado exitosamente en ambos índices" << std::endl;
            
            // Insertar en disco también
            if (disk_manager->insertRecord("gps_data", gps_data)) {
                std::cout << "✅ Registro también guardado en disco" << std::endl;
            }
        } else {
            std::cout << "❌ Error insertando registro en índices" << std::endl;
        }
    }
    
    // ============================================================================
    // CARGA MASIVA DE DATOS - ✅ CORREGIDA
    // ============================================================================
    
    void loadGPSDataset() {
        if (!requiresIndexes()) return;
        
        std::cout << "\n=== CARGA MASIVA DEL DATASET GPS ===" << std::endl;
        
        std::string csv_file = "data/data-GPS.csv";
        
        if (!std::filesystem::exists(csv_file)) {
            std::cout << "❌ Archivo no encontrado: " << csv_file << std::endl;
            return;
        }
        
        std::ifstream file(csv_file);
        if (!file.is_open()) {
            std::cout << "❌ Error abriendo archivo: " << csv_file << std::endl;
            return;
        }
        
        std::string line;
        std::getline(file, line); // Saltar header
        
        int loaded_count = 0;
        int hash_inserts = 0;
        int btree_inserts = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        while (std::getline(file, line) && !line.empty()) {
            auto values = parseCSVLine(line);
            
            if (values.size() >= 21) {
                std::string imei = values[1];      // Campo IMEI
                std::string timestamp = values[3]; // Campo timestamp
                
                // ✅ CREAR VariableRecord CORRECTAMENTE
                auto record = std::make_unique<VariableRecord>();
                record->setFieldValues(values);
                
                // Insertar en Hash (IMEI)
                if (imei_index->insert(imei, std::move(record))) {
                    hash_inserts++;
                }
                
                // ✅ CREAR RecordReference para B+ Tree
                PhysicalAddress addr(0, 0, 0, loaded_count);
                RecordReference record_ref(addr, loaded_count % 10);
                
                if (timestamp_index->insert(timestamp, record_ref)) {
                    btree_inserts++;
                }
                
                loaded_count++;
                
                if (loaded_count % 100 == 0) {
                    std::cout << "📈 Procesados: " << loaded_count << " registros..." << std::endl;
                }
            }
        }
        
        file.close();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n✅ CARGA COMPLETADA:" << std::endl;
        std::cout << "   📊 Registros procesados: " << loaded_count << std::endl;
        std::cout << "   📊 Insertados en Hash (IMEI): " << hash_inserts << std::endl;
        std::cout << "   📊 Insertados en B+ Tree (Timestamp): " << btree_inserts << std::endl;
        std::cout << "   ⏱️ Tiempo total: " << duration.count() << " ms" << std::endl;
        
        // Guardar índices en disco
        index_manager->saveHashIndex(*imei_index, "gps_data", "imei");
        index_manager->saveBTreeIndex(*timestamp_index, "gps_data", "timestamp");
        
        gps_table_name = "gps_data";
        std::cout << "💾 Índices guardados en disco" << std::endl;
    }
    
    // ============================================================================
    // MÉTODOS AUXILIARES - ✅ CORREGIDOS
    // ============================================================================
    
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',') {
        std::vector<std::string> result;
        std::string current_field;
        bool in_quotes = false;
        
        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];
            
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == delimiter && !in_quotes) {
                result.push_back(current_field);
                current_field.clear();
            } else {
                current_field += c;
            }
        }
        
        result.push_back(current_field);
        return result;
    }
    
    void showSystemStatus() {
        std::cout << "\n📊 ESTADO DEL SISTEMA SGBD" << std::endl;
        std::cout << "=============================" << std::endl;
        
        std::string state_str;
        switch (current_state) {
            case SystemState::NOT_INITIALIZED: state_str = "❌ NO INICIALIZADO"; break;
            case SystemState::DISK_READY: state_str = "🟡 DISCO LISTO"; break;
            case SystemState::BUFFER_POOL_READY: state_str = "🟠 BUFFER POOL LISTO"; break;
            case SystemState::INDEXES_READY: state_str = "✅ ÍNDICES LISTOS"; break;
            case SystemState::ERROR_STATE: state_str = "❌ ERROR"; break;
        }
        
        std::cout << "Estado: " << state_str << std::endl;
        std::cout << "Servidor actual: " << current_server << std::endl;
        std::cout << "Buffer Pool Size: " << buffer_pool_size << " frames" << std::endl;
        std::cout << "Disk Path: " << disk_path << std::endl;
        
        if (buffer_manager) {
            std::cout << "\n" << buffer_manager->getStatistics() << std::endl;
        }
        
        if (imei_index) {
            std::cout << "\n📈 Hash Extensible (IMEI):\n" << imei_index->getStatistics() << std::endl;
        }
        
        if (timestamp_index) {
            std::cout << "\n🌳 B+ Tree (Timestamp):\n" << timestamp_index->getStatistics() << std::endl;
        }
        
        std::cout << "\n" << index_manager->getIndexInfo() << std::endl;
    }
    
    void runMenu() {
        std::cout << "🎮 Entrando al bucle del menú..." << std::endl;
        std::cout.flush();
        
        while (true) {
            std::cout << "\n🔥 SGBD CON ÍNDICES ESPECIALIZADOS" << std::endl;
            std::cout << "===================================" << std::endl;
            std::cout << "1️⃣  Inicializar Disco" << std::endl;
            std::cout << "2️⃣  Inicializar Buffer Pool" << std::endl;
            std::cout << "3️⃣  Inicializar Índices" << std::endl;
            std::cout << "4️⃣  Cargar Dataset GPS" << std::endl;
            std::cout << "5️⃣  Buscar por IMEI (Hash)" << std::endl;
            std::cout << "6️⃣  Buscar por Rango Timestamp (B+ Tree)" << std::endl;
            std::cout << "7️⃣  Insertar Registro GPS" << std::endl;
            std::cout << "8️⃣  Ver Estado del Sistema" << std::endl;
            std::cout << "9️⃣  Cambiar Servidor (A/B)" << std::endl;
            std::cout << "0️⃣  Salir" << std::endl;
            std::cout << "\nSeleccione opción: ";
            std::cout.flush(); // ✅ FORZAR FLUSH ANTES DE LEER ENTRADA
            
            std::string option;
            if (!std::getline(std::cin, option)) {
                std::cout << "\n❌ Error leyendo entrada. Saliendo..." << std::endl;
                break;
            }
            
            std::cout << "🔍 Opción seleccionada: [" << option << "]" << std::endl;
            std::cout.flush();
            
            if (option == "1") {
                initializeDisk();
            } else if (option == "2") {
                initializeBufferPool();
            } else if (option == "3") {
                initializeIndexes();
            } else if (option == "4") {
                loadGPSDataset();
            } else if (option == "5") {
                searchByIMEI();
            } else if (option == "6") {
                searchByTimestampRange();
            } else if (option == "7") {
                insertGPSRecord();
            } else if (option == "8") {
                showSystemStatus();
            } else if (option == "9") {
                current_server = (current_server == "Server_A") ? "Server_B" : "Server_A";
                std::cout << "🔄 Cambiado a " << current_server << std::endl;
            } else if (option == "0") {
                std::cout << "👋 Saliendo del sistema..." << std::endl;
                break;
            } else {
                std::cout << "❌ Opción inválida: [" << option << "]. Intente nuevamente." << std::endl;
            }
            
            std::cout.flush();
        }
    }
};

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

int main() {
#ifdef _WIN32
    // Configurar codificación para Windows
    SetConsoleOutputCP(CP_UTF8);
    std::setlocale(LC_ALL, "");
#endif
    
    std::cout << "🚀 Iniciando SGBD con Índices Especializados..." << std::endl;
    std::cout.flush(); // ✅ FORZAR FLUSH EN WINDOWS
    
    try {
        std::cout << "🔧 Creando sistema..." << std::endl;
        std::cout.flush();
        
        SGBDSystemExtended system("./bin/mi_disco_sgbde", 8);
        
        std::cout << "✅ Sistema creado exitosamente" << std::endl;
        std::cout << "🎮 Iniciando menú principal..." << std::endl;
        std::cout.flush();
        
        system.runMenu();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error fatal: " << e.what() << std::endl;
        std::cout.flush();
        return 1;
    }
    
    std::cout << "✅ Sistema cerrado correctamente." << std::endl;
    return 0;
}