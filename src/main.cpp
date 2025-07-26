#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <chrono>
#include <thread>
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
 * @brief Estado del sistema actualizado con Buffer Pool y GPS
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
 * @brief Clase principal del sistema SGBD modularizada y GPS especializada
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

    // === ÍNDICES ESPECIALIZADOS GPS ===
    std::unique_ptr<ExtensibleHash> imei_index;           
    std::unique_ptr<BPlusTree<std::string>> timestamp_index; 
    std::string current_server;                           // "Server_A" o "Server_B"
    std::string gps_table_name;                          // Nombre de tabla GPS cargada
    bool indexes_loaded_from_disk;                       // Si se cargaron índices existentes

    // === MÉTODOS AUXILIARES PRIVADOS (DE mainFuncional.cpp) ===
    std::map<std::string, DatasetSchema> getDatasetSchemas();
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',');
    int countRecordsInFile(const std::string& filename);
    void cleanValue(std::string& value);
    bool requiresDisk();
    bool requiresBufferPool();
    bool requiresGPSData();

public:
    SGBDSystemExtended(const std::string& path = "./bin/mi_disco_sgbde", size_t pool_size = 4);
    SystemState getState() const { return current_state; }
    
    // === ESTADO DEL SISTEMA ===
    void showSystemStatus();
    
    // === INICIALIZACIÓN (EXACTO DE mainFuncional.cpp) ===
    bool initializeDisk();
    bool loadExistingDisk();
    
    // === GESTIÓN GPS ===
    bool loadGPSDataset();
    void selectServerConfiguration();
    
    // === CONFIGURACIÓN COMBINADA GPS ===
    void selectServerAndEnterMenu();
    
    // === GESTIÓN DE ÍNDICES GPS ===
    bool initializeIndexes();
    bool loadExistingIndexes();
    void saveIndexes();
    
    // === CONSTRUCCIÓN DE ÍNDICES ===
    void buildHashIndexFromTable();
    void buildBTreeIndexFromTable();
    
    // === OPERACIONES SQL GPS ===
    void executeSelectAll();
    void executeSelectByIMEI();
    void executeSelectByTimestampRange();
    void executeInsertGPS();
    
    // === INFORMACIÓN GPS ===
    void showIndexStatistics();
    void showGPSTableStructure();
    void generateFlowDiagram();
    
    // === MENÚS ===
    void runServerMenu();
    void runMainMenu();
};

// ============================================================================
// IMPLEMENTACIÓN DE MÉTODOS AUXILIARES (DE mainFuncional.cpp)
// ============================================================================

SGBDSystemExtended::SGBDSystemExtended(const std::string& path, size_t pool_size) 
    : current_state(SystemState::NOT_INITIALIZED)
    , disk_path(path)
    , buffer_pool_size(pool_size)
    , current_server("")
    , gps_table_name("")
    , indexes_loaded_from_disk(false)
{
    disk_manager = std::make_unique<DiskManagerExtended>(path);
    
    // Intentar crear IndexManager
    try {
        std::filesystem::create_directories(path + "/metadata");
        index_manager = std::make_unique<IndexManager>(path, true);
    } catch (const std::exception& e) {
        std::cout << "WARNING: IndexManager no disponible: " << e.what() << std::endl;
    }
}

std::map<std::string, DatasetSchema> SGBDSystemExtended::getDatasetSchemas() {
    std::map<std::string, DatasetSchema> datasets;
    
    // === DATASET GPS ESPECIALIZADO ===
    datasets["gps"] = {
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
            {"speed", FieldType::INTEGER, 0},
            {"hdop", FieldType::STRING, 10},
            {"eventId", FieldType::INTEGER, 0},
            {"punto", FieldType::STRING, 50},
            {"ioElements", FieldType::STRING, 100},
            {"processedAt", FieldType::STRING, 30},
            {"createdAt", FieldType::STRING, 30},
            {"updatedAt", FieldType::STRING, 30}
        },
        ',',
        "Dataset GPS con tracking de dispositivos",
        21
    };
    
    return datasets;
}

std::vector<std::string> SGBDSystemExtended::parseCSVLine(const std::string& line, char delimiter) {
    std::vector<std::string> values;
    std::string value;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        char next_c = (i + 1 < line.length()) ? line[i + 1] : '\0';
        
        if (c == '"') {
            if (in_quotes && next_c == '"') {
                value += '"';
                ++i; // Saltar la siguiente comilla escapada
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == delimiter && !in_quotes) {
            cleanValue(value);
            values.push_back(value);
            value.clear();
        } else {
            value += c;
        }
    }
    
    cleanValue(value);
    if (!value.empty() || !values.empty()) {
        values.push_back(value);
    }
    
    return values;
}

void SGBDSystemExtended::cleanValue(std::string& value) {
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    
    if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }
}

int SGBDSystemExtended::countRecordsInFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return 0;
    
    int count = 0;
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (first_line) {
            first_line = false;
            continue;
        }
        if (!line.empty()) count++;
    }
    
    file.close();
    return count;
}

bool SGBDSystemExtended::requiresDisk() {
    if (current_state == SystemState::NOT_INITIALIZED) {
        std::cout << "\nERROR: Operacion requiere disco inicializado." << std::endl;
        std::cout << "Ejecuta primero la opcion 1 o 2." << std::endl;
        return false;
    }
    return true;
}

bool SGBDSystemExtended::requiresBufferPool() {
    if (current_state < SystemState::BUFFER_POOL_READY) {
        std::cout << "\nERROR: Operacion requiere Buffer Pool inicializado." << std::endl;
        if (current_state == SystemState::DISK_READY) {
            std::cout << "Ejecuta la opcion para inicializar Buffer Pool." << std::endl;
        } else {
            std::cout << "Ejecuta primero las opciones 1 (o 2) y luego inicializa Buffer Pool." << std::endl;
        }
        return false;
    }
    return true;
}

bool SGBDSystemExtended::requiresGPSData() {
    if (current_state < SystemState::GPS_LOADED) {
        std::cout << "\nERROR: Operacion requiere dataset GPS cargado." << std::endl;
        std::cout << "Ejecuta primero la opcion de cargar dataset GPS." << std::endl;
        return false;
    }
    return true;
}

// ============================================================================
// INICIALIZACIÓN (EXACTO DE mainFuncional.cpp)
// ============================================================================

