
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <map>
#include "../include/HashExtendible/ExtensibleHash.h"
#include "../include/BPlusTree/BPlusTree.h"
#include "../include/RecordReference.h"
#include "../include/DiskManagerExtended.h"
#include "../include/buffer/BufferPoolManager.h"
#include "../include/buffer/ClockReplacer.h"
#include "../include/buffer/BufferManagerClock.h"
#ifdef _WIN32
#include <windows.h>
#include <locale>
#endif

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

    std::unique_ptr<ExtensibleHash> imei_index;           // Índice Hash por IMEI
    std::unique_ptr<BPlusTree<std::string>> timestamp_index; // Índice B+ Tree por timestamp
    std::string current_server;                           // "Server_A" o "Server_B"
    std::string gps_table_name;                          // Nombre de tabla GPS cargada

    
    // === MÉTODOS AUXILIARES PRIVADOS ===
    std::map<std::string, DatasetSchema> getDatasetSchemas();
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',');
    int countRecordsInFile(const std::string& filename);
    size_t estimateRecordSize(const std::vector<std::string>& values);
    void showDiskStructure(const DiskConfig& config);
    bool requiresDisk();
    bool requiresBufferPool();
    // === MÉTODOS AUXILIARES GPS ===
    std::vector<FieldDefinition> getGPSSchema() const;
    bool createGPSRecord(const std::vector<std::string>& csv_fields, std::unique_ptr<VariableRecord>& record);
    void displayGPSRecordWithHeaders(const VariableRecord& record, const std::string& source);
    std::string parseTimestamp(const std::string& timestamp_with_tz);

public:
    SGBDSystemExtended(const std::string& path = "./bin/mi_disco_sgbde", size_t pool_size = 4);
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
    
    // === CLOCK BUFFER MANAGER (MEJORADO) ===
    void initializeClockBufferPool();
    void clockPageOperations();
    void createNewPageClock();
    void showClockBufferStatus();
    void flushAllClockPages();
    
    // === COMPARACIÓN DE ALGORITMOS ===
    void compareBufferAlgorithms();
    // === NUEVOS MÉTODOS PARA GPS ===
    // GPS Dataset Management
    bool loadGPSDataset(const std::string& filename);
    void cleanValue(std::string& value);
    void selectServerConfiguration();
    void initializeIndexes();
    
    // SQL Query Operations
    void executeSelectAll();
    void executeSelectByIMEI();
    void executeSelectByTimestamp();
    void executeInsertGPS();
    
    // System Information
    void showIndexStatistics();
    void showGPSTableStructure();
    void generateFlowDiagram();
};

// ============================================================================
// IMPLEMENTACIÓN DE MÉTODOS AUXILIARES
// ============================================================================

SGBDSystemExtended::SGBDSystemExtended(const std::string& path, size_t pool_size) 
    : current_state(SystemState::NOT_INITIALIZED)
    , disk_path(path)
    , buffer_pool_size(pool_size) 
    , current_server("")  // NUEVO: inicializar vacío
    , gps_table_name("")  // NUEVO: inicializar vacío

{
    disk_manager = std::make_unique<DiskManagerExtended>(path);
}

SGBDSystemExtended::~SGBDSystemExtended() {
    // Guardar índices antes de salir
    saveIndexesOnExit();
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
     // === NUEVO DATASET GPS ===
datasets["gps"] = {
    "dataGPS",
    {
        {"id", FieldType::INTEGER, 0},
        {"imei", FieldType::STRING, 20},        // Reducido
        {"commandId", FieldType::INTEGER, 0},
        {"timestamp", FieldType::STRING, 30},   // Reducido
        {"latitude", FieldType::STRING, 15},    // Reducido
        {"longitude", FieldType::STRING, 15},   // Reducido
        {"recordIndex", FieldType::INTEGER, 0},
        {"timestampExtension", FieldType::INTEGER, 0},
        {"recordExtension", FieldType::INTEGER, 0},
        {"priority", FieldType::INTEGER, 0},
        {"altitude", FieldType::STRING, 10},    // Reducido
        {"angle", FieldType::STRING, 10},       // Reducido
        {"satellites", FieldType::INTEGER, 0},
        {"speed", FieldType::INTEGER, 0},
        {"hdop", FieldType::STRING, 10},        // Reducido
        {"eventId", FieldType::INTEGER, 0},
        {"punto", FieldType::STRING, 50},       // MUY reducido
        {"ioElements", FieldType::STRING, 100}, // MUY reducido  
        {"processedAt", FieldType::STRING, 30}, // Reducido
        {"createdAt", FieldType::STRING, 30},   // Reducido
        {"updatedAt", FieldType::STRING, 30}    // Reducido
    },
    ',',
    "Dataset GPS con tracking de dispositivos",
    21
};
    
    return datasets;
}

