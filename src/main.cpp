#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <map>
#include "../include/DiskManagerExtended.h"
#include "../include/buffer/BufferPoolManager.h"
#include "../include/buffer/ClockReplacer.h"
#include "../include/buffer/BufferManagerClock.h"

/**
 * @brief Estado del sistema actualizado con Buffer Pool
 */
enum class SystemState {
    NOT_INITIALIZED,
    DISK_READY,
    BUFFER_POOL_READY,
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
    SystemState current_state;
    std::string disk_path;
    size_t buffer_pool_size;
    
    // === MÉTODOS AUXILIARES PRIVADOS ===
    std::map<std::string, DatasetSchema> getDatasetSchemas();
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',');
    int countRecordsInFile(const std::string& filename);
    size_t estimateRecordSize(const std::vector<std::string>& values);
    void showDiskStructure(const DiskConfig& config);
    bool requiresDisk();
    bool requiresBufferPool();

public:
    SGBDSystemExtended(const std::string& path = "mi_disco_sgbd", size_t pool_size = 4);
    SystemState getState() const { return current_state; }
    
    // === ESTADO DEL SISTEMA ===
    void showSystemStatus();
    
    // === INICIALIZACIÓN ===
    bool initializeDisk();
    bool loadExistingDisk();
    
    // === GESTIÓN DE TABLAS ===
    void createTable();
    
    // === INSERCIÓN DE DATOS ===
    void insertSingleRecord();
    void loadNRecords();
    void loadCompleteCSV();
    
    // === DATASETS PREDEFINIDOS ===
    bool loadDataset(const std::string& dataset_name, const std::string& filename);
    
    // === SIMULACIONES ===
    void simulateInsufficientSpace();
    void simulateFullSectors();
    
    // === OPERACIONES CRUD ===
    void findRecord();
    void deleteRecord();
    void displayTable();
    void compactTable();
    
    // === INFORMACIÓN DEL SISTEMA ===
    void showStatistics();
    void showDirectoryStructure();
    void showPageDirectory();
    
    // === BUFFER POOL LRU (SIMPLIFICADO) ===
    bool initializeBufferPool();
    void bufferPoolPageOperations();
    void createNewPageBuffered();
    void showBufferPoolStatus();
    void flushAllPages();
    
    // === CLOCK BUFFER MANAGER (SIMPLIFICADO) ===
    void initializeClockBufferPool();
    void clockPageOperations();
    void createNewPageClock();
    void showClockBufferStatus();
    void flushAllClockPages();
    
    // === COMPARACIÓN DE ALGORITMOS ===
    void compareBufferAlgorithms();
};

// ============================================================================
// IMPLEMENTACIÓN DE MÉTODOS AUXILIARES
// ============================================================================

SGBDSystemExtended::SGBDSystemExtended(const std::string& path, size_t pool_size) 
    : current_state(SystemState::NOT_INITIALIZED)
    , disk_path(path)
    , buffer_pool_size(pool_size) 
{
    disk_manager = std::make_unique<DiskManagerExtended>(path);
}

std::map<std::string, DatasetSchema> SGBDSystemExtended::getDatasetSchemas() {
    std::map<std::string, DatasetSchema> datasets;
    
    datasets["housing"] = {
        "viviendas",
        {
            {"price", FieldType::INTEGER, 0},
            {"area", FieldType::INTEGER, 0},
            {"bedrooms", FieldType::INTEGER, 0},
            {"bathrooms", FieldType::INTEGER, 0},
            {"stories", FieldType::INTEGER, 0},
            {"mainroad", FieldType::STRING, 10},
            {"guestroom", FieldType::STRING, 10},
            {"basement", FieldType::STRING, 10},
            {"hotwaterheating", FieldType::STRING, 10},
            {"airconditioning", FieldType::STRING, 10},
            {"parking", FieldType::INTEGER, 0},
            {"prefarea", FieldType::STRING, 10},
            {"furnishingstatus", FieldType::STRING, 20}
        },
        ',',
        "Dataset de viviendas con 13 campos",
        13
    };
    
    datasets["titanic"] = {
        "pasajeros_titanic",
        {
            {"passenger_id", FieldType::INTEGER, 0},
            {"survived", FieldType::INTEGER, 0},
            {"pclass", FieldType::INTEGER, 0},
            {"name", FieldType::STRING, 100},
            {"sex", FieldType::STRING, 10},
            {"age", FieldType::FLOAT, 0},
            {"sibsp", FieldType::INTEGER, 0},
            {"parch", FieldType::INTEGER, 0},
            {"ticket", FieldType::STRING, 30},
            {"fare", FieldType::FLOAT, 0},
            {"cabin", FieldType::STRING, 20},
            {"embarked", FieldType::STRING, 5}
        },
        '\t',
        "Dataset del Titanic con 12 campos",
        12
    };
    
    return datasets;
}

std::vector<std::string> SGBDSystemExtended::parseCSVLine(const std::string& line, char delimiter) {
    std::vector<std::string> values;
    std::string value;
    bool in_quotes = false;
    
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == delimiter && !in_quotes) {
            value.erase(0, value.find_first_not_of(" \t\r"));
            value.erase(value.find_last_not_of(" \t\r") + 1);
            values.push_back(value);
            value.clear();
        } else {
            value += c;
        }
    }
    
    value.erase(0, value.find_first_not_of(" \t\r"));
    value.erase(value.find_last_not_of(" \t\r") + 1);
    if (!value.empty()) {
        values.push_back(value);
    }
    
    return values;
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

size_t SGBDSystemExtended::estimateRecordSize(const std::vector<std::string>& values) {
    size_t size = 0;
    for (const auto& val : values) {
        size += val.length() + 8;
    }
    return size;
}