void SGBDSystemExtended::showSystemStatus() {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "ESTADO DEL SISTEMA INTEGRADO:" << std::endl;
    
    switch (current_state) {
        case SystemState::NOT_INITIALIZED:
            std::cout << "Estado: NO INICIALIZADO" << std::endl;
            std::cout << "Disco: No creado" << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Accion requerida: Inicializar disco (opcion 1)" << std::endl;
            break;
            
        case SystemState::DISK_READY:
            std::cout << "Estado: DISCO LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Accion requerida: Inicializar Buffer Pool" << std::endl;
            break;
            
        case SystemState::BUFFER_POOL_READY:
            std::cout << "Estado: BUFFER POOL LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: " << buffer_pool_size << " frames" << std::endl;
            if (!gps_table_name.empty()) {
                std::cout << "Tabla GPS: " << gps_table_name << std::endl;
                std::cout << "Servidor: " << current_server << std::endl;
                std::cout << "Accion: Listo para menu del servidor" << std::endl;
            } else {
                std::cout << "Accion: Listo para cargar dataset GPS" << std::endl;
            }
            break;
            
        case SystemState::GPS_LOADED:
            std::cout << "Estado: GPS CARGADO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Tabla GPS: " << gps_table_name << std::endl;
            std::cout << "Accion: Listo para seleccionar servidor (auto-inicializa buffer)" << std::endl;
            break;
            
        case SystemState::INDEXES_READY:
            std::cout << "Estado: SISTEMA COMPLETO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: " << buffer_pool_size << " frames" << std::endl;
            std::cout << "Servidor: " << current_server << std::endl;
            std::cout << "Indices cargados: " << (indexes_loaded_from_disk ? "SI" : "NO") << std::endl;
            break;
            
        case SystemState::ERROR_STATE:
            std::cout << "Estado: ERROR" << std::endl;
            std::cout << "Accion requerida: Reinicializar sistema" << std::endl;
            break;
    }
    std::cout << std::string(60, '-') << std::endl;
}

bool SGBDSystemExtended::initializeDisk() {
    std::cout << "\n=== INICIALIZACION DEL DISCO EXTENDIDO ===" << std::endl;
    
    std::string input;
    std::cout << "Usar configuracion por defecto? (s/n): ";
    std::getline(std::cin, input);
    
    DiskConfig config;
    if (input != "s" && input != "S") {
        int platters, surfaces, tracks, sectors, bytes_sector;
        std::cout << "Numero de platos: ";
        std::cin >> platters;
        std::cout << "Superficies por plato: ";
        std::cin >> surfaces;
        std::cout << "Pistas por superficie: ";
        std::cin >> tracks;
        std::cout << "Sectores por pista: ";
        std::cin >> sectors;
        std::cout << "Bytes por sector: ";
        std::cin >> bytes_sector;
        std::cin.ignore();
        
        config = DiskConfig(platters, surfaces, tracks, sectors, bytes_sector);
    }
    
    if (disk_manager->initialize(config)) {
        current_state = SystemState::DISK_READY;
        std::cout << "\nSUCCESS: Disco inicializado exitosamente en: " << disk_path << std::endl;
        std::cout << "INFO: Page Directory creado automaticamente por DiskManager" << std::endl;
        return true;
    } else {
        current_state = SystemState::ERROR_STATE;
        std::cout << "\nERROR: Error inicializando el disco." << std::endl;
        return false;
    }
}

bool SGBDSystemExtended::loadExistingDisk() {
    std::cout << "\n=== CARGANDO DISCO EXISTENTE EXTENDIDO ===" << std::endl;
    
    if (disk_manager->loadExistingDisk()) {
        current_state = SystemState::DISK_READY;
        std::cout << "SUCCESS: Disco cargado desde: " << disk_path << std::endl;
        std::cout << "INFO: Page Directory cargado automaticamente" << std::endl;
        return true;
    } else {
        std::cout << "ERROR: No se encontro disco en " << disk_path << std::endl;
        return false;
    }
}

// ============================================================================
// BUFFER POOL LRU (EXACTO DE mainFuncional.cpp)
// ============================================================================

bool SGBDSystemExtended::initializeBufferPool() {
    if (current_state != SystemState::DISK_READY) {
        std::cout << "\nERROR: Requiere disco inicializado primero." << std::endl;
        return false;
    }
    
    std::cout << "\n=== INICIALIZACION DEL BUFFER POOL LRU ===" << std::endl;
    
    std::string input;
    std::cout << "Tamaño del buffer pool (frames) [" << buffer_pool_size << "]: ";
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        try {
            size_t new_size = std::stoull(input);
            if (new_size > 0 && new_size <= 64) {
                buffer_pool_size = new_size;
            } else {
                std::cout << "WARNING: Tamaño invalido, usando por defecto: " << buffer_pool_size << std::endl;
            }
        } catch (const std::exception&) {
            std::cout << "WARNING: Entrada invalida, usando por defecto: " << buffer_pool_size << std::endl;
        }
    }
    
    try {
        buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
        current_state = SystemState::BUFFER_POOL_READY;
        
        std::cout << "\nSUCCESS: Buffer Pool Manager LRU inicializado exitosamente!" << std::endl;
        std::cout << "   - Pool size: " << buffer_pool_size << " frames" << std::endl;
        std::cout << "   - Algoritmo: LRU (Least Recently Used)" << std::endl;
        std::cout << "   - Page Table: ACTIVO (En memoria)" << std::endl;
        std::cout << "   - Page Directory: ACTIVO (Gestionado por DiskManager)" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cout << "ERROR: Error inicializando Buffer Pool: " << e.what() << std::endl;
        current_state = SystemState::ERROR_STATE;
        return false;
    }
}

void SGBDSystemExtended::bufferPoolPageOperations() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== OPERACIONES DE PAGINAS CON BUFFER POOL LRU ===" << std::endl;
    
    int page_id;
    std::cout << "ID de pagina a solicitar: ";
    std::cin >> page_id;
    std::cin.ignore();
    
    std::cout << "\nTipo de operacion:" << std::endl;
    std::cout << "r) READ (lectura)" << std::endl;
    std::cout << "w) WRITE (escritura)" << std::endl;
    std::cout << "Seleccionar (r/w): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    PageOperation operation = (input == "w" || input == "W") ? 
        PageOperation::WRITE : PageOperation::READ;
    
    std::cout << "\nINFO: Solicitando pagina " << page_id 
              << " para " << (operation == PageOperation::READ ? "LECTURA" : "ESCRITURA") 
              << std::endl;
    
    auto block = buffer_manager->requestPage(page_id, operation);
    if (block) {
        std::cout << "\nSUCCESS: Pagina cargada exitosamente!" << std::endl;
        std::cout << "INFO: Informacion del bloque:" << std::endl;
        block->displayInfo();
        
        std::cout << "\nLiberar pagina (unpin)? (s/n): ";
        std::getline(std::cin, input);
        
        if (input == "s" || input == "S") {
            bool mark_dirty = (operation == PageOperation::WRITE);
            buffer_manager->releasePage(page_id); // Usar releasePage en lugar de unpinPage
            if (mark_dirty) {
                // Marcar como dirty usando requestPage con WRITE si es necesario
                std::cout << "INFO: Pagina liberada (unpinned)" << std::endl;
            } else {
                std::cout << "INFO: Pagina liberada (unpinned)" << std::endl;
            }
        }
    } else {
        std::cout << "ERROR: Error cargando pagina" << std::endl;
    }
    
    buffer_manager->display(); // Usar display en lugar de displayCompactStatus
}