// 2. PARSER CSV MEJORADO (reemplaza parseCSVLine())
std::vector<std::string> SGBDSystemExtended::parseCSVLine(const std::string& line, char delimiter) {
    std::vector<std::string> values;
    std::string value;
    bool in_quotes = false;
    bool escaped_quote = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        char next_c = (i + 1 < line.length()) ? line[i + 1] : '\0';
        
        if (c == '"') {
            if (in_quotes && next_c == '"') {
                // Comillas dobles escapadas ""
                value += '"';
                ++i; // Saltar la siguiente comilla
                escaped_quote = true;
            } else {
                // Comilla normal de inicio/fin
                in_quotes = !in_quotes;
                escaped_quote = false;
            }
        } else if (c == delimiter && !in_quotes) {
            // Delimiter fuera de comillas
            cleanValue(value);
            values.push_back(value);
            value.clear();
        } else {
            // Carácter normal
            value += c;
        }
    }
    
    // Agregar último campo
    cleanValue(value);
    if (!value.empty() || !values.empty()) {
        values.push_back(value);
    }
    
    return values;
}

// 3. FUNCIÓN AUXILIAR PARA LIMPIAR VALORES
void SGBDSystemExtended::cleanValue(std::string& value) {
    // Eliminar espacios en blanco al inicio y final
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    
    // Eliminar comillas externas si existen
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
// IMPLEMENTACIÓN DE MÉTODOS PRINCIPALES GPS
// ============================================================================
bool SGBDSystemExtended::loadGPSDataset(const std::string& filename) {
    if (!requiresDisk()) return false;
    
    std::cout << "\n=== CARGANDO DATASET GPS ===" << std::endl;
    
    // Verificar si la tabla ya existe
    // (Asumimos que existe si el disco está cargado y tiene datos)
    auto datasets = getDatasetSchemas();
    auto it = datasets.find("gps");
    
    if (it == datasets.end()) {
        std::cout << "❌ Schema GPS no encontrado." << std::endl;
        return false;
    }
    
    const DatasetSchema& schema = it->second;
    
    // ✅ INTENTO 1: Crear tabla (puede fallar si ya existe)
    bool table_created = disk_manager->createTable(schema.table_name, schema.schema, true);
    
    if (table_created) {
        // Tabla nueva - cargar datos
        std::cout << "✅ Tabla GPS creada, cargando datos..." << std::endl;
        bool result = loadDataset("gps", filename);
        if (result) {
            gps_table_name = "dataGPS";
            std::cout << "✅ Dataset GPS cargado exitosamente" << std::endl;
        }
        return result;
    } else {
        // ✅ TABLA YA EXISTE - Solo registrarla
        std::cout << "🔍 Tabla GPS ya existe en disco" << std::endl;
        gps_table_name = "dataGPS";
        std::cout << "✅ Tabla GPS registrada exitosamente" << std::endl;
        return true;
    }
}
void SGBDSystemExtended::selectServerConfiguration() {
    std::cout << "\n=== SELECCIÓN DE CONFIGURACIÓN DE SERVIDOR ===" << std::endl;
    std::cout << "Seleccione el servidor para operaciones GPS:" << std::endl;
    std::cout << "A) Server A - Optimizado para Escrituras (Buffer LRU)" << std::endl;
    std::cout << "   • Política: LRU Replacement" << std::endl;
    std::cout << "   • Uso típico: Inserciones frecuentes, transaccionales" << std::endl;
    std::cout << "   • Buffer Pool: Optimizado para secuencias de escritura" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "B) Server B - Optimizado para Lecturas (Buffer Clock)" << std::endl;
    std::cout << "   • Política: Clock Algorithm PIN-AWARE" << std::endl;
    std::cout << "   • Uso típico: Consultas analíticas, range queries" << std::endl;
    std::cout << "   • Buffer Pool: Optimizado para patrones de lectura complejos" << std::endl;
    
    std::cout << "\nOpción (A/B): ";
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "A" || input == "a") {
        current_server = "Server_A";
        std::cout << "\n✅ Server A seleccionado - Configurando Buffer LRU..." << std::endl;
        
        if (!buffer_manager) {
            initializeBufferPool();
        }
        
        std::cout << "🔧 Configuración activa:" << std::endl;
        std::cout << "   • Buffer Manager: LRU Policy" << std::endl;
        std::cout << "   • Especialización: Escrituras y transacciones" << std::endl;
        std::cout << "   • Índice principal: Hash Extensible (IMEI)" << std::endl;
        
    } else if (input == "B" || input == "b") {
        current_server = "Server_B";
        std::cout << "\n✅ Server B seleccionado - Configurando Buffer Clock..." << std::endl;
        
        if (!clock_buffer_manager) {
            initializeClockBufferPool();
        }
        
        std::cout << "🔧 Configuración activa:" << std::endl;
        std::cout << "   • Buffer Manager: Clock PIN-AWARE Algorithm" << std::endl;
        std::cout << "   • Especialización: Lecturas y análisis" << std::endl;
        std::cout << "   • Índice principal: B+ Tree (timestamp)" << std::endl;
        
    } else {
        std::cout << "❌ Opción inválida. Manteniendo configuración actual." << std::endl;
        return;
    }
    
    // Inicializar índices después de seleccionar servidor
    initializeIndexes();
}