void SGBDSystemExtended::showDiskStructure(const DiskConfig& config) {
    std::cout << "\n=== ESTRUCTURA DEL DISCO EXTENDIDO ===" << std::endl;
    std::cout << "Configuración:" << std::endl;
    std::cout << "  Platos: " << config.getNumPlatters() << std::endl;
    std::cout << "  Superficies por plato: " << config.getSurfacesPerPlatter() << std::endl;
    std::cout << "  Pistas por superficie: " << config.getTracksPerSurface() << std::endl;
    std::cout << "  Sectores por pista: " << config.getSectorsPerTrack() << std::endl;
    std::cout << "  Bytes por sector: " << config.getBytesPerSector() << std::endl;
    
    std::cout << "\nCapacidades:" << std::endl;
    std::cout << "  Capacidad total: " << config.getFormattedCapacity() << std::endl;
    std::cout << "  Capacidad por bloque: " << config.getBytesPerSector() << " bytes" << std::endl;
    std::cout << "  Total de sectores: " << config.getTotalSectors() << std::endl;
    
    std::cout << "\n🏗️ Arquitectura del Sistema:" << std::endl;
    std::cout << "APLICACIÓN → BUFFER POOL MANAGER → DISK MANAGER → ARCHIVO FÍSICO" << std::endl;
}

bool SGBDSystemExtended::requiresDisk() {
    if (current_state == SystemState::NOT_INITIALIZED) {
        std::cout << "\n❌ ERROR: Operación requiere disco inicializado." << std::endl;
        std::cout << "Ejecuta primero la opción 1 o 2." << std::endl;
        return false;
    }
    return true;
}

bool SGBDSystemExtended::requiresBufferPool() {
    if (current_state != SystemState::BUFFER_POOL_READY) {
        std::cout << "\n❌ ERROR: Operación requiere Buffer Pool inicializado." << std::endl;
        if (current_state == SystemState::DISK_READY) {
            std::cout << "Ejecuta la opción para inicializar Buffer Pool." << std::endl;
        } else {
            std::cout << "Ejecuta primero las opciones 1 (o 2) y luego inicializa Buffer Pool." << std::endl;
        }
        return false;
    }
    return true;
}

// ============================================================================
// IMPLEMENTACIÓN DE MÉTODOS PRINCIPALES
// ============================================================================

void SGBDSystemExtended::showSystemStatus() {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "ESTADO DEL SISTEMA INTEGRADO:" << std::endl;
    
    switch (current_state) {
        case SystemState::NOT_INITIALIZED:
            std::cout << "Estado: NO INICIALIZADO" << std::endl;
            std::cout << "Disco: No creado" << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Acción requerida: Inicializar disco (opción 1)" << std::endl;
            break;
            
        case SystemState::DISK_READY:
            std::cout << "Estado: DISCO LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Acción requerida: Inicializar Buffer Pool" << std::endl;
            break;
            
        case SystemState::BUFFER_POOL_READY:
            std::cout << "Estado: SISTEMA COMPLETO LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: " << buffer_pool_size << " frames" << std::endl;
            std::cout << "Acción: Sistema listo para operaciones avanzadas" << std::endl;
            break;
            
        case SystemState::ERROR_STATE:
            std::cout << "Estado: ERROR" << std::endl;
            std::cout << "Acción requerida: Reinicializar sistema" << std::endl;
            break;
    }
    std::cout << std::string(60, '-') << std::endl;
}

bool SGBDSystemExtended::initializeDisk() {
    std::cout << "\n=== INICIALIZACIÓN DEL DISCO EXTENDIDO ===" << std::endl;
    
    std::string input;
    std::cout << "¿Usar configuración por defecto? (s/n): ";
    std::getline(std::cin, input);
    
    DiskConfig config;
    if (input != "s" && input != "S") {
        int platters, surfaces, tracks, sectors, bytes_sector;
        std::cout << "Número de platos: ";
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
    
    showDiskStructure(config);
    
    if (disk_manager->initialize(config)) {
        current_state = SystemState::DISK_READY;
        std::cout << "\n✅ Disco inicializado exitosamente en: " << disk_path << std::endl;
        std::cout << "📁 Page Directory creado automáticamente por DiskManager" << std::endl;
        return true;
    } else {
        current_state = SystemState::ERROR_STATE;
        std::cout << "\n❌ Error inicializando el disco." << std::endl;
        return false;
    }
}

bool SGBDSystemExtended::loadExistingDisk() {
    std::cout << "\n=== CARGANDO DISCO EXISTENTE EXTENDIDO ===" << std::endl;
    
    if (disk_manager->loadExistingDisk()) {
        current_state = SystemState::DISK_READY;
        std::cout << "✅ Disco cargado desde: " << disk_path << std::endl;
        std::cout << "📁 Page Directory cargado automáticamente" << std::endl;
        return true;
    } else {
        std::cout << "❌ Error: No se encontró disco en " << disk_path << std::endl;
        return false;
    }
}

void SGBDSystemExtended::createTable() {
    if (!requiresDisk()) return;
    
    std::string table_name;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    
    std::cout << "\nTipo de registro:" << std::endl;
    std::cout << "f) Longitud Fija" << std::endl;
    std::cout << "v) Longitud Variable" << std::endl;
    std::cout << "Tipo (f/v): ";
    std::string input;
    std::getline(std::cin, input);
    bool use_fixed = (input == "f" || input == "F");
    
    std::vector<FieldDefinition> schema;
    std::cout << "Número de campos: ";
    int num_fields;
    std::cin >> num_fields;
    std::cin.ignore();
    
    for (int i = 0; i < num_fields; ++i) {
        std::string field_name;
        int type_int;
        size_t max_length = 0;
        
        std::cout << "\nCampo " << (i+1) << ":" << std::endl;
        std::cout << "Nombre: ";
        std::getline(std::cin, field_name);
        
        std::cout << "Tipo (0=INTEGER, 1=FLOAT, 2=STRING, 3=DATE): ";
        std::cin >> type_int;
        
        if (type_int == 2) {
            std::cout << "Longitud máxima: ";
            std::cin >> max_length;
        }
        std::cin.ignore();
        
        FieldType type = static_cast<FieldType>(type_int);
        schema.emplace_back(field_name, type, max_length);
    }
    
    if (disk_manager->createTable(table_name, schema, use_fixed)) {
        std::cout << "\n✅ Tabla '" << table_name << "' creada." << std::endl;
        std::cout << "Tipo: " << (use_fixed ? "Longitud Fija" : "Longitud Variable") << std::endl;
        
        std::cout << "\n📁 Page Directory actualizado automáticamente:" << std::endl;
        disk_manager->displayPageDirectory();
    } else {
        std::cout << "\n❌ Error creando la tabla." << std::endl;
    }
}

void SGBDSystemExtended::insertSingleRecord() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== INSERCIÓN DETALLADA DE REGISTRO ===" << std::endl;
    
    std::string table_name;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    
    std::cout << "Valores separados por comas: ";
    std::string values_str;
    std::getline(std::cin, values_str);
    
    std::vector<std::string> values = parseCSVLine(values_str);
    
    if (disk_manager->insertRecord(table_name, values)) {
        std::cout << "\n✅ Registro insertado exitosamente." << std::endl;
        std::cout << "\n📁 Page Directory actualizado:" << std::endl;
        disk_manager->displayPageDirectory();
    } else {
        std::cout << "\n❌ Error insertando el registro." << std::endl;
    }
}