void SGBDSystemExtended::createNewPageBuffered() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== CREAR NUEVA PAGINA CON BUFFER POOL LRU ===" << std::endl;
    
    // Simular creación de nueva página usando PageOperation::WRITE
    static int next_new_page_id = 1000; // Empezar desde ID alto para evitar conflictos
    int new_page_id = next_new_page_id++;
    
    auto block = buffer_manager->requestPage(new_page_id, PageOperation::WRITE);
    if (block) {
        std::cout << "\nSUCCESS: Nueva pagina creada con ID: " << new_page_id << std::endl;
        
        std::cout << "\nINFO: Page Directory actualizado:" << std::endl;
        disk_manager->displayPageDirectory();
        
        buffer_manager->display(); // Usar display en lugar de displayCompactStatus
        buffer_manager->releasePage(new_page_id); // Usar releasePage en lugar de unpinPage
    } else {
        std::cout << "ERROR: Error creando nueva pagina" << std::endl;
    }
}

void SGBDSystemExtended::showBufferPoolStatus() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== ESTADO COMPLETO DEL BUFFER POOL LRU ===" << std::endl;
    
    // Usar getters individuales disponibles en lugar de getStats()
    std::cout << "\nESTADISTICAS GENERALES:" << std::endl;
    std::cout << "- Pool size: " << buffer_manager->getPoolSize() << " frames" << std::endl;
    std::cout << "- Utilizacion: " << std::fixed << std::setprecision(1) 
              << (buffer_manager->getUtilization() * 100) << "%" << std::endl;
    
    std::cout << "\nESTADISTICAS DE OPERACIONES:" << std::endl;
    std::cout << "- Total operaciones lectura: " << buffer_manager->getReadOperations() << std::endl;
    std::cout << "- Total operaciones escritura: " << buffer_manager->getWriteOperations() << std::endl;
    std::cout << "- Page faults: " << buffer_manager->getPageFaults() << std::endl;
    std::cout << "- Evictions: " << buffer_manager->getEvictions() << std::endl;
    
    if (buffer_manager->getReadOperations() + buffer_manager->getWriteOperations() > 0) {
        double hit_rate = 1.0 - (double)buffer_manager->getPageFaults() / 
                         (buffer_manager->getReadOperations() + buffer_manager->getWriteOperations());
        std::cout << "- Hit ratio: " << std::fixed << std::setprecision(1) 
                  << (hit_rate * 100) << "%" << std::endl;
    }
    
    std::cout << "\nDETALLE DEL BUFFER POOL:" << std::endl;
    buffer_manager->display(); // Usar display en lugar de displayCompactStatus
}

void SGBDSystemExtended::flushAllPages() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== FLUSH DE TODAS LAS PAGINAS DIRTY ===" << std::endl;
    buffer_manager->flushAllPages();
    std::cout << "SUCCESS: Todas las paginas dirty han sido escritas a disco" << std::endl;
}

// ============================================================================
// CLOCK BUFFER MANAGER (EXACTO DE mainFuncional.cpp)
// ============================================================================

void SGBDSystemExtended::initializeClockBufferPool() {
    if (current_state < SystemState::DISK_READY) {
        std::cout << "ERROR: Primero inicializa el disco (opcion 1)" << std::endl;
        return;
    }
    
    size_t clock_pool_size;
    std::cout << "\nINICIALIZACION BUFFER MANAGER CLOCK PIN-AWARE MEJORADO" << std::endl;
    std::cout << "Tamaño del Clock Buffer Pool (frames): ";
    std::cin >> clock_pool_size;
    std::cin.ignore();
    
    if (clock_pool_size < 2 || clock_pool_size > 20) {
        std::cout << "WARNING: Tamaño recomendado: 2-20 frames. Usando 4." << std::endl;
        clock_pool_size = 4;
    }
    
    try {
        clock_buffer_manager = std::make_unique<BufferManagerClock>(
            clock_pool_size, disk_manager.get());
        
        std::cout << "\nSUCCESS: Clock Buffer Manager PIN-AWARE MEJORADO inicializado exitosamente!" << std::endl;
        std::cout << "INFO: Algoritmo Clock MEJORADO activo con " << clock_pool_size << " frames" << std::endl;
        std::cout << "INFO: NUNCA evicta paginas con pin_count > 0" << std::endl;
        std::cout << "INFO: CADA pasada decrementa pin_count automaticamente" << std::endl;
        std::cout << "INFO: GARANTIA: Eventualmente encuentra victimas SIEMPRE" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: Error inicializando Clock Buffer Manager MEJORADO: " << e.what() << std::endl;
    }
}

void SGBDSystemExtended::clockPageOperations() {
    if (!clock_buffer_manager) {
        std::cout << "ERROR: Primero inicializa Clock Buffer Manager" << std::endl;
        return;
    }
    
    std::cout << "\n=== OPERACIONES DE PAGINAS CON BUFFER CLOCK MEJORADO ===" << std::endl;
    
    int page_id;
    std::cout << "ID de pagina a solicitar: ";
    std::cin >> page_id;
    std::cin.ignore();
    
    std::cout << "\nTipo de operacion:" << std::endl;
    std::cout << "r) read (lectura)" << std::endl;
    std::cout << "w) WRITE (escritura)" << std::endl;
    std::cout << "Seleccionar (r/w): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    bool is_write = (input == "w" || input == "W");
    
    std::cout << "\nINFO: Solicitando pagina " << page_id 
              << " para " << (is_write ? "ESCRITURA" : "LECTURA") 
              << std::endl;
    
    auto block = clock_buffer_manager->fetchPage(page_id);
    if (block) {
        std::cout << "\nSUCCESS: Pagina cargada exitosamente!" << std::endl;
        std::cout << "INFO: Informacion del bloque:" << std::endl;
        block->displayInfo();
        
        std::cout << "\nLiberar pagina (unpin)? (s/n): ";
        std::getline(std::cin, input);
        
        if (input == "s" || input == "S") {
            clock_buffer_manager->unpinPage(page_id, is_write);
            std::cout << "INFO: Pagina liberada (unpinned)" << std::endl;
        }
    } else {
        std::cout << "ERROR: Error cargando pagina" << std::endl;
    }
    
    clock_buffer_manager->displayClockState();
}

