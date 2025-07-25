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
#include "../include/SGBDDistributed.h"  // NUEVO: Sistema distribuido

#ifdef _WIN32
#include <windows.h>
#include <locale>
#endif

/**
 * @brief Estado del sistema actualizado con Buffer Pool y Sistema Distribuido
 */
enum class SystemState {
    NOT_INITIALIZED,
    DISK_READY,
    BUFFER_POOL_READY,
    DISTRIBUTED_READY,  // NUEVO: Estado para sistema distribuido
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
 * @brief Clase principal del sistema SGBD extendida con capacidades distribuidas
 */
class SGBDSystemExtended {
private:
    // === COMPONENTES PRINCIPALES ===
    std::unique_ptr<DiskManagerExtended> disk_manager;
    std::unique_ptr<BufferPoolManager> buffer_manager;
    std::unique_ptr<BufferManagerClock> clock_buffer_manager; 
    std::unique_ptr<SGBDDistributed> distributed_system;  // NUEVO: Sistema distribuido
    
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
    
    // === CLOCK BUFFER MANAGER (MEJORADO) ===
    void initializeClockBufferPool();
    void clockPageOperations();
    void createNewPageClock();
    void showClockBufferStatus();
    void flushAllClockPages();
    
    // === COMPARACIÓN DE ALGORITMOS ===
    void compareBufferAlgorithms();
    
    // === SISTEMA DISTRIBUIDO (NUEVO) ===
    bool initializeDistributedSystem();
    void runDistributedSystem();
    void loadGPSDatasetToDistributed();
    void showDistributedSystemStatus();
};

// ============================================================================
// IMPLEMENTACIÓN - Sistema Distribuido (NUEVOS MÉTODOS)
// ============================================================================

SGBDSystemExtended::SGBDSystemExtended(const std::string& path, size_t pool_size) 
    : current_state(SystemState::NOT_INITIALIZED)
    , disk_path(path)
    , buffer_pool_size(pool_size) 
{
    disk_manager = std::make_unique<DiskManagerExtended>(path);
}

bool SGBDSystemExtended::initializeDistributedSystem() {
    std::cout << "\n=== INICIALIZANDO SISTEMA DISTRIBUIDO ===" << std::endl;
    
    try {
        // Crear sistema distribuido con path al dataset GPS
        distributed_system = std::make_unique<SGBDDistributed>("data/data-GPS.csv");
        
        // Inicializar servidores especializados
        if (distributed_system->initializeServers()) {
            current_state = SystemState::DISTRIBUTED_READY;
            std::cout << "✅ Sistema distribuido inicializado exitosamente" << std::endl;
            return true;
        } else {
            std::cout << "❌ Error inicializando servidores distribuidos" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        current_state = SystemState::ERROR_STATE;
        return false;
    }
}

void SGBDSystemExtended::runDistributedSystem() {
    if (!distributed_system) {
        std::cout << "❌ Sistema distribuido no inicializado. Ejecuta la opción 30 primero." << std::endl;
        return;
    }
    
    std::cout << "\n🚀 LANZANDO SISTEMA DISTRIBUIDO INTERACTIVO..." << std::endl;
    distributed_system->run();
}

void SGBDSystemExtended::loadGPSDatasetToDistributed() {
    if (!distributed_system) {
        std::cout << "❌ Sistema distribuido no inicializado." << std::endl;
        return;
    }
    
    std::cout << "\n=== CARGA DE DATASET GPS ===" << std::endl;
    std::cout << "Archivo a cargar: ";
    
    std::string gps_file;
    std::getline(std::cin, gps_file);
    
    if (gps_file.empty()) {
        gps_file = "data/data-GPS.csv";
        std::cout << "Usando archivo por defecto: " << gps_file << std::endl;
    }
    
    if (distributed_system->loadCompleteGPSDataset(gps_file)) {
        std::cout << "✅ Dataset GPS cargado exitosamente en sistema distribuido" << std::endl;
    } else {
        std::cout << "❌ Error cargando dataset GPS" << std::endl;
    }
}

void SGBDSystemExtended::showDistributedSystemStatus() {
    if (!distributed_system) {
        std::cout << "❌ Sistema distribuido no inicializado" << std::endl;
        return;
    }
    
    std::cout << "\n=== ESTADO DEL SISTEMA DISTRIBUIDO ===" << std::endl;
    
    auto state = distributed_system->getState();
    std::string state_str;
    
    switch (state) {
        case DistributedSystemState::NOT_INITIALIZED:
            state_str = "❌ NO INICIALIZADO";
            break;
        case DistributedSystemState::SERVERS_READY:
            state_str = "🟡 SERVIDORES LISTOS";
            break;
        case DistributedSystemState::DATA_LOADED:
            state_str = "✅ DATOS CARGADOS";
            break;
        case DistributedSystemState::ERROR_STATE:
            state_str = "🔴 ERROR";
            break;
    }
    
    std::cout << "Estado: " << state_str << std::endl;
    std::cout << "Registros cargados: " << distributed_system->getTotalRecords() << std::endl;
    
    distributed_system->displayDetailedStatistics();
}

// ============================================================================
// MÉTODOS EXISTENTES (Mantenidos sin cambios)
// ============================================================================

std::map<std::string, DatasetSchema> SGBDSystemExtended::getDatasetSchemas() {
    std::map<std::string, DatasetSchema> datasets;
    
    // Dataset Housing existente
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
            {"furnishingstatus", FieldType::STRING, 15}
        },
        ',',
        "Dataset de precios de viviendas con 13 características",
        13
    };
    