void SGBDSystemExtended::loadNRecords() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== CARGA DE N REGISTROS ===" << std::endl;
    
    std::string table_name, csv_file;
    int n_records;
    
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    std::cout << "Archivo CSV: ";
    std::getline(std::cin, csv_file);
    std::cout << "Número de registros a cargar: ";
    std::cin >> n_records;
    std::cin.ignore();
    
    std::ifstream file(csv_file);
    if (!file.is_open()) {
        std::cout << "❌ Error: No se pudo abrir " << csv_file << std::endl;
        return;
    }
    
    std::string line;
    int loaded = 0;
    
    while (std::getline(file, line) && loaded < n_records) {
        if (line.empty()) continue;
        
        std::vector<std::string> values = parseCSVLine(line);
        if (!values.empty()) {
            if (disk_manager->insertRecord(table_name, values)) {
                loaded++;
            }
        }
    }
    
    file.close();
    std::cout << "\n✅ Carga completada: " << loaded << " registros procesados." << std::endl;
    
    std::cout << "\n📁 Page Directory final:" << std::endl;
    disk_manager->displayPageDirectory();
}

void SGBDSystemExtended::loadCompleteCSV() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== CARGA COMPLETA DE CSV ===" << std::endl;
    
    std::string table_name, csv_file;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    std::cout << "Archivo CSV: ";
    std::getline(std::cin, csv_file);
    
    int total_records = countRecordsInFile(csv_file);
    if (total_records == 0) {
        std::cout << "❌ Error: Archivo vacío o no encontrado." << std::endl;
        return;
    }
    
    std::cout << "Registros detectados: " << total_records << std::endl;
    
    if (disk_manager->loadFromCSV(table_name, csv_file)) {
        std::cout << "✅ Carga completa exitosa: " << total_records << " registros." << std::endl;
        
        std::cout << "\n📁 Page Directory final:" << std::endl;
        disk_manager->displayPageDirectory();
    } else {
        std::cout << "❌ Error en la carga completa." << std::endl;
    }
}

bool SGBDSystemExtended::loadDataset(const std::string& dataset_name, const std::string& filename) {
    if (!requiresDisk()) return false;
    
    auto datasets = getDatasetSchemas();
    auto it = datasets.find(dataset_name);
    
    if (it == datasets.end()) {
        std::cout << "❌ Dataset " << dataset_name << " no encontrado." << std::endl;
        return false;
    }
    
    const DatasetSchema& schema = it->second;
    
    std::cout << "\n=== CARGANDO DATASET " << dataset_name << " ===" << std::endl;
    std::cout << "Descripción: " << schema.description << std::endl;
    std::cout << "Tabla destino: " << schema.table_name << std::endl;
    
    if (!disk_manager->createTable(schema.table_name, schema.schema, true)) {
        std::cout << "❌ Error creando tabla." << std::endl;
        return false;
    }
    
    std::cout << "✅ Tabla creada con " << schema.expected_fields << " campos." << std::endl;
    
    int total_records = countRecordsInFile(filename);
    std::cout << "📊 Registros a procesar: " << total_records << std::endl;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "❌ Error abriendo archivo " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::getline(file, line); // Saltar header
    
    int loaded = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> values = parseCSVLine(line, schema.delimiter);
        
        if (static_cast<int>(values.size()) > schema.expected_fields) {
            values.resize(schema.expected_fields);
        }
        
        if (static_cast<int>(values.size()) == schema.expected_fields) {
            if (disk_manager->insertRecord(schema.table_name, values)) {
                loaded++;
                if (loaded % 100 == 0) {
                    std::cout << "📈 Procesados: " << loaded << " registros..." << std::endl;
                }
            }
        }
    }
    
    file.close();
    
    std::cout << "\n✅ Carga completada: " << loaded << " registros exitosos." << std::endl;
    
    std::cout << "\n📁 Page Directory actualizado:" << std::endl;
    disk_manager->displayPageDirectory();
    
    return loaded > 0;
}