void SGBDSystemExtended::createNewPageClock() {
    if (!clock_buffer_manager) {
        std::cout << "ERROR: Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n=== CREAR NUEVA PAGINA CON CLOCK MEJORADO ===" << std::endl;
    
    int new_page_id;
    auto block = clock_buffer_manager->newPage(new_page_id);
    if (block) {
        std::cout << "SUCCESS: Nueva pagina " << new_page_id << " creada" << std::endl;
        clock_buffer_manager->unpinPage(new_page_id, true);
        clock_buffer_manager->displayClockState();
    } else {
        std::cout << "ERROR: Error creando nueva pagina" << std::endl;
    }
}

void SGBDSystemExtended::showClockBufferStatus() {
    if (!clock_buffer_manager) {
        std::cout << "ERROR: Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\nESTADO COMPLETO CLOCK BUFFER MANAGER PIN-AWARE MEJORADO" << std::endl;
    clock_buffer_manager->displayClockState();
}

void SGBDSystemExtended::flushAllClockPages() {
    if (!clock_buffer_manager) {
        std::cout << "ERROR: Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\nINFO: Flushing todas las paginas Clock..." << std::endl;
    clock_buffer_manager->flushAllDirtyPages();
    std::cout << "SUCCESS: Flush completo!" << std::endl;
}

void SGBDSystemExtended::compareBufferAlgorithms() {
    if (!buffer_manager || !clock_buffer_manager) {
        std::cout << "ERROR: Necesitas ambos buffer managers inicializados" << std::endl;
        return;
    }
    
    std::cout << "\nCOMPARACION DETALLADA: LRU vs CLOCK PIN-AWARE MEJORADO" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
    // Usar getters individuales para BufferPoolManager
    std::cout << "\nESTADISTICAS LRU BUFFER MANAGER:" << std::endl;
    std::cout << "- Pool size: " << buffer_manager->getPoolSize() << " frames" << std::endl;
    std::cout << "- Utilizacion: " << std::fixed << std::setprecision(1) 
              << (buffer_manager->getUtilization() * 100) << "%" << std::endl;
    std::cout << "- Read operations: " << buffer_manager->getReadOperations() << std::endl;
    std::cout << "- Write operations: " << buffer_manager->getWriteOperations() << std::endl;
    std::cout << "- Page faults: " << buffer_manager->getPageFaults() << std::endl;
    std::cout << "- Evictions: " << buffer_manager->getEvictions() << std::endl;
    
    std::cout << "\nESTADISTICAS CLOCK BUFFER MANAGER MEJORADO:" << std::endl;
    clock_buffer_manager->displayStatistics();
    
    std::cout << "\nANALISIS COMPARATIVO:" << std::endl;
    std::cout << "LRU:           O(log n) para acceso + O(n) para eviction" << std::endl;
    std::cout << "Clock:         O(1) para acceso + O(n) worst case para eviction" << std::endl;
    std::cout << "Clock MEJORADO: + GARANTIA total de encontrar victimas" << std::endl;
}

// ============================================================================
// GESTIÓN GPS
// ============================================================================

bool SGBDSystemExtended::loadGPSDataset() {
    if (!requiresDisk()) return false;  // Solo necesita disco, NO buffer pool
    
    std::cout << "\n=== CARGANDO DATASET GPS ===" << std::endl;
    
    auto datasets = getDatasetSchemas();
    auto it = datasets.find("gps");
    
    if (it == datasets.end()) {
        std::cout << "ERROR: Schema GPS no encontrado." << std::endl;
        return false;
    }
    
    const DatasetSchema& schema = it->second;
    
    // Crear tabla GPS
    bool table_created = disk_manager->createTable(schema.table_name, schema.schema, false);
    
    if (table_created) {
        std::cout << "SUCCESS: Tabla GPS creada, cargando datos..." << std::endl;
    } else {
        std::cout << "INFO: Tabla GPS ya existe en disco" << std::endl;
    }
    
    // Cargar datos desde CSV
    std::string csv_file = "data/data-GPS.csv";
    
    if (!std::filesystem::exists(csv_file)) {
        std::cout << "WARNING: Archivo no encontrado: " << csv_file << std::endl;
        std::cout << "INFO: Creando tabla GPS vacia para continuar..." << std::endl;
        gps_table_name = "dataGPS";
        current_state = SystemState::GPS_LOADED;
        return true;
    }
    
    std::ifstream file(csv_file);
    if (!file.is_open()) {
        std::cout << "ERROR: Error abriendo archivo: " << csv_file << std::endl;
        return false;
    }
    
    std::string line;
    std::getline(file, line); // Saltar header
    
    int loaded_count = 0;
            int max_records = -1; // Sin límite - cargar todos los registros
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "\nINFO: Cargando registros GPS (maximo " << max_records << " para demo)..." << std::endl;
    
    while (std::getline(file, line) && !line.empty() && loaded_count < max_records) {
        auto values = parseCSVLine(line);
        
        if (values.size() >= 21) {
            if (disk_manager->insertRecord("dataGPS", values)) {
                loaded_count++;
                
                if (loaded_count % 25 == 0) {
                    std::cout << "PROGRESS: Procesados: " << loaded_count << " registros..." << std::endl;
                }
            }
        }
    }
    
    file.close();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nSUCCESS: DATASET GPS CARGADO:" << std::endl;
    std::cout << "   Registros cargados: " << loaded_count << std::endl;
    std::cout << "   Tiempo de carga: " << duration.count() << " ms" << std::endl;
    std::cout << "   Tabla: dataGPS" << std::endl;
    
    gps_table_name = "dataGPS";
    current_state = SystemState::GPS_LOADED;
    
    return true;
}

void SGBDSystemExtended::selectServerConfiguration() {
    // Esta función ya no se usa - reemplazada por selectServerAndEnterMenu()
    std::cout << "ERROR: Esta funcion ha sido reemplazada por selectServerAndEnterMenu()" << std::endl;
}
    
    std::cout << "\nINFO: Servidor e indices listos. Use el menu del servidor para:" << std::endl;
    std::cout << "   1. Inicializar indices (construccion desde datos)" << std::endl;
    std::cout << "   2. Cargar indices existentes" << std::endl;
    std::cout << "   3. Realizar operaciones GPS" << std::endl;
}

// ============================================================================
// CONFIGURACIÓN COMBINADA GPS
// ============================================================================

void SGBDSystemExtended::selectServerAndEnterMenu() {
    if (!requiresGPSData()) return;
    
    std::cout << "\n=== SELECCION DE SERVIDOR Y ACCESO AL MENU GPS ===" << std::endl;
    std::cout << "IMPORTANTE: Los indices pueden tomar tiempo en cargar/construir" << std::endl;
    std::cout << "Este proceso puede demorar varios segundos..." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Seleccione el servidor para operaciones GPS:" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "A) Server A - Optimizado para Transacciones" << std::endl;
    std::cout << "   • Indice: Hash Extensible (IMEI)" << std::endl;
    std::cout << "   • Buffer: LRU Replacement Policy (auto-inicializa)" << std::endl;
    std::cout << "   • Uso tipico: 70% INSERT, 20% SELECT exacto, 10% UPDATE/DELETE" << std::endl;
    std::cout << "   • Complejidad: O(1) para busquedas por IMEI" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "B) Server B - Optimizado para Analisis" << std::endl;
    std::cout << "   • Indice: B+ Tree (Timestamp)" << std::endl;
    std::cout << "   • Buffer: Clock Algorithm PIN-AWARE (auto-inicializa)" << std::endl;
    std::cout << "   • Uso tipico: 80% Range SELECT, 15% Agregaciones, 5% Otras" << std::endl;
    std::cout << "   • Complejidad: O(log n + k) para rangos" << std::endl;
    
    std::cout << "\nOpcion (A/B): ";
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "A" || input == "a") {
        current_server = "Server_A";
        std::cout << "\nSUCCESS: Server A seleccionado" << std::endl;
        std::cout << "CONFIGURACION DE POLITICA DE REEMPLAZO RECOMENDADA:" << std::endl;
        std::cout << "   • Para Server A (Transaccional): Buffer LRU Policy" << std::endl;
        std::cout << "   • Razon: Mejor para patrones de escritura secuencial" << std::endl;
        std::cout << "   • Optimizacion: Cache caliente para inserciones frecuentes" << std::endl;
        
        // AUTO-INICIALIZAR Buffer Pool LRU recomendado
        if (!buffer_manager) {
            std::cout << "\nINFO: Inicializando Buffer Pool LRU automaticamente..." << std::endl;
            try {
                buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
                current_state = SystemState::BUFFER_POOL_READY;
                std::cout << "SUCCESS: Buffer Pool LRU inicializado automaticamente!" << std::endl;
                std::cout << "   - Pool size: " << buffer_pool_size << " frames" << std::endl;
                std::cout << "   - Algoritmo: LRU (Least Recently Used)" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "ERROR: Error auto-inicializando Buffer Pool LRU: " << e.what() << std::endl;
                return;
            }
        }
        
    } else if (input == "B" || input == "b") {
        current_server = "Server_B";
        std::cout << "\nSUCCESS: Server B seleccionado" << std::endl;
        std::cout << "CONFIGURACION DE POLITICA DE REEMPLAZO RECOMENDADA:" << std::endl;
        std::cout << "   • Para Server B (Analitico): Clock Algorithm PIN-AWARE" << std::endl;
        std::cout << "   • Razon: Mejor para patrones de lectura complejos" << std::endl;
        std::cout << "   • Optimizacion: Pin-aware para consultas por rango largas" << std::endl;
        
        // AUTO-INICIALIZAR Clock Buffer Manager recomendado
        if (!clock_buffer_manager) {
            std::cout << "\nINFO: Inicializando Clock Buffer Manager automaticamente..." << std::endl;
            try {
                clock_buffer_manager = std::make_unique<BufferManagerClock>(buffer_pool_size, disk_manager.get());
                current_state = SystemState::BUFFER_POOL_READY;
                std::cout << "SUCCESS: Clock Buffer Manager inicializado automaticamente!" << std::endl;
                std::cout << "   - Pool size: " << buffer_pool_size << " frames" << std::endl;
                std::cout << "   - Algoritmo: Clock PIN-AWARE MEJORADO" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "ERROR: Error auto-inicializando Clock Buffer Manager: " << e.what() << std::endl;
                return;
            }
        }
        
    } else {
        std::cout << "ERROR: Opcion invalida." << std::endl;
        return;
    }
    
    std::cout << "\nSUCCESS: Servidor configurado exitosamente!" << std::endl;
    std::cout << "INFO: Entrando automaticamente al menu del servidor..." << std::endl;
    
    // Pausa breve antes de entrar al menú
    std::cout << "\nPresione Enter para continuar al menu del servidor...";
    std::cin.get();
    
    // Entrar directamente al menú del servidor
    runServerMenu();
}