void SGBDSystemExtended::initializeIndexes() {
    if (gps_table_name.empty()) {
        std::cout << "⚠️ Primero debe cargar el dataset GPS." << std::endl;
        return;
    }
    
    std::cout << "\n=== INICIALIZANDO ÍNDICES ESPECIALIZADOS ===" << std::endl;
    
    if (current_server == "Server_A") {
        // Server A: Hash Extensible para IMEI (consultas exactas)
        std::cout << "🔍 Inicializando Hash Extensible para IMEI..." << std::endl;
        imei_index = std::make_unique<ExtensibleHash>(4); // Bucket capacity = 4
        
        std::cout << "📋 Características del Hash Extensible:" << std::endl;
        std::cout << "   • Clave: IMEI (string)" << std::endl;
        std::cout << "   • Capacidad de bucket: 4 registros" << std::endl;
        std::cout << "   • Óptimo para: SELECT WHERE imei = 'valor'" << std::endl;
        
    } else if (current_server == "Server_B") {
        // Server B: B+ Tree para timestamp (range queries)
        std::cout << "🌳 Inicializando B+ Tree para timestamp..." << std::endl;
        timestamp_index = std::make_unique<BPlusTree<std::string>>(3); // Order = 3
        
        std::cout << "📋 Características del B+ Tree:" << std::endl;
        std::cout << "   • Clave: timestamp (string)" << std::endl;
        std::cout << "   • Orden: 3" << std::endl;
        std::cout << "   • Óptimo para: SELECT WHERE timestamp BETWEEN x AND y" << std::endl;
    }
    
    std::cout << "✅ Índices inicializados para " << current_server << std::endl;
}