void SGBDSystemExtended::simulateInsufficientSpace() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== SIMULACIÓN: ESPACIO INSUFICIENTE ===" << std::endl;
    
    std::string table_name;
    std::cout << "Tabla para simulación: ";
    std::getline(std::cin, table_name);
    
    std::cout << "\n🎯 ESCENARIO SIMULADO:" << std::endl;
    std::cout << "- Sector actual: Espacio disponible: 196 bytes" << std::endl;
    std::cout << "- Registro nuevo: 512 bytes" << std::endl;
    std::cout << "\n❌ RESULTADO: Espacio insuficiente (déficit: 316 bytes)" << std::endl;
    std::cout << "\n🔄 SOLUCIÓN: DiskManager busca próximo sector disponible" << std::endl;
    std::cout << "✅ Page Directory actualizado automáticamente" << std::endl;
}

void SGBDSystemExtended::simulateFullSectors() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== SIMULACIÓN: SECTORES LLENOS ===" << std::endl;
    
    std::string table_name;
    std::cout << "Tabla para simulación: ";
    std::getline(std::cin, table_name);
    
    std::cout << "\n🎯 ESCENARIO SIMULADO:" << std::endl;
    std::cout << "❌ RESULTADO: Todos los sectores de la pista están llenos" << std::endl;
    std::cout << "\n🔄 SOLUCIÓN: DiskManager busca siguiente pista disponible" << std::endl;
    std::cout << "✅ Page Directory registra automáticamente el mapeo" << std::endl;
}

void SGBDSystemExtended::findRecord() {
    if (!requiresDisk()) return;
    
    std::string table_name;
    int record_id;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    std::cout << "ID del registro: ";
    std::cin >> record_id;
    std::cin.ignore();
    
    auto record = disk_manager->findRecord(table_name, record_id);
    if (record) {
        std::cout << "\n✅ Registro encontrado:" << std::endl;
        record->display();
    } else {
        std::cout << "\n❌ Registro no encontrado." << std::endl;
    }
}

void SGBDSystemExtended::deleteRecord() {
    if (!requiresDisk()) return;
    
    std::string table_name;
    int record_id;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    std::cout << "ID del registro: ";
    std::cin >> record_id;
    std::cin.ignore();
    
    if (disk_manager->deleteRecord(table_name, record_id)) {
        std::cout << "✅ Registro eliminado exitosamente." << std::endl;
    } else {
        std::cout << "❌ Error eliminando el registro." << std::endl;
    }
}

void SGBDSystemExtended::displayTable() {
    if (!requiresDisk()) return;
    
    std::string table_name;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    
    disk_manager->displayTable(table_name);
}

void SGBDSystemExtended::compactTable() {
    if (!requiresDisk()) return;
    
    std::string table_name;
    std::cout << "Nombre de la tabla: ";
    std::getline(std::cin, table_name);
    
    disk_manager->compactTable(table_name);
}

void SGBDSystemExtended::showStatistics() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== ESTADÍSTICAS DEL SISTEMA INTEGRADO ===" << std::endl;
    disk_manager->displayStatistics();
    
    if (current_state == SystemState::BUFFER_POOL_READY && buffer_manager) {
        std::cout << "\n📊 Estadísticas del Buffer Pool:" << std::endl;
        auto stats = buffer_manager->getStats();
        std::cout << "   - Frames totales: " << stats.total_frames << std::endl;
        std::cout << "   - Frames ocupados: " << stats.occupied_frames << std::endl;
        std::cout << "   - Utilización: " << std::fixed << std::setprecision(1) 
                  << stats.utilization << "%" << std::endl;
    }
}

void SGBDSystemExtended::showDirectoryStructure() {
    if (!requiresDisk()) return;
    disk_manager->showDirectoryStructure();
}

void SGBDSystemExtended::showPageDirectory() {
    if (!requiresDisk()) return;
    
    std::cout << "\n=== PAGE DIRECTORY (GESTIONADO POR DISK MANAGER) ===" << std::endl;
    disk_manager->displayPageDirectory();
}

// ============================================================================
// BUFFER POOL LRU (SIMPLIFICADO)
// ============================================================================

bool SGBDSystemExtended::initializeBufferPool() {
    if (current_state != SystemState::DISK_READY) {
        std::cout << "\n❌ ERROR: Requiere disco inicializado primero." << std::endl;
        return false;
    }
    
    std::cout << "\n=== INICIALIZACIÓN DEL BUFFER POOL LRU ===" << std::endl;
    
    std::string input;
    std::cout << "Tamaño del buffer pool (frames) [" << buffer_pool_size << "]: ";
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        try {
            size_t new_size = std::stoull(input);
            if (new_size > 0 && new_size <= 64) {
                buffer_pool_size = new_size;
            } else {
                std::cout << "⚠️ Tamaño inválido, usando por defecto: " << buffer_pool_size << std::endl;
            }
        } catch (const std::exception&) {
            std::cout << "⚠️ Entrada inválida, usando por defecto: " << buffer_pool_size << std::endl;
        }
    }
    
    try {
        buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
        current_state = SystemState::BUFFER_POOL_READY;
        
        std::cout << "\n🚀 Buffer Pool Manager LRU inicializado exitosamente!" << std::endl;
        std::cout << "   - Pool size: " << buffer_pool_size << " frames" << std::endl;
        std::cout << "   - Algoritmo: LRU (Least Recently Used)" << std::endl;
        std::cout << "   - Page Table: ✓ (En memoria)" << std::endl;
        std::cout << "   - Page Directory: ✓ (Gestionado por DiskManager)" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cout << "❌ Error inicializando Buffer Pool: " << e.what() << std::endl;
        current_state = SystemState::ERROR_STATE;
        return false;
    }
}