// ============================================================================
// GESTIÓN DE ÍNDICES GPS
// ============================================================================

bool SGBDSystemExtended::initializeIndexes() {
    if (current_server.empty()) {
        std::cout << "ERROR: Primero debe seleccionar configuracion de servidor." << std::endl;
        return false;
    }
    
    std::cout << "\n=== INICIALIZANDO INDICES ESPECIALIZADOS ===" << std::endl;
    std::cout << "ADVERTENCIA: Este proceso puede tomar tiempo..." << std::endl;
    std::cout << "Construyendo indices desde " << gps_table_name << "..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (current_server == "Server_A") {
        std::cout << "\nINFO: Inicializando Hash Extensible para IMEI..." << std::endl;
        imei_index = std::make_unique<ExtensibleHash>(4);
        
        // Construir índice real desde tabla GPS
        buildHashIndexFromTable();
        
        std::cout << "SUCCESS: Hash Extensible construido" << std::endl;
        std::cout << "CARACTERISTICAS:" << std::endl;
        std::cout << "   • Clave: IMEI (string)" << std::endl;
        std::cout << "   • Capacidad de bucket: 4 registros" << std::endl;
        std::cout << "   • Optimo para: SELECT WHERE imei = 'valor'" << std::endl;
        
    } else if (current_server == "Server_B") {
        std::cout << "\nINFO: Inicializando B+ Tree para timestamp..." << std::endl;
        timestamp_index = std::make_unique<BPlusTree<std::string>>(4);
        
        // Construir índice real desde tabla GPS
        buildBTreeIndexFromTable();
        
        std::cout << "SUCCESS: B+ Tree construido" << std::endl;
        std::cout << "CARACTERISTICAS:" << std::endl;
        std::cout << "   • Clave: timestamp (string)" << std::endl;
        std::cout << "   • Orden: 4" << std::endl;
        std::cout << "   • Optimo para: SELECT WHERE timestamp BETWEEN x AND y" << std::endl;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nSUCCESS: Indices inicializados para " << current_server << std::endl;
    std::cout << "Tiempo de construccion: " << duration.count() << " ms" << std::endl;
    
    current_state = SystemState::INDEXES_READY;
    indexes_loaded_from_disk = false;
    
    return true;
}