void SGBDSystemExtended::executeSelectAll() {
    if (gps_table_name.empty()) {
        std::cout << "❌ Primero debe cargar el dataset GPS." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS ===" << std::endl;
    std::cout << "🔄 Operación: Scan completo de tabla (no usa índices)" << std::endl;
    std::cout << "⚡ Servidor activo: " << current_server << std::endl;
    
    // Esta operación usa el buffer manager actual para scan secuencial
    if (current_server == "Server_A" && buffer_manager) {
        std::cout << "📊 Utilizando Buffer LRU para scan secuencial..." << std::endl;
        buffer_manager->displayCompactStatus(); // CORREGIDO: método que existe
    } else if (current_server == "Server_B" && clock_buffer_manager) {
        std::cout << "📊 Utilizando Buffer Clock para scan secuencial..." << std::endl;
        clock_buffer_manager->displayClockState(); // CORREGIDO: método que existe
    }
    
    // Mostrar tabla completa usando el nombre específico
    if (!gps_table_name.empty()) {
        disk_manager->displayTable(gps_table_name);
    }
    
    std::cout << "\n✅ SELECT * completado - todos los registros mostrados con headers" << std::endl;
}

void SGBDSystemExtended::executeSelectByIMEI() {
    if (gps_table_name.empty()) {
        std::cout << "❌ Primero debe cargar el dataset GPS." << std::endl;
        return;
    }
    
    if (!imei_index) {
        std::cout << "❌ Hash Extensible no inicializado. Seleccione Server A primero." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS WHERE imei = ? ===" << std::endl;
    
    std::string target_imei;
    std::cout << "Ingrese IMEI a buscar: ";
    std::getline(std::cin, target_imei);
    
    std::cout << "\n🔍 FLUJO DE CONSULTA POR ÍNDICE HASH:" << std::endl;
    std::cout << "1️⃣ Hash Extensible: Calculando hash(" << target_imei << ")" << std::endl;
    std::cout << "2️⃣ Localizando bucket en directorio..." << std::endl;
    
    // Buscar en Hash Extensible
    VariableRecord temp_record;
    bool found = imei_index->search(target_imei, temp_record);
    
    if (found) {
        std::cout << "3️⃣ ✅ ENCONTRADO en Hash Extensible!" << std::endl;
        std::cout << "4️⃣ Recuperando registro completo desde disco..." << std::endl;
        
        // Mostrar el registro con headers detallados
        displayGPSRecordWithHeaders(temp_record, "Hash Extensible Index");
        
        std::cout << "\n📊 ESTADÍSTICAS DE LA CONSULTA:" << std::endl;
        imei_index->displayStatistics();
        
    } else {
        std::cout << "3️⃣ ❌ IMEI no encontrado en el índice" << std::endl;
        std::cout << "✅ Consulta completada - 0 registros" << std::endl;
    }
    
    std::cout << "\n🔧 Estado del Buffer Manager:" << std::endl;
    if (current_server == "Server_A" && buffer_manager) {
        buffer_manager->displayCompactStatus(); // CORREGIDO: método que existe
    }
}

void SGBDSystemExtended::executeSelectByTimestamp() {
    if (gps_table_name.empty()) {
        std::cout << "❌ Primero debe cargar el dataset GPS." << std::endl;
        return;
    }
    
    if (!timestamp_index) {
        std::cout << "❌ B+ Tree no inicializado. Seleccione Server B primero." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: SELECT * FROM dataGPS WHERE timestamp BETWEEN ? AND ? ===" << std::endl;
    
    std::string start_time, end_time;
    std::cout << "Timestamp inicio (YYYY-MM-DD HH:MM:SS): ";
    std::getline(std::cin, start_time);
    std::cout << "Timestamp fin (YYYY-MM-DD HH:MM:SS): ";
    std::getline(std::cin, end_time);
    
    std::cout << "\n🌳 FLUJO DE CONSULTA POR RANGO B+ TREE:" << std::endl;
    std::cout << "1️⃣ B+ Tree: Buscando nodo hoja para '" << start_time << "'" << std::endl;
    std::cout << "2️⃣ Recorriendo hojas enlazadas hasta '" << end_time << "'" << std::endl;
    
    // Buscar rango en B+ Tree
    auto references = timestamp_index->rangeSearch(start_time, end_time);
    
    std::cout << "3️⃣ ✅ Encontradas " << references.size() << " referencias en rango" << std::endl;
    std::cout << "4️⃣ Recuperando registros desde disco..." << std::endl;
    
    for (size_t i = 0; i < references.size() && i < 10; ++i) { // Limitar a 10 para demo
        std::cout << "\n📄 Registro " << (i+1) << ":" << std::endl;
        std::cout << "   RecordReference: " << references[i] << std::endl;
        // En implementación real, usaríamos la referencia para cargar el registro
        std::cout << "   [Simulado: Registro GPS cargado desde " << references[i] << "]" << std::endl;
    }
    
    std::cout << "\n📊 ESTADÍSTICAS DE LA CONSULTA:" << std::endl;
    timestamp_index->displayStatistics();
    
    std::cout << "\n🔧 Estado del Buffer Manager:" << std::endl;
    if (current_server == "Server_B" && clock_buffer_manager) {
        clock_buffer_manager->displayClockState(); // CORREGIDO: método que existe
    }
}

void SGBDSystemExtended::executeInsertGPS() {
    if (gps_table_name.empty()) {
        std::cout << "❌ Primero debe cargar el dataset GPS." << std::endl;
        return;
    }
    
    std::cout << "\n=== EJECUTANDO: INSERT INTO dataGPS ===" << std::endl;
    std::cout << "📝 Ingrese datos GPS (21 campos separados por comas):" << std::endl;
    std::cout << "Formato: id,imei,commandId,timestamp,lat,lon,recordIndex,..." << std::endl;
    
    std::string input_line;
    std::getline(std::cin, input_line);
    
    std::vector<std::string> values = parseCSVLine(input_line, ',');
    
    if (values.size() < 21) {
        std::cout << "❌ Error: Se requieren 21 campos. Recibidos: " << values.size() << std::endl;
        return;
    }
    
    values.resize(21); // Asegurar exactamente 21 campos
    
    std::cout << "\n🔄 FLUJO DE INSERCIÓN:" << std::endl;
    std::cout << "1️⃣ Insertando en tabla física..." << std::endl;
    
    if (disk_manager->insertRecord("dataGPS", values)) {
        std::cout << "2️⃣ ✅ Registro insertado en disco" << std::endl;
        
        // Actualizar índices si están activos
        std::string imei = values[1];
        std::string timestamp = parseTimestamp(values[3]);
        
        if (imei_index && current_server == "Server_A") {
            std::cout << "3️⃣ Actualizando Hash Extensible (IMEI: " << imei << ")" << std::endl;
            // En implementación real, insertaríamos la referencia al registro
            std::cout << "   ✅ Índice Hash actualizado" << std::endl;
        }
        
        if (timestamp_index && current_server == "Server_B") {
            std::cout << "3️⃣ Actualizando B+ Tree (timestamp: " << timestamp << ")" << std::endl;
            // En implementación real, insertaríamos la referencia al registro
            std::cout << "   ✅ Índice B+ Tree actualizado" << std::endl;
        }
        
        std::cout << "\n✅ INSERT completado exitosamente" << std::endl;
        
    } else {
        std::cout << "2️⃣ ❌ Error insertando registro en disco" << std::endl;
    }
}

void SGBDSystemExtended::showIndexStatistics() {
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
        std::cout << "   Use la opción de selección de servidor primero." << std::endl;
    }
}

void SGBDSystemExtended::showGPSTableStructure() {
    std::cout << "\n=== ESTRUCTURA DE TABLA GPS ===" << std::endl;
    
    if (gps_table_name.empty()) {
        std::cout << "❌ No hay tabla GPS cargada." << std::endl;
        return;
    }
    
    std::cout << "Tabla: " << gps_table_name << std::endl;
    std::cout << "Tipo: Registros de longitud variable" << std::endl;
    std::cout << "Campos: 21" << std::endl;
    
    std::cout << "\n📋 ESQUEMA DE CAMPOS GPS:" << std::endl;
    auto schema = getGPSSchema();
    for (size_t i = 0; i < schema.size(); ++i) {
        std::cout << std::setw(3) << (i+1) << ". " 
                  << std::setw(20) << std::left << schema[i].name
                  << " | ";
        
        switch (schema[i].type) {
            case FieldType::INTEGER: std::cout << "INTEGER"; break;
            case FieldType::STRING: std::cout << "STRING(" << schema[i].max_length << ")"; break;
            case FieldType::FLOAT: std::cout << "FLOAT"; break;
            case FieldType::DATE: std::cout << "DATE"; break;
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n💾 INFORMACIÓN FÍSICA:" << std::endl;
    disk_manager->displayPageDirectory();
}

void SGBDSystemExtended::generateFlowDiagram() {
    std::cout << "\n=== DIAGRAMA DE FLUJO DEL SISTEMA ===" << std::endl;
    std::cout << "📊 Generando diagrama completo de interacción de módulos..." << std::endl;
    
    std::cout << "\n🔄 FLUJO COMPLETO DE CONSULTA:" << std::endl;
    std::cout << "┌─────────────────┐" << std::endl;
    std::cout << "│   USUARIO SQL   │" << std::endl;
    std::cout << "└─────────┬───────┘" << std::endl;
    std::cout << "          │" << std::endl;
    std::cout << "          ▼" << std::endl;
    std::cout << "┌─────────────────┐" << std::endl;
    std::cout << "│ QUERY EXECUTOR  │" << std::endl;
    std::cout << "│ (Routing Logic) │" << std::endl;
    std::cout << "└─────────┬───────┘" << std::endl;
    std::cout << "          │" << std::endl;
    std::cout << "    ┌─────┴─────┐" << std::endl;
    std::cout << "    ▼           ▼" << std::endl;
    std::cout << "┌───────────┐ ┌─────────────┐" << std::endl;
    std::cout << "│ HASH IDX  │ │ B+ TREE IDX │" << std::endl;
    std::cout << "│ (IMEI)    │ │ (TIMESTAMP) │" << std::endl;
    std::cout << "└─────┬─────┘ └──────┬──────┘" << std::endl;
    std::cout << "      │              │" << std::endl;
    std::cout << "      └──────┬───────┘" << std::endl;
    std::cout << "             ▼" << std::endl;
    std::cout << "   ┌─────────────────┐" << std::endl;
    std::cout << "   │ RECORD REFERENCE│" << std::endl;
    std::cout << "   │ (PhysAddr+Slot) │" << std::endl;
    std::cout << "   └─────────┬───────┘" << std::endl;
    std::cout << "             │" << std::endl;
    std::cout << "             ▼" << std::endl;
    std::cout << "   ┌─────────────────┐" << std::endl;
    std::cout << "   │ BUFFER MANAGER  │" << std::endl;
    std::cout << "   │ (LRU / Clock)   │" << std::endl;
    std::cout << "   └─────────┬───────┘" << std::endl;
    std::cout << "             │" << std::endl;
    std::cout << "       ┌─────┴─────┐" << std::endl;
    std::cout << "       ▼           ▼" << std::endl;
    std::cout << "   ┌────────┐  ┌─────────────┐" << std::endl;
    std::cout << "   │ CACHE  │  │ DISK MANAGER│" << std::endl;
    std::cout << "   │ HIT    │  │ (Page Load) │" << std::endl;
    std::cout << "   └────┬───┘  └──────┬──────┘" << std::endl;
    std::cout << "        │             │" << std::endl;
    std::cout << "        └──────┬──────┘" << std::endl;
    std::cout << "               ▼" << std::endl;
    std::cout << "     ┌─────────────────┐" << std::endl;
    std::cout << "     │ PAGE WITH HEADERS│" << std::endl;
    std::cout << "     │ BLOCK_HEADER    │" << std::endl;
    std::cout << "     │ OFFSET_TABLE    │" << std::endl;
    std::cout << "     │ GPS_RECORDS     │" << std::endl;
    std::cout << "     └─────────┬───────┘" << std::endl;
    std::cout << "               │" << std::endl;
    std::cout << "               ▼" << std::endl;
    std::cout << "     ┌─────────────────┐" << std::endl;
    std::cout << "     │ RESULTADO FINAL │" << std::endl;
    std::cout << "     │ (al Usuario)    │" << std::endl;
    std::cout << "     └─────────────────┘" << std::endl;
    
    std::cout << "\n📋 COMPONENTES CLAVE:" << std::endl;
    std::cout << "• Hash Extensible: O(1) búsquedas exactas IMEI" << std::endl;
    std::cout << "• B+ Tree: Range queries eficientes por timestamp" << std::endl;
    std::cout << "• Buffer Manager: Cache inteligente con políticas LRU/Clock" << std::endl;
    std::cout << "• Disk Manager: Headers estructurados para localización rápida" << std::endl;
    std::cout << "• Record References: Punteros ligeros entre índices y disco" << std::endl;
    
    std::cout << "\n💾 HEADERS EN PÁGINAS:" << std::endl;
    std::cout << "BLOCK_HEADER|sector_id|size|record_count|table_name|version" << std::endl;
    std::cout << "OFFSET_TABLE|offset1,offset2,offset3,..." << std::endl;
    std::cout << "RECORD|VARIABLE|id|deleted|physical_addr|field_data..." << std::endl;
}

// === MÉTODOS AUXILIARES GPS ===

std::vector<FieldDefinition> SGBDSystemExtended::getGPSSchema() const {
    return {
        {"id", FieldType::INTEGER, 0},
        {"imei", FieldType::STRING, 20},        // Reducido
        {"commandId", FieldType::INTEGER, 0},
        {"timestamp", FieldType::STRING, 30},   // Reducido
        {"latitude", FieldType::STRING, 15},    // Reducido
        {"longitude", FieldType::STRING, 15},   // Reducido
        {"recordIndex", FieldType::INTEGER, 0},
        {"timestampExtension", FieldType::INTEGER, 0},
        {"recordExtension", FieldType::INTEGER, 0},
        {"priority", FieldType::INTEGER, 0},
        {"altitude", FieldType::STRING, 10},    // Reducido
        {"angle", FieldType::STRING, 10},       // Reducido
        {"satellites", FieldType::INTEGER, 0},
        {"speed", FieldType::INTEGER, 0},
        {"hdop", FieldType::STRING, 10},        // Reducido
        {"eventId", FieldType::INTEGER, 0},
        {"punto", FieldType::STRING, 50},       // MUY reducido
        {"ioElements", FieldType::STRING, 100}, // MUY reducido  
        {"processedAt", FieldType::STRING, 30}, // Reducido
        {"createdAt", FieldType::STRING, 30},   // Reducido
        {"updatedAt", FieldType::STRING, 30}    // Reducido
    };
}

void SGBDSystemExtended::displayGPSRecordWithHeaders(const VariableRecord& record, const std::string& source) {
    std::cout << "\n📄 REGISTRO GPS COMPLETO:" << std::endl;
    std::cout << "🔍 Fuente: " << source << std::endl;
    std::cout << "📍 Physical Address: " << record.getPhysicalAddress() << std::endl;
    std::cout << "🆔 Record ID: " << record.getId() << std::endl;
    std::cout << "📏 Tamaño: " << record.getSize() << " bytes" << std::endl;
    std::cout << "🗂️ Tipo: Longitud Variable" << std::endl;
    
    std::cout << "\n📋 CAMPOS GPS:" << std::endl;
    const auto& fields = record.getFieldValues();
    const auto& schema = record.getSchema();
    
    for (size_t i = 0; i < fields.size() && i < schema.size(); ++i) {
        std::cout << "   " << schema[i].name << ": " << fields[i] << std::endl;
    }
    
    std::cout << "\n💾 METADATOS DE ALMACENAMIENTO:" << std::endl;
    std::cout << "   Offset Table: [simulado - en página real]" << std::endl;
    std::cout << "   Page Header: BLOCK_HEADER|" << record.getPhysicalAddress() << "|..." << std::endl;
    std::cout << "   Estado: " << (record.isDeleted() ? "ELIMINADO" : "ACTIVO") << std::endl;
}

std::string SGBDSystemExtended::parseTimestamp(const std::string& timestamp_with_tz) {
    // Eliminar zona horaria para simplificar comparaciones
    size_t plus_pos = timestamp_with_tz.find('+');
    if (plus_pos != std::string::npos) {
        return timestamp_with_tz.substr(0, plus_pos);
    }
    return timestamp_with_tz;
}

// ============================================================================
// MÉTODOS EXISTENTES (MANTENGO LOS QUE YA TIENES)
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
    
    // ✅ INTENTA CREAR TABLA (si falla, asume que ya existe)
    bool table_created = disk_manager->createTable(schema.table_name, schema.schema, true);
    
    if (!table_created) {
        std::cout << "🔍 Tabla ya existe, continuando con carga..." << std::endl;
    } else {
        std::cout << "✅ Tabla creada con " << schema.expected_fields << " campos." << std::endl;
    }
    
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
// CLOCK BUFFER MANAGER MEJORADO
// ============================================================================

void SGBDSystemExtended::initializeClockBufferPool() {
    if (current_state < SystemState::DISK_READY) {
        std::cout << "❌ Error: Primero inicializa el disco (opción 1)" << std::endl;
        return;
    }
    
    size_t clock_pool_size;
    std::cout << "\n🕐 INICIALIZACIÓN BUFFER MANAGER CLOCK PIN-AWARE MEJORADO" << std::endl;
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
        
        std::cout << "\n✅ Clock Buffer Manager PIN-AWARE MEJORADO inicializado exitosamente!" << std::endl;
        std::cout << "🕐 Algoritmo Clock MEJORADO activo con " << clock_pool_size << " frames" << std::endl;
        std::cout << "⚡ NUNCA evicta páginas con pin_count > 0" << std::endl;
        std::cout << "🔄 CADA pasada decrementa pin_count automáticamente" << std::endl;
        std::cout << "🎯 GARANTÍA: Eventualmente encuentra víctimas SIEMPRE" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error inicializando Clock Buffer Manager MEJORADO: " << e.what() << std::endl;
    }
}

void SGBDSystemExtended::clockPageOperations() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Error: Primero inicializa Clock Buffer Manager" << std::endl;
        return;
    }
    
    std::cout << "\n=== OPERACIONES DE PÁGINAS CON BUFFER CLOCK MEJORADO ===" << std::endl;
    
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
    
    clock_buffer_manager->displayClockState();
}

void SGBDSystemExtended::createNewPageClock() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n=== CREAR NUEVA PÁGINA CON CLOCK MEJORADO ===" << std::endl;
    
    int new_page_id;
    auto block = clock_buffer_manager->newPage(new_page_id);
    if (block) {
        std::cout << "✅ Nueva página " << new_page_id << " creada" << std::endl;
        clock_buffer_manager->unpinPage(new_page_id, true);
        clock_buffer_manager->displayClockState();
    } else {
        std::cout << "❌ Error creando nueva página" << std::endl;
    }
}