void SGBDSystemExtended::bufferPoolPageOperations() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== OPERACIONES DE PÁGINAS CON BUFFER POOL LRU ===" << std::endl;
    
    int page_id;
    std::cout << "ID de página a solicitar: ";
    std::cin >> page_id;
    std::cin.ignore();
    
    std::cout << "\nTipo de operación:" << std::endl;
    std::cout << "r) READ (lectura)" << std::endl;
    std::cout << "w) WRITE (escritura)" << std::endl;
    std::cout << "Seleccionar (r/w): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    PageOperation operation = (input == "w" || input == "W") ? 
        PageOperation::WRITE : PageOperation::READ;
    
    std::cout << "\n🔍 Solicitando página " << page_id 
              << " para " << (operation == PageOperation::READ ? "LECTURA" : "ESCRITURA") 
              << std::endl;
    
    auto block = buffer_manager->requestPage(page_id, operation);
    if (block) {
        std::cout << "\n✅ Página cargada exitosamente!" << std::endl;
        std::cout << "📄 Información del bloque:" << std::endl;
        block->displayInfo();
        
        std::cout << "\n¿Liberar página (unpin)? (s/n): ";
        std::getline(std::cin, input);
        
        if (input == "s" || input == "S") {
            bool mark_dirty = (operation == PageOperation::WRITE);
            buffer_manager->unpinPage(page_id, mark_dirty);
            std::cout << "📍 Página liberada (unpinned)" << std::endl;
        }
    } else {
        std::cout << "❌ Error cargando página" << std::endl;
    }
    
    buffer_manager->displayCompactStatus();
}

void SGBDSystemExtended::createNewPageBuffered() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== CREAR NUEVA PÁGINA CON BUFFER POOL LRU ===" << std::endl;
    
    int new_page_id = buffer_manager->createNewPage();
    if (new_page_id != -1) {
        std::cout << "\n✨ Nueva página creada con ID: " << new_page_id << std::endl;
        
        std::cout << "\n📁 Page Directory actualizado:" << std::endl;
        disk_manager->displayPageDirectory();
        
        buffer_manager->displayCompactStatus();
        
        buffer_manager->unpinPage(new_page_id, true);
    } else {
        std::cout << "❌ Error creando nueva página" << std::endl;
    }
}

void SGBDSystemExtended::showBufferPoolStatus() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== ESTADO COMPLETO DEL BUFFER POOL LRU ===" << std::endl;
    
    // Mostrar estadísticas generales
    auto stats = buffer_manager->getStats();
    std::cout << "\n📊 Estadísticas Generales:" << std::endl;
    std::cout << "- Frames totales: " << stats.total_frames << std::endl;
    std::cout << "- Frames ocupados: " << stats.occupied_frames << std::endl;
    std::cout << "- Frames libres: " << (stats.total_frames - stats.occupied_frames) << std::endl;
    std::cout << "- Páginas dirty: " << stats.dirty_pages << std::endl;
    std::cout << "- Páginas pinned: " << stats.pinned_pages << std::endl;
    std::cout << "- Utilización: " << std::fixed << std::setprecision(1) 
              << stats.utilization << "%" << std::endl;
    
    // Mostrar tabla detallada del Buffer Pool
    std::cout << "\n🔍 Buffer Pool (Frames en Memoria):" << std::endl;
    std::cout << "Frame  PageID  Status    Dirty  PinCount  LoadTime" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    // Usar el método displayBufferPoolInfo existente pero con formato personalizado
    buffer_manager->displayCompactStatus();
    
    std::cout << "\n📈 Estadísticas de Operaciones:" << std::endl;
    std::cout << "- Total operaciones: " << stats.total_operations << std::endl;
    std::cout << "- Page faults: " << stats.page_faults << std::endl;
    std::cout << "- Evictions: " << stats.evictions << std::endl;
    std::cout << "- Hit ratio: " << std::fixed << std::setprecision(1) 
              << (stats.total_operations > 0 ? (1.0 - (double)stats.page_faults / stats.total_operations) * 100 : 0.0) 
              << "%" << std::endl;
}

void SGBDSystemExtended::flushAllPages() {
    if (!requiresBufferPool()) return;
    
    std::cout << "\n=== FLUSH DE TODAS LAS PÁGINAS DIRTY ===" << std::endl;
    buffer_manager->flushAllPages();
    std::cout << "✅ Todas las páginas dirty han sido escritas a disco" << std::endl;
}

// ============================================================================
// CLOCK BUFFER MANAGER (SIMPLIFICADO)
// ============================================================================

void SGBDSystemExtended::initializeClockBufferPool() {
    if (current_state < SystemState::DISK_READY) {
        std::cout << "❌ Error: Primero inicializa el disco (opción 1)" << std::endl;
        return;
    }
    
    size_t clock_pool_size;
    std::cout << "\n🕐 INICIALIZACIÓN BUFFER MANAGER CLOCK PIN-AWARE" << std::endl;
    std::cout << "Tamaño del Clock Buffer Pool (frames): ";
    std::cin >> clock_pool_size;
    std::cin.ignore();
    
    if (clock_pool_size < 2 || clock_pool_size > 20) {
        std::cout << "⚠️ Tamaño recomendado: 2-20 frames. Usando 4." << std::endl;
        clock_pool_size = 4;
    }
    
    try {
        clock_buffer_manager = std::make_unique<BufferManagerClock>(
            clock_pool_size, disk_manager.get());
        
        std::cout << "\n✅ Clock Buffer Manager PIN-AWARE inicializado exitosamente!" << std::endl;
        std::cout << "🕐 Algoritmo Clock activo con " << clock_pool_size << " frames" << std::endl;
        std::cout << "⚡ NUNCA evicta páginas con pin_count > 0" << std::endl;
        std::cout << "🔄 Segunda pasada decrementa pin_count automáticamente" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error inicializando Clock Buffer Manager: " << e.what() << std::endl;
    }
}