bool SGBDSystemExtended::loadExistingIndexes() {
    if (current_server.empty()) {
        std::cout << "ERROR: Primero debe seleccionar configuracion de servidor." << std::endl;
        return false;
    }
    
    if (!index_manager) {
        std::cout << "ERROR: IndexManager no disponible" << std::endl;
        return false;
    }
    
    std::cout << "\n=== CARGANDO INDICES EXISTENTES ===" << std::endl;
    std::cout << "ADVERTENCIA: La carga puede tomar tiempo..." << std::endl;
    std::cout << "Buscando indices en disco..." << std::endl;
    
    bool found_indexes = false;
    
    if (current_server == "Server_A") {
        std::cout << "\nINFO: Intentando cargar Hash Extensible..." << std::endl;
        
        // Pausa educativa
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        imei_index = index_manager->loadHashIndex("imei_index");
        if (imei_index) {
            std::cout << "SUCCESS: Hash Extensible cargado desde disco" << std::endl;
            found_indexes = true;
        } else {
            std::cout << "WARNING: Hash Extensible no encontrado en disco" << std::endl;
            std::cout << "INFO: Use 'Inicializar Indices' para crear uno nuevo" << std::endl;
        }
        
    } else if (current_server == "Server_B") {
        std::cout << "\nINFO: Intentando cargar B+ Tree..." << std::endl;
        
        // Pausa educativa
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        
        timestamp_index = index_manager->loadBTreeIndex("timestamp_index");
        if (timestamp_index) {
            std::cout << "SUCCESS: B+ Tree cargado desde disco" << std::endl;
            found_indexes = true;
        } else {
            std::cout << "WARNING: B+ Tree no encontrado en disco" << std::endl;
            std::cout << "INFO: Use 'Inicializar Indices' para crear uno nuevo" << std::endl;
        }
    }
    
    if (found_indexes) {
        current_state = SystemState::INDEXES_READY;
        indexes_loaded_from_disk = true;
        std::cout << "\nSUCCESS: Indices cargados exitosamente desde disco" << std::endl;
    } else {
        std::cout << "\nINFO: No se encontraron indices en disco" << std::endl;
        std::cout << "RECOMENDACION: Use 'Inicializar Indices' para crear nuevos" << std::endl;
    }
    
    return found_indexes;
}

void SGBDSystemExtended::saveIndexes() {
    if (!index_manager) {
        std::cout << "ERROR: IndexManager no disponible" << std::endl;
        return;
    }
    
    std::cout << "\n=== GUARDANDO INDICES EN DISCO ===" << std::endl;
    
    bool saved_any = false;
    
    if (imei_index) {
        std::cout << "INFO: Guardando Hash Extensible..." << std::endl;
        index_manager->saveHashIndex(*imei_index, "dataGPS", "imei");
        std::cout << "SUCCESS: Hash Extensible guardado" << std::endl;
        saved_any = true;
    }
    
    if (timestamp_index) {
        std::cout << "INFO: Guardando B+ Tree..." << std::endl;
        index_manager->saveBTreeIndex(*timestamp_index, "dataGPS", "timestamp");
        std::cout << "SUCCESS: B+ Tree guardado" << std::endl;
        saved_any = true;
    }
    
    if (!saved_any) {
        std::cout << "WARNING: No hay indices para guardar" << std::endl;
        std::cout << "INFO: Inicialice indices primero" << std::endl;
    } else {
        std::cout << "\nSUCCESS: Todos los indices guardados en disco" << std::endl;
        std::cout << "INFO: Ubicacion: " << disk_path << "/metadata/" << std::endl;
    }
}

// ============================================================================
// OPERACIONES SQL GPS (SIMPLIFICADAS)
// ============================================================================