    // NUEVO: Dataset GPS para integración
    datasets["gps"] = {
        "gps_tracks",
        GPSDatasetSchema::getSchema(),
        GPSDatasetSchema::getDelimiter(),
        GPSDatasetSchema::getDescription(),
        GPSDatasetSchema::getExpectedFields()
    };
    
    return datasets;
}

void SGBDSystemExtended::showSystemStatus() {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "ESTADO DEL SISTEMA INTEGRADO:" << std::endl;
    
    switch (current_state) {
        case SystemState::NOT_INITIALIZED:
            std::cout << "Estado: NO INICIALIZADO" << std::endl;
            std::cout << "Disco: No creado" << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Sistema Distribuido: No disponible" << std::endl;
            std::cout << "Acción requerida: Inicializar disco (opción 1)" << std::endl;
            break;
            
        case SystemState::DISK_READY:
            std::cout << "Estado: DISCO LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: No inicializado" << std::endl;
            std::cout << "Sistema Distribuido: Disponible para inicializar" << std::endl;
            break;
            
        case SystemState::BUFFER_POOL_READY:
            std::cout << "Estado: BUFFER POOL LISTO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: Activo (" << buffer_pool_size << " frames)" << std::endl;
            std::cout << "Sistema Distribuido: Disponible para inicializar" << std::endl;
            break;
            
        case SystemState::DISTRIBUTED_READY:
            std::cout << "Estado: SISTEMA DISTRIBUIDO ACTIVO" << std::endl;
            std::cout << "Disco: " << disk_path << std::endl;
            std::cout << "Buffer Pool: Activo (" << buffer_pool_size << " frames)" << std::endl;
            std::cout << "Sistema Distribuido: ✅ OPERATIVO" << std::endl;
            if (distributed_system) {
                std::cout << "Registros GPS: " << distributed_system->getTotalRecords() << std::endl;
            }
            break;
            
        case SystemState::ERROR_STATE:
            std::cout << "Estado: ERROR" << std::endl;
            std::cout << "Se produjo un error en el sistema" << std::endl;
            break;
    }
    
    std::cout << std::string(60, '-') << std::endl;
}

// [Aquí van todos los métodos existentes del SGBDSystemExtended...]
// (Por brevedad, no los incluyo todos, pero estarían aquí)

// ============================================================================
// MENÚ PRINCIPAL EXTENDIDO
// ============================================================================