void SGBDSystemExtended::clockPageOperations() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Error: Primero inicializa Clock Buffer Manager" << std::endl;
        return;
    }
    
    std::cout << "\n=== OPERACIONES DE PÁGINAS CON BUFFER CLOCK ===" << std::endl;
    
    int page_id;
    std::cout << "ID de página a solicitar: ";
    std::cin >> page_id;
    std::cin.ignore();
    
    std::cout << "\nTipo de operación:" << std::endl;
    std::cout << "r) READ (lectura)" << std::endl;
    std::cout << "w) WRITE (escritura)" << std::endl;
    std::cout << "Seleccionar (r/w): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    bool is_write = (input == "w" || input == "W");
    
    std::cout << "\n🔍 Solicitando página " << page_id 
              << " para " << (is_write ? "ESCRITURA" : "LECTURA") 
              << std::endl;
    
    auto block = clock_buffer_manager->fetchPage(page_id);
    if (block) {
        std::cout << "\n✅ Página cargada exitosamente!" << std::endl;
        std::cout << "📄 Información del bloque:" << std::endl;
        block->displayInfo();
        
        std::cout << "\n¿Liberar página (unpin)? (s/n): ";
        std::getline(std::cin, input);
        
        if (input == "s" || input == "S") {
            clock_buffer_manager->unpinPage(page_id, is_write);
            std::cout << "📍 Página liberada (unpinned)" << std::endl;
        }
    } else {
        std::cout << "❌ Error cargando página" << std::endl;
    }
    
    clock_buffer_manager->displayCompactState();
}

void SGBDSystemExtended::createNewPageClock() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n=== CREAR NUEVA PÁGINA CON CLOCK ===" << std::endl;
    
    int new_page_id;
    auto block = clock_buffer_manager->newPage(new_page_id);
    if (block) {
        std::cout << "✅ Nueva página " << new_page_id << " creada" << std::endl;
        clock_buffer_manager->unpinPage(new_page_id, true);
        clock_buffer_manager->displayCompactState();
    } else {
        std::cout << "❌ Error creando nueva página" << std::endl;
    }
}

void SGBDSystemExtended::showClockBufferStatus() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n🕐 ESTADO COMPLETO CLOCK BUFFER MANAGER PIN-AWARE" << std::endl;
    
    // Mostrar estadísticas generales
    std::cout << "\n📊 Estadísticas Generales:" << std::endl;
    clock_buffer_manager->displayStatistics();
    
    // Mostrar tabla formateada usando el nuevo método
    auto frames_info = clock_buffer_manager->getFramesInfo();
    
    std::cout << "\n🕐 Clock Buffer (Frames en Memoria):" << std::endl;
    std::cout << "Frame  PageID  Status    Dirty  PinCount  RefBit  ClockPos" << std::endl;
    std::cout << std::string(65, '-') << std::endl;
    
    for (size_t i = 0; i < frames_info.size(); ++i) {
        const auto& info = frames_info[i];
        
        std::cout << std::setw(5) << i << "  ";
        std::cout << std::setw(6) << (info.is_free ? "-" : std::to_string(info.page_id)) << "  ";
        std::cout << std::setw(8) << (info.is_free ? "FREE" : "USED") << "  ";
        std::cout << std::setw(5) << (info.is_free ? "-" : (info.is_dirty ? "YES" : "NO")) << "  ";
        std::cout << std::setw(8) << (info.is_free ? "-" : std::to_string(info.pin_count)) << "  ";
        std::cout << std::setw(6) << (info.is_free ? "-" : (info.reference_bit ? "1" : "0")) << "  ";
        std::cout << std::setw(8) << (info.is_clock_hand ? "←HAND" : "") << std::endl;
    }
    
    std::cout << "\n🛡️ Protección PIN-AWARE:" << std::endl;
    std::cout << "✅ NUNCA evicta páginas con pin_count > 0" << std::endl;
    std::cout << "🔄 Segunda pasada decrementa pin_count automáticamente" << std::endl;
    std::cout << "⚡ Algoritmo Clock con reference bits activo" << std::endl;
}

void SGBDSystemExtended::flushAllClockPages() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n💾 Flushing todas las páginas Clock..." << std::endl;
    clock_buffer_manager->flushAllDirtyPages();
    std::cout << "✅ Flush completo!" << std::endl;
}

// ============================================================================
// COMPARACIÓN DE ALGORITMOS
// ============================================================================