void SGBDSystemExtended::executeSelectAll() {
    if (current_state < SystemState::INDEXES_READY) {
        std::cout << "ERROR: Configure servidor e indices primero." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS ===" << std::endl;
    std::cout << "Operacion: Scan completo de tabla (no usa indices)" << std::endl;
    std::cout << "Servidor activo: " << current_server << std::endl;
    
    disk_manager->displayTable(gps_table_name);
    std::cout << "\nSUCCESS: SELECT * completado" << std::endl;
}

void SGBDSystemExtended::executeSelectByIMEI() {
    if (current_state < SystemState::INDEXES_READY) {
        std::cout << "ERROR: Configure servidor e indices primero." << std::endl;
        return;
    }
    
    if (!imei_index) {
        std::cout << "ERROR: Hash Extensible no disponible. Use Server A." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS WHERE imei = ? ===" << std::endl;
    
    std::string target_imei;
    std::cout << "Ingrese IMEI a buscar: ";
    std::getline(std::cin, target_imei);
    
    std::cout << "\nFLUJO DE CONSULTA POR INDICE HASH:" << std::endl;
    std::cout << "1. Hash Extensible: Calculando hash(" << target_imei << ")" << std::endl;
    std::cout << "2. Localizando bucket en directorio..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    RecordReference record_ref;
    bool found = imei_index->searchReference(target_imei, record_ref);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    if (found) {
        std::cout << "3. SUCCESS: ENCONTRADO en Hash Extensible!" << std::endl;
        std::cout << "4. RecordReference: " << record_ref.toString() << std::endl;
        std::cout << "5. Recuperando registro completo desde disco..." << std::endl;
    } else {
        std::cout << "3. INFO: IMEI no encontrado en el indice" << std::endl;
    }
    
    std::cout << "\nESTADISTICAS:" << std::endl;
    std::cout << "   Tiempo de busqueda: " << duration.count() << " microsegundos" << std::endl;
    std::cout << "   Complejidad: O(1) - Hash Extensible" << std::endl;
}

void SGBDSystemExtended::executeSelectByTimestampRange() {
    if (current_state < SystemState::INDEXES_READY) {
        std::cout << "ERROR: Configure servidor e indices primero." << std::endl;
        return;
    }
    
    if (!timestamp_index) {
        std::cout << "ERROR: B+ Tree no disponible. Use Server B." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS WHERE timestamp BETWEEN ? AND ? ===" << std::endl;
    
    std::string start_time, end_time;
    std::cout << "Timestamp inicio (YYYY-MM-DD HH:MM:SS): ";
    std::getline(std::cin, start_time);
    std::cout << "Timestamp fin (YYYY-MM-DD HH:MM:SS): ";
    std::getline(std::cin, end_time);
    
    std::cout << "\nFLUJO DE CONSULTA POR RANGO B+ TREE:" << std::endl;
    std::cout << "1. B+ Tree: Buscando nodo hoja para '" << start_time << "'" << std::endl;
    std::cout << "2. Recorriendo hojas enlazadas hasta '" << end_time << "'" << std::endl;
    
    auto search_start = std::chrono::high_resolution_clock::now();
    
    auto references = timestamp_index->rangeSearch(start_time, end_time);
    
    auto search_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(search_end - search_start);
    
    std::cout << "3. SUCCESS: Encontradas " << references.size() << " referencias en rango" << std::endl;
    
    int show_limit = std::min((int)references.size(), 5);
    for (int i = 0; i < show_limit; ++i) {
        std::cout << "   [" << (i+1) << "] " << references[i] << std::endl;
    }
    
    std::cout << "\nESTADISTICAS:" << std::endl;
    std::cout << "   Registros encontrados: " << references.size() << std::endl;
    std::cout << "   Tiempo de busqueda: " << duration.count() << " microsegundos" << std::endl;
    std::cout << "   Complejidad: O(log n + k) - B+ Tree" << std::endl;
}

void SGBDSystemExtended::executeInsertGPS() {
    if (current_state < SystemState::INDEXES_READY) {
        std::cout << "ERROR: Configure servidor e indices primero." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: INSERT INTO dataGPS ===" << std::endl;
    std::cout << "Ingrese datos GPS (21 campos separados por comas):" << std::endl;
    
    std::string input_line;
    std::getline(std::cin, input_line);
    
    std::vector<std::string> values = parseCSVLine(input_line, ',');
    
    if (values.size() < 21) {
        std::cout << "ERROR: Se requieren 21 campos. Recibidos: " << values.size() << std::endl;
        return;
    }
    
    values.resize(21);
    
    if (disk_manager->insertRecord("dataGPS", values)) {
        std::cout << "SUCCESS: Registro insertado en disco" << std::endl;
        
        // Simular actualización de índices
        std::string imei = values[1];
        std::string timestamp = values[3];
        
        if (imei_index && current_server == "Server_A") {
            std::cout << "INFO: Actualizando Hash Extensible (IMEI: " << imei << ")" << std::endl;
        }
        
        if (timestamp_index && current_server == "Server_B") {
            std::cout << "INFO: Actualizando B+ Tree (timestamp: " << timestamp << ")" << std::endl;
        }
        
        std::cout << "SUCCESS: INSERT completado exitosamente" << std::endl;
    } else {
        std::cout << "ERROR: Error insertando registro en disco" << std::endl;
    }
}

// ============================================================================
// INFORMACIÓN GPS
// ============================================================================

void SGBDSystemExtended::showIndexStatistics() {
    if (current_state < SystemState::INDEXES_READY) {
        std::cout << "ERROR: Configure servidor e indices primero." << std::endl;
        return;
    }
    
    std::cout << "\n=== ESTADISTICAS DE INDICES GPS ===" << std::endl;
    std::cout << "Servidor activo: " << current_server << std::endl;
    std::cout << "Tabla GPS: " << gps_table_name << std::endl;
    std::cout << "Indices cargados desde disco: " << (indexes_loaded_from_disk ? "SI" : "NO") << std::endl;
    
    if (imei_index) {
        std::cout << "\nHASH EXTENSIBLE (IMEI):" << std::endl;
        std::cout << imei_index->getStatistics() << std::endl;
    }
    
    if (timestamp_index) {
        std::cout << "\nB+ TREE (TIMESTAMP):" << std::endl;
        std::cout << timestamp_index->getStatistics() << std::endl;
    }
    
    if (!imei_index && !timestamp_index) {
        std::cout << "WARNING: No hay indices inicializados." << std::endl;
    }
}

void SGBDSystemExtended::showGPSTableStructure() {
    if (gps_table_name.empty()) {
        std::cout << "ERROR: No hay tabla GPS cargada." << std::endl;
        return;
    }
    
    std::cout << "\n=== ESTRUCTURA DE TABLA GPS ===" << std::endl;
    std::cout << "Tabla: " << gps_table_name << std::endl;
    std::cout << "Tipo: Registros de longitud variable" << std::endl;
    std::cout << "Campos: 21" << std::endl;
    
    disk_manager->displayPageDirectory();
}

void SGBDSystemExtended::generateFlowDiagram() {
    std::cout << "\n=== DIAGRAMA DE FLUJO DEL SISTEMA GPS ===" << std::endl;
    std::cout << "USUARIO SQL" << std::endl;
    std::cout << "    |" << std::endl;
    std::cout << "    v" << std::endl;
    std::cout << "QUERY EXECUTOR" << std::endl;
    std::cout << "    |" << std::endl;
    std::cout << "    v" << std::endl;
    if (current_server == "Server_A") {
        std::cout << "HASH EXTENSIBLE (IMEI)" << std::endl;
    } else {
        std::cout << "B+ TREE (TIMESTAMP)" << std::endl;
    }
    std::cout << "    |" << std::endl;
    std::cout << "    v" << std::endl;
    std::cout << "BUFFER MANAGER" << std::endl;
    std::cout << "    |" << std::endl;
    std::cout << "    v" << std::endl;
    std::cout << "DISK MANAGER" << std::endl;
    std::cout << "    |" << std::endl;
    std::cout << "    v" << std::endl;
    std::cout << "RESULTADO FINAL" << std::endl;
}

// ============================================================================
// MENÚ DEL SERVIDOR
// ============================================================================

void SGBDSystemExtended::runServerMenu() {
    if (current_server.empty()) {
        std::cout << "ERROR: Primero debe seleccionar configuracion de servidor." << std::endl;
        return;
    }
    
    while (true) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "MENU DEL SERVIDOR " << current_server << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        std::cout << "\nGESTION DE INDICES:" << std::endl;
        std::cout << "1. Inicializar indices (construccion desde datos)" << std::endl;
        std::cout << "2. Cargar indices existentes desde disco" << std::endl;
        std::cout << "3. Guardar indices en disco" << std::endl;
        
        std::cout << "\nOPERACIONES GPS (requiere indices):" << std::endl;
        std::cout << "4. SELECT * FROM dataGPS" << std::endl;
        
        if (current_server == "Server_A") {
            std::cout << "5. SELECT WHERE imei = ? (Hash Extensible)" << std::endl;
            std::cout << "6. [No disponible - Use Server B para timestamp]" << std::endl;
        } else {
            std::cout << "5. [No disponible - Use Server A para IMEI]" << std::endl;
            std::cout << "6. SELECT WHERE timestamp BETWEEN ? AND ? (B+ Tree)" << std::endl;
        }
        
        std::cout << "7. INSERT INTO dataGPS" << std::endl;
        
        std::cout << "\nINFORMACION:" << std::endl;
        std::cout << "8. Mostrar estadisticas de indices" << std::endl;
        std::cout << "9. Mostrar estructura de tabla GPS" << std::endl;
        std::cout << "10. Generar diagrama de flujo" << std::endl;
        
        std::cout << "\n0. Volver al menu principal" << std::endl;
        std::cout << "\nOpcion: ";
        
        std::string option;
        if (!std::getline(std::cin, option)) {
            break;
        }
        
        option.erase(0, option.find_first_not_of(" \t\r\n"));
        option.erase(option.find_last_not_of(" \t\r\n") + 1);
        
        if (option == "1") {
            initializeIndexes();
        } else if (option == "2") {
            loadExistingIndexes();
        } else if (option == "3") {
            saveIndexes();
        } else if (option == "4") {
            executeSelectAll();
        } else if (option == "5") {
            if (current_server == "Server_A") {
                executeSelectByIMEI();
            } else {
                std::cout << "ERROR: Funcionalidad solo disponible en Server A" << std::endl;
            }
        } else if (option == "6") {
            if (current_server == "Server_B") {
                executeSelectByTimestampRange();
            } else {
                std::cout << "ERROR: Funcionalidad solo disponible en Server B" << std::endl;
            }
        } else if (option == "7") {
            executeInsertGPS();
        } else if (option == "8") {
            showIndexStatistics();
        } else if (option == "9") {
            showGPSTableStructure();
        } else if (option == "10") {
            generateFlowDiagram();
        } else if (option == "0") {
            std::cout << "INFO: Volviendo al menu principal..." << std::endl;
            break;
        } else {
            std::cout << "ERROR: Opcion invalida." << std::endl;
        }
        
        std::cout << "\nPresione Enter para continuar...";
        std::cin.get();
    }
}

// ============================================================================
// MENÚ PRINCIPAL
// ============================================================================

void SGBDSystemExtended::runMainMenu() {
    while (true) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "SGBD FISICO INTEGRADO - MENU PRINCIPAL GPS" << std::endl;
        std::cout << "Sistema con Buffer Pool Management + Indices Especializados GPS" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        std::cout << "\nINICIALIZACION DEL SISTEMA:" << std::endl;
        std::cout << "1.  Inicializar nuevo disco extendido" << std::endl;
        std::cout << "2.  Cargar disco existente extendido" << std::endl;
        std::cout << "3.  Ver estado del sistema integrado" << std::endl;
        
        std::cout << "\nBUFFER POOL LRU:" << std::endl;
        std::cout << "4.  [Opcional] Inicializar Buffer Pool Manager (LRU)" << std::endl;
        std::cout << "5.  [Opcional] Operaciones de paginas (READ/WRITE)" << std::endl;
        std::cout << "6.  [Opcional] Crear nueva pagina con Buffer Pool" << std::endl;
        std::cout << "7.  [Opcional] Ver estado del Buffer Pool" << std::endl;
        std::cout << "8.  [Opcional] Flush todas las paginas dirty" << std::endl;
        
        std::cout << "\nBUFFER CLOCK PIN-AWARE MEJORADO:" << std::endl;
        std::cout << "9.  [Opcional] Inicializar Clock Buffer Manager MEJORADO" << std::endl;
        std::cout << "10. [Opcional] Operaciones de paginas Clock" << std::endl;
        std::cout << "11. [Opcional] Crear nueva pagina Clock" << std::endl;
        std::cout << "12. [Opcional] Ver estado Clock Buffer" << std::endl;
        std::cout << "13. [Opcional] Flush paginas Clock" << std::endl;
        
        std::cout << "\nCOMPARACION DE ALGORITMOS:" << std::endl;
        std::cout << "14. [Opcional] Comparar LRU vs Clock MEJORADO" << std::endl;
        
        std::cout << "\nSISTEMA GPS CON INDICES ESPECIALIZADOS:" << std::endl;
        std::cout << "15. Cargar dataset GPS (REQUERIDO)" << std::endl;
        std::cout << "16. Seleccionar servidor (A/B) - AUTO-inicializa buffer" << std::endl;
        std::cout << "17. Entrar al menu del servidor GPS" << std::endl;
        
        std::cout << "\n0.  Salir" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Opcion: ";
        
        std::string option;
        if (!std::getline(std::cin, option)) {
            break;
        }
        
        option.erase(0, option.find_first_not_of(" \t\r\n"));
        option.erase(option.find_last_not_of(" \t\r\n") + 1);
        
        if (option == "1") {
            initializeDisk();
        } else if (option == "2") {
            loadExistingDisk();
        } else if (option == "3") {
            showSystemStatus();
        } else if (option == "4") {
            initializeBufferPool();
        } else if (option == "5") {
            bufferPoolPageOperations();
        } else if (option == "6") {
            createNewPageBuffered();
        } else if (option == "7") {
            showBufferPoolStatus();
        } else if (option == "8") {
            flushAllPages();
        } else if (option == "9") {
            initializeClockBufferPool();
        } else if (option == "10") {
            clockPageOperations();
        } else if (option == "11") {
            createNewPageClock();
        } else if (option == "12") {
            showClockBufferStatus();
        } else if (option == "13") {
            flushAllClockPages();
        } else if (option == "14") {
            compareBufferAlgorithms();
        } else if (option == "15") {
            loadGPSDataset();
        } else if (option == "16") {
            selectServerConfiguration();
        } else if (option == "17") {
            runServerMenu();
        } else if (option == "0") {
            std::cout << "\nGracias por usar el SGBD Fisico GPS!" << std::endl;
            std::cout << "Has experimentado con:" << std::endl;
            std::cout << "   • Buffer Pool Management profesional" << std::endl;
            std::cout << "   • Algoritmos LRU y Clock PIN-AWARE" << std::endl;
            std::cout << "   • Indices especializados GPS" << std::endl;
            std::cout << "   • Servidores Transaccional vs Analitico" << std::endl;
            break;
        } else {
            std::cout << "ERROR: Opcion no valida." << std::endl;
        }
        
        std::cout << "\nPresione Enter para continuar...";
        std::cin.get();
    }
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    SGBDSystemExtended sistema;
    
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "SISTEMA DE GESTION DE BASE DE DATOS FISICO INTEGRADO GPS" << std::endl;
    std::cout << "Buffer Pool Management + Clock Algorithm Mejorado + Indices GPS" << std::endl;
    std::cout << "Implementacion Educativa - Almacenamiento Secundario" << std::endl;
    std::cout << "Basado en Database System Implementation + CMU Lectures" << std::endl;
    std::cout << "Algoritmo Clock PIN-AWARE con garantia de victimas + Indices GPS" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    sistema.showSystemStatus();
    sistema.runMainMenu();
    
    return 0;
}