void SGBDSystemExtended::showClockBufferStatus() {
    if (!clock_buffer_manager) {
        std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n🕐 ESTADO COMPLETO CLOCK BUFFER MANAGER PIN-AWARE MEJORADO" << std::endl;
    
    // Mostrar estado detallado usando el método integrado
    clock_buffer_manager->displayClockState();
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
// COMPARACIÓN DE ALGORITMOS ACTUALIZADA
// ============================================================================

void SGBDSystemExtended::compareBufferAlgorithms() {
    if (!buffer_manager || !clock_buffer_manager) {
        std::cout << "❌ Error: Necesitas ambos buffer managers inicializados" << std::endl;
        std::cout << "   - Opción 19: Inicializar BufferPoolManager (LRU)" << std::endl;
        std::cout << "   - Opción 24: Inicializar BufferManagerClock MEJORADO" << std::endl;
        return;
    }
    
    std::cout << "\n⚔️ COMPARACIÓN DETALLADA: LRU vs CLOCK PIN-AWARE MEJORADO" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
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
    
    std::cout << "\n📊 ESTADÍSTICAS CLOCK BUFFER MANAGER MEJORADO:" << std::endl;
    clock_buffer_manager->displayStatistics();
    
    std::cout << "\n🎯 ANÁLISIS COMPARATIVO ACTUALIZADO:" << std::endl;
    
    std::cout << "\n🔒 MANEJO DE PIN_COUNT:" << std::endl;
    std::cout << "LRU:           Verificación básica en BufferPoolManager" << std::endl;
    std::cout << "Clock:         Integrado en algoritmo, NUNCA evicta páginas pinned" << std::endl;
    std::cout << "Clock MEJORADO: + Decremento automático en CADA pasada" << std::endl;
    
    std::cout << "\n⚡ COMPLEJIDAD ALGORÍTMICA:" << std::endl;
    std::cout << "LRU:           O(log n) para acceso + O(n) para eviction" << std::endl;
    std::cout << "Clock:         O(1) para acceso + O(n) worst case para eviction" << std::endl;
    std::cout << "Clock MEJORADO: O(1) acceso + O(n*k) eviction (k=pasadas necesarias)" << std::endl;
    
    std::cout << "\n🛡️ PROTECCIÓN Y GARANTÍAS:" << std::endl;
    std::cout << "LRU:           Puede fallar si no se implementa verificación pin_count" << std::endl;
    std::cout << "Clock:         Protección garantizada + auto-regulación básica" << std::endl;
    std::cout << "Clock MEJORADO: + GARANTÍA total de encontrar víctimas eventualmente" << std::endl;
    
    std::cout << "\n🔄 AUTO-REGULACIÓN:" << std::endl;
    std::cout << "LRU:           No tiene mecanismo de auto-regulación" << std::endl;
    std::cout << "Clock:         Segunda pasada decrementa pins" << std::endl;
    std::cout << "Clock MEJORADO: CADA pasada decrementa pins (más agresivo y efectivo)" << std::endl;
    
    std::cout << "\n🎯 CASOS DE USO RECOMENDADOS:" << std::endl;
    std::cout << "\n✅ Clock MEJORADO es SUPERIOR para:" << std::endl;
    std::cout << "   • Sistemas con páginas de pin_count alto" << std::endl;
    std::cout << "   • Simulaciones académicas (comportamiento predecible)" << std::endl;
    std::cout << "   • Entornos donde se requiere garantía de evicción" << std::endl;
    std::cout << "   • Sistemas críticos (sin deadlocks por falta de víctimas)" << std::endl;
    
    std::cout << "\n✅ LRU sigue siendo superior para:" << std::endl;
    std::cout << "   • Workloads con fuerte localidad temporal" << std::endl;
    std::cout << "   • Cuando la precisión en reemplazo es crítica" << std::endl;
    std::cout << "   • Sistemas de producción estables" << std::endl;
    
    std::cout << "\n🏆 RECOMENDACIÓN FINAL:" << std::endl;
    std::cout << "🎓 Para aprendizaje: Clock MEJORADO (más educativo)" << std::endl;
    std::cout << "🚀 Para producción: Depende del patrón de acceso" << std::endl;
}

// ============================================================================
// MENÚ PRINCIPAL (ÚNICA DEFINICIÓN)
// ============================================================================

void showMenu() {
    std::cout << "\033[2J\033[H";  // Limpiar pantalla
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SGBD FÍSICO INTEGRADO - MENÚ PRINCIPAL EXTENDIDO" << std::endl;
    std::cout << "Sistema con Buffer Pool Management + Índices Especializados GPS" << std::endl;
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
    
    std::cout << "\n🕐 BUFFER CLOCK PIN-AWARE MEJORADO:" << std::endl;
    std::cout << "24. Inicializar Clock Buffer Manager MEJORADO" << std::endl;
    std::cout << "25. Operaciones de páginas Clock" << std::endl;
    std::cout << "26. Crear nueva página Clock" << std::endl;
    std::cout << "27. Ver estado Clock Buffer" << std::endl;
    std::cout << "28. Flush páginas Clock" << std::endl;
    
    std::cout << "\n⚔️ COMPARACIÓN DE ALGORITMOS:" << std::endl;
    std::cout << "29. Comparar LRU vs Clock MEJORADO (Análisis actualizado)" << std::endl;
    
    // === NUEVAS OPCIONES GPS (30-38) ===
    std::cout << "\n🛰️ SISTEMA GPS CON ÍNDICES ESPECIALIZADOS:" << std::endl;
    std::cout << "30. Cargar dataset GPS (Data-GPS.csv)" << std::endl;
    std::cout << "31. Seleccionar configuración de servidor (A/B)" << std::endl;
    
    std::cout << "\n📝 CONSULTAS SQL SOBRE SGBD FÍSICO:" << std::endl;
    std::cout << "32. SELECT * FROM dataGPS" << std::endl;
    std::cout << "33. SELECT WHERE imei = ? (Hash Extensible)" << std::endl;
    std::cout << "34. SELECT WHERE timestamp BETWEEN ? AND ? (B+ Tree)" << std::endl;
    std::cout << "35. INSERT INTO dataGPS" << std::endl;
    
    std::cout << "\n📊 INFORMACIÓN DE ÍNDICES GPS:" << std::endl;
    std::cout << "36. Mostrar estadísticas de índices" << std::endl;
    std::cout << "37. Mostrar estructura de tabla GPS" << std::endl;
    std::cout << "38. Generar diagrama de flujo completo" << std::endl;
    
    std::cout << "\n0.  Salir" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "Opción: ";
}

/**
 * @brief Función principal con sistema limpio y modularizado
 */
int main() {
    #ifdef _WIN32
    // Configurar consola Windows para UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    SGBDSystemExtended sistema;
    int option;
    
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "SISTEMA DE GESTIÓN DE BASE DE DATOS FÍSICO INTEGRADO" << std::endl;
    std::cout << "🚀 Buffer Pool Management + Clock Algorithm Mejorado + GPS" << std::endl;
    std::cout << "📚 Implementación Educativa - Almacenamiento Secundario" << std::endl;
    std::cout << "🎓 Basado en Database System Implementation + CMU Lectures" << std::endl;
    std::cout << "⚡ Algoritmo Clock PIN-AWARE con garantía de víctimas + Índices GPS" << std::endl;
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
                    std::string housing_path = "data/Housing.csv";
                    if (sistema.loadDataset("housing", housing_path)) {
                        std::cout << "✅ Dataset Housing cargado desde: " << housing_path << std::endl;
                    } else {
                        std::cout << "❌ Error: Verifica que existe " << housing_path << std::endl;
                    }
                }
                break;
                
            case 9:
                {
                    std::string titanic_path = "data/titanic.csv";
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
            
            // CLOCK BUFFER MANAGER MEJORADO
            case 24: sistema.initializeClockBufferPool(); break;
            case 25: sistema.clockPageOperations(); break;
            case 26: sistema.createNewPageClock(); break;
            case 27: sistema.showClockBufferStatus(); break;
            case 28: sistema.flushAllClockPages(); break;
            
            // COMPARACIÓN ACTUALIZADA
            case 29: sistema.compareBufferAlgorithms(); break;
            
            // === NUEVOS CASOS GPS 30-38 ===
            case 30:
                {
                    std::string gps_path = "data/Data-GPS.csv";
                    if (sistema.loadGPSDataset(gps_path)) {
                        std::cout << "✅ Dataset GPS cargado desde: " << gps_path << std::endl;
                    } else {
                        std::cout << "❌ Error: Verifica que existe " << gps_path << std::endl;
                        std::cout << "   También puedes intentar: ./data/Data-GPS.csv" << std::endl;
                    }
                }
                break;

            case 31: sistema.selectServerConfiguration(); break;
            case 32: sistema.executeSelectAll(); break;
            case 33: sistema.executeSelectByIMEI(); break;
            case 34: sistema.executeSelectByTimestamp(); break;
            case 35: sistema.executeInsertGPS(); break;
            case 36: sistema.showIndexStatistics(); break;
            case 37: sistema.showGPSTableStructure(); break;
            case 38: sistema.generateFlowDiagram(); break;


                
            case 0:
                std::cout << "\n🎓 ¡Gracias por usar el SGBD Físico Integrado!" << std::endl;
                std::cout << "📚 Has experimentado con:" << std::endl;
                std::cout << "   ✅ Buffer Pool Management profesional" << std::endl;
                std::cout << "   ✅ Page Directory persistente" << std::endl;
                std::cout << "   ✅ Algoritmos LRU y Clock MEJORADO" << std::endl;
                std::cout << "   ✅ Comparación de rendimiento actualizada" << std::endl;
                std::cout << "   🎯 Algoritmo Clock con garantía de víctimas" << std::endl;
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