void SGBDSystemExtended::compareBufferAlgorithms() {
    if (!buffer_manager || !clock_buffer_manager) {
        std::cout << "❌ Error: Necesitas ambos buffer managers inicializados" << std::endl;
        std::cout << "   - Opción 19: Inicializar BufferPoolManager (LRU)" << std::endl;
        std::cout << "   - Opción 24: Inicializar BufferManagerClock" << std::endl;
        return;
    }
    
    std::cout << "\n⚔️ COMPARACIÓN DETALLADA: LRU vs CLOCK PIN-AWARE" << std::endl;
    std::cout << "=== Análisis de rendimiento y características ===" << std::endl;
    
    // Obtener estadísticas de LRU
    auto lru_stats = buffer_manager->getStats();
    
    std::cout << "\n📊 ESTADÍSTICAS LRU BUFFER MANAGER:" << std::endl;
    std::cout << "- Frames totales: " << lru_stats.total_frames << std::endl;
    std::cout << "- Frames ocupados: " << lru_stats.occupied_frames << std::endl;
    std::cout << "- Utilización: " << std::fixed << std::setprecision(1) << lru_stats.utilization << "%" << std::endl;
    std::cout << "- Total operaciones: " << lru_stats.total_operations << std::endl;
    std::cout << "- Page faults: " << lru_stats.page_faults << std::endl;
    std::cout << "- Evictions: " << lru_stats.evictions << std::endl;
    
    double lru_hit_ratio = lru_stats.total_operations > 0 ? 
        (1.0 - (double)lru_stats.page_faults / lru_stats.total_operations) * 100 : 0.0;
    std::cout << "- Hit ratio: " << std::fixed << std::setprecision(1) << lru_hit_ratio << "%" << std::endl;
    
    std::cout << "\n📊 ESTADÍSTICAS CLOCK BUFFER MANAGER:" << std::endl;
    clock_buffer_manager->displayStatistics();
    
    std::cout << "\n🎯 ANÁLISIS COMPARATIVO:" << std::endl;
    
    std::cout << "\n🔒 MANEJO DE PIN_COUNT:" << std::endl;
    std::cout << "LRU:   Verificación básica en BufferPoolManager" << std::endl;
    std::cout << "Clock: Integrado en algoritmo, NUNCA evicta páginas pinned" << std::endl;
    
    std::cout << "\n⚡ COMPLEJIDAD ALGORÍTMICA:" << std::endl;
    std::cout << "LRU:   O(log n) para acceso + O(n) para eviction" << std::endl;
    std::cout << "Clock: O(1) para acceso + O(n) worst case para eviction" << std::endl;
    
    std::cout << "\n💾 USO DE MEMORIA:" << std::endl;
    std::cout << "LRU:   Timestamps + estructuras de ordenamiento" << std::endl;
    std::cout << "Clock: Solo reference bits + clock hand pointer" << std::endl;
    
    std::cout << "\n🛡️ PROTECCIÓN:" << std::endl;
    std::cout << "LRU:   Puede fallar si no se implementa verificación pin_count" << std::endl;
    std::cout << "Clock: Protección garantizada + auto-regulación" << std::endl;
    
    std::cout << "\n🔄 AUTO-REGULACIÓN:" << std::endl;
    std::cout << "LRU:   No tiene mecanismo de segunda pasada" << std::endl;
    std::cout << "Clock: Segunda pasada decrementa pins automáticamente" << std::endl;
    
    std::cout << "\n🌊 RESISTENCIA A SEQUENTIAL FLOODING:" << std::endl;
    std::cout << "LRU:   Vulnerable (páginas recientes evictan frecuentes)" << std::endl;
    std::cout << "Clock: Resistente (reference bits dan segunda oportunidad)" << std::endl;
    
    // Recomendación
    std::cout << "\n🏆 RECOMENDACIÓN:" << std::endl;
    std::cout << "✅ Clock es SUPERIOR para sistemas con:" << std::endl;
    std::cout << "   • Páginas que deben permanecer pinned" << std::endl;
    std::cout << "   • Patrones de acceso secuencial" << std::endl;
    std::cout << "   • Necesidad de protección robusta" << std::endl;
    std::cout << "   • Sistemas con memoria limitada" << std::endl;
    
    std::cout << "\n✅ LRU es superior para:" << std::endl;
    std::cout << "   • Workloads con fuerte localidad temporal" << std::endl;
    std::cout << "   • Cuando se necesita precisión en reemplazo" << std::endl;
    std::cout << "   • Sistemas con suficiente memoria para metadata" << std::endl;
}

// ============================================================================
// MENÚ PRINCIPAL SIMPLIFICADO
// ============================================================================

void showMenu() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SGBD FÍSICO INTEGRADO - MENÚ PRINCIPAL" << std::endl;
    std::cout << "Sistema con Buffer Pool Management Simplificado" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n🚀 INICIALIZACIÓN DEL SISTEMA:" << std::endl;
    std::cout << "1.  Inicializar nuevo disco extendido" << std::endl;
    std::cout << "2.  Cargar disco existente extendido" << std::endl;
    std::cout << "3.  Ver estado del sistema integrado" << std::endl;
    
    std::cout << "\n🗂️ GESTIÓN DE TABLAS:" << std::endl;
    std::cout << "4.  Crear tabla (longitud fija/variable)" << std::endl;
    
    std::cout << "\n📊 INSERCIÓN DE DATOS:" << std::endl;
    std::cout << "5.  Insertar 1 registro" << std::endl;
    std::cout << "6.  Cargar N registros desde CSV" << std::endl;
    std::cout << "7.  Cargar CSV completo" << std::endl;
    
    std::cout << "\n📋 DATASETS PREDEFINIDOS:" << std::endl;
    std::cout << "8.  Cargar dataset Housing (545 registros)" << std::endl;
    std::cout << "9.  Cargar dataset Titanic (891 registros)" << std::endl;
    
    std::cout << "\n🎯 SIMULACIONES:" << std::endl;
    std::cout << "10. Simular sector sin espacio suficiente" << std::endl;
    std::cout << "11. Simular sectores llenos" << std::endl;
    
    std::cout << "\n🔍 CONSULTAS Y OPERACIONES:" << std::endl;
    std::cout << "12. Buscar registro por ID" << std::endl;
    std::cout << "13. Eliminar registro" << std::endl;
    std::cout << "14. Mostrar tabla completa" << std::endl;
    std::cout << "15. Compactar tabla" << std::endl;
    
    std::cout << "\n📈 INFORMACIÓN DEL SISTEMA:" << std::endl;
    std::cout << "16. Mostrar estadísticas integradas" << std::endl;
    std::cout << "17. Mostrar estructura de directorios" << std::endl;
    std::cout << "18. Mostrar Page Directory" << std::endl;

    std::cout << "\n🏊 BUFFER POOL LRU:" << std::endl;
    std::cout << "19. Inicializar Buffer Pool Manager (LRU)" << std::endl;
    std::cout << "20. Operaciones de páginas (READ/WRITE)" << std::endl;
    std::cout << "21. Crear nueva página con Buffer Pool" << std::endl;
    std::cout << "22. Ver estado del Buffer Pool" << std::endl;
    std::cout << "23. Flush todas las páginas dirty" << std::endl;
    
    std::cout << "\n🕐 BUFFER CLOCK PIN-AWARE:" << std::endl;
    std::cout << "24. Inicializar Clock Buffer Manager" << std::endl;
    std::cout << "25. Operaciones de páginas Clock" << std::endl;
    std::cout << "26. Crear nueva página Clock" << std::endl;
    std::cout << "27. Ver estado Clock Buffer" << std::endl;
    std::cout << "28. Flush páginas Clock" << std::endl;
    
    std::cout << "\n⚔️ COMPARACIÓN DE ALGORITMOS:" << std::endl;
    std::cout << "29. Comparar LRU vs Clock (Análisis detallado)" << std::endl;
    
    std::cout << "\n0.  Salir" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "Opción: ";
}