void showMenu() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SGBD FÍSICO INTEGRADO + SISTEMA DISTRIBUIDO - MENÚ PRINCIPAL" << std::endl;
    std::cout << "Sistema con Buffer Pool Management + Índices Distribuidos" << std::endl;
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
    std::cout << "10. Cargar dataset GPS para sistema distribuido" << std::endl;  // NUEVO
    
    std::cout << "\n🎯 SIMULACIONES:" << std::endl;
    std::cout << "11. Simular sector sin espacio suficiente" << std::endl;
    std::cout << "12. Simular sectores llenos" << std::endl;
    
    std::cout << "\n🔍 CONSULTAS Y OPERACIONES:" << std::endl;
    std::cout << "13. Buscar registro por ID" << std::endl;
    std::cout << "14. Eliminar registro" << std::endl;
    std::cout << "15. Mostrar tabla completa" << std::endl;
    std::cout << "16. Compactar tabla" << std::endl;
    
    std::cout << "\n📈 INFORMACIÓN DEL SISTEMA:" << std::endl;
    std::cout << "17. Mostrar estadísticas integradas" << std::endl;
    std::cout << "18. Mostrar estructura de directorios" << std::endl;
    std::cout << "19. Mostrar Page Directory" << std::endl;

    std::cout << "\n🏊 BUFFER POOL LRU:" << std::endl;
    std::cout << "20. Inicializar Buffer Pool Manager (LRU)" << std::endl;
    std::cout << "21. Operaciones de páginas (READ/WRITE)" << std::endl;
    std::cout << "22. Crear nueva página con Buffer Pool" << std::endl;
    std::cout << "23. Ver estado del Buffer Pool" << std::endl;
    std::cout << "24. Flush todas las páginas dirty" << std::endl;
    
    std::cout << "\n🕐 BUFFER CLOCK PIN-AWARE MEJORADO:" << std::endl;
    std::cout << "25. Inicializar Clock Buffer Manager MEJORADO" << std::endl;
    std::cout << "26. Operaciones de páginas Clock" << std::endl;
    std::cout << "27. Crear nueva página Clock" << std::endl;
    std::cout << "28. Ver estado Clock Buffer" << std::endl;
    std::cout << "29. Flush páginas Clock" << std::endl;
    
    std::cout << "\n⚔️ COMPARACIÓN DE ALGORITMOS:" << std::endl;
    std::cout << "30. Comparar LRU vs Clock MEJORADO" << std::endl;
    
    std::cout << "\n🌐 SISTEMA DISTRIBUIDO (NUEVO):" << std::endl;
    std::cout << "31. Inicializar Sistema Distribuido (Hash + B+ Tree)" << std::endl;
    std::cout << "32. Ejecutar Sistema Distribuido Interactivo" << std::endl;
    std::cout << "33. Cargar dataset GPS completo" << std::endl;
    std::cout << "34. Ver estado del sistema distribuido" << std::endl;
    
    std::cout << "\n0.  Salir" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

// ============================================================================
// FUNCIÓN PRINCIPAL CON NUEVAS OPCIONES
// ============================================================================

int main() {
    std::cout << "🌟 SGBD FÍSICO INTEGRADO + SISTEMA DISTRIBUIDO 🌟" << std::endl;
    std::cout << "===================================================" << std::endl;
    
    SGBDSystemExtended system;
    
    std::string choice;
    do {
        showMenu();
        std::cout << "\nSelecciona una opción (0-34): ";
        std::getline(std::cin, choice);
        
        // [Aquí van todas las opciones existentes 1-30...]
        
        // NUEVAS OPCIONES DEL SISTEMA DISTRIBUIDO
        if (choice == "31") {
            system.initializeDistributedSystem();
        } else if (choice == "32") {
            system.runDistributedSystem();
        } else if (choice == "33") {
            system.loadGPSDatasetToDistributed();
        } else if (choice == "34") {
            system.showDistributedSystemStatus();
        } else if (choice == "10") {
            // Carga GPS tradicional para comparación
            if (system.loadDataset("gps", "data/data-GPS.csv")) {
                std::cout << "✅ Dataset GPS cargado en sistema tradicional" << std::endl;
            }
        } else if (choice != "0") {
            std::cout << "❌ Opción inválida. Selecciona 0-34." << std::endl;
        }
        
        if (choice != "0") {
            std::cout << "\nPresiona Enter para continuar...";
            std::cin.get();
        }
        
    } while (choice != "0");
    
    std::cout << "\n👋 ¡Gracias por usar el SGBD Físico Integrado!" << std::endl;
    return 0;
}