/**
 * @brief Función principal con sistema limpio y modularizado
 */
int main() {
    SGBDSystemExtended sistema;
    int option;
    
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "SISTEMA DE GESTIÓN DE BASE DE DATOS FÍSICO INTEGRADO" << std::endl;
    std::cout << "🚀 Buffer Pool Management Simplificado" << std::endl;
    std::cout << "📚 Implementación Educativa - Almacenamiento Secundario" << std::endl;
    std::cout << "🎓 Basado en Database System Implementation + CMU Lectures" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    sistema.showSystemStatus();
    
    while (true) {
        showMenu();
        std::cin >> option;
        std::cin.ignore();
        
        switch (option) {
            // INICIALIZACIÓN
            case 1: sistema.initializeDisk(); break;
            case 2: sistema.loadExistingDisk(); break;
            case 3: sistema.showSystemStatus(); break;
            
            // GESTIÓN DE TABLAS
            case 4: sistema.createTable(); break;
            
            // INSERCIÓN DE DATOS
            case 5: sistema.insertSingleRecord(); break;
            case 6: sistema.loadNRecords(); break;
            case 7: sistema.loadCompleteCSV(); break;
            
            // DATASETS
            case 8: 
                {
                    std::string housing_path = "../data/Housing.csv";
                    if (sistema.loadDataset("housing", housing_path)) {
                        std::cout << "✅ Dataset Housing cargado desde: " << housing_path << std::endl;
                    } else {
                        std::cout << "❌ Error: Verifica que existe " << housing_path << std::endl;
                    }
                }
                break;
                
            case 9:
                {
                    std::string titanic_path = "../data/titanic.csv";
                    if (sistema.loadDataset("titanic", titanic_path)) {
                        std::cout << "✅ Dataset Titanic cargado desde: " << titanic_path << std::endl;
                    } else {
                        std::cout << "❌ Error: Verifica que existe " << titanic_path << std::endl;
                    }
                }
                break;
            
            // SIMULACIONES
            case 10: sistema.simulateInsufficientSpace(); break;
            case 11: sistema.simulateFullSectors(); break;
            
            // CONSULTAS Y OPERACIONES
            case 12: sistema.findRecord(); break;
            case 13: sistema.deleteRecord(); break;
            case 14: sistema.displayTable(); break;
            case 15: sistema.compactTable(); break;
            
            // INFORMACIÓN DEL SISTEMA
            case 16: sistema.showStatistics(); break;
            case 17: sistema.showDirectoryStructure(); break;
            case 18: sistema.showPageDirectory(); break;
            
            // BUFFER POOL LRU
            case 19: sistema.initializeBufferPool(); break;
            case 20: sistema.bufferPoolPageOperations(); break;
            case 21: sistema.createNewPageBuffered(); break;
            case 22: sistema.showBufferPoolStatus(); break;
            case 23: sistema.flushAllPages(); break;
            
            // CLOCK BUFFER MANAGER
            case 24: sistema.initializeClockBufferPool(); break;
            case 25: sistema.clockPageOperations(); break;
            case 26: sistema.createNewPageClock(); break;
            case 27: sistema.showClockBufferStatus(); break;
            case 28: sistema.flushAllClockPages(); break;
            
            // COMPARACIÓN
            case 29: sistema.compareBufferAlgorithms(); break;
                
            case 0:
                std::cout << "\n🎓 ¡Gracias por usar el SGBD Físico Integrado!" << std::endl;
                std::cout << "📚 Has experimentado con:" << std::endl;
                std::cout << "   ✅ Buffer Pool Management profesional" << std::endl;
                std::cout << "   ✅ Page Directory persistente" << std::endl;
                std::cout << "   ✅ Algoritmos LRU y Clock" << std::endl;
                std::cout << "   ✅ Comparación de rendimiento" << std::endl;
                std::cout << "🚀 ¡Sistema de base de datos de nivel profesional!" << std::endl;
                return 0;
                
            default:
                std::cout << "\n❌ Opción no válida. Selecciona 0-29." << std::endl;
                break;
        }
        
        std::cout << "\n⏸️ Presiona Enter para continuar...";
        std::cin.get();
    }
    
    return 0;
}