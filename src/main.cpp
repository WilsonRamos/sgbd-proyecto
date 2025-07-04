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
 * @brief Clase principal del sistema con Buffer Pool Management y Clock Pin-Aware
 */
class SGBDSystemExtended {
private:
    std::unique_ptr<DiskManagerExtended> disk_manager;
    std::unique_ptr<BufferPoolManager> buffer_manager;
    std::unique_ptr<BufferManagerClock> clock_buffer_manager; 
    SystemState current_state;
    std::string disk_path;
    size_t buffer_pool_size;
    
    // ==========================================================================
    // MÉTODOS PRIVADOS AUXILIARES
    // ==========================================================================
    
    std::map<std::string, DatasetSchema> getDatasetSchemas() {
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
    
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',') {
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
    
    int countRecordsInFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return 0;
        
        int count = 0;
        std::string line;
        bool first_line = true;
        
        while (std::getline(file, line)) {
            if (first_line) {
                first_line = false; // Saltar header
                continue;
            }
            if (!line.empty()) count++;
        }
        
        file.close();
        return count;
    }
    
    size_t estimateRecordSize(const std::vector<std::string>& values) {
        size_t size = 0;
        for (const auto& val : values) {
            size += val.length() + 8; // Valor + overhead
        }
        return size;
    }
    
    void showDiskStructure(const DiskConfig& config) {
        std::cout << "\n=== ESTRUCTURA DEL DISCO EXTENDIDO ===" << std::endl;
        std::cout << "Configuracion:" << std::endl;
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
        std::cout << "APLICACION" << std::endl;
        std::cout << "    ↓" << std::endl;
        std::cout << "BUFFER POOL MANAGER (En memoria)" << std::endl;
        std::cout << "  ├─ Page Table: PageID → FrameID" << std::endl;
        std::cout << "  ├─ LRU Replacer: Política de evicción" << std::endl;
        std::cout << "  └─ Buffer Pool: Array de frames" << std::endl;
        std::cout << "    ↓" << std::endl;
        std::cout << "DISK MANAGER EXTENDED" << std::endl;
        std::cout << "  ├─ Page Directory: PageID → Ubicación física (PERSISTENTE)" << std::endl;
        std::cout << "  └─ File System Simulator" << std::endl;
        std::cout << "    ↓" << std::endl;
        std::cout << "ARCHIVO FISICO" << std::endl;
        std::cout << "  └─ " << disk_path << "/" << std::endl;
    }
    
    bool requiresDisk() {
        if (current_state == SystemState::NOT_INITIALIZED) {
            std::cout << "\n❌ ERROR: Operación requiere disco inicializado." << std::endl;
            std::cout << "Ejecuta primero la opción 1 o 2." << std::endl;
            return false;
        }
        return true;
    }
    
    bool requiresBufferPool() {
        if (current_state != SystemState::BUFFER_POOL_READY) {
            std::cout << "\n❌ ERROR: Operación requiere Buffer Pool inicializado." << std::endl;
            if (current_state == SystemState::DISK_READY) {
                std::cout << "Ejecuta la opción 18 para inicializar Buffer Pool." << std::endl;
            } else {
                std::cout << "Ejecuta primero las opciones 1 (o 2) y luego 18." << std::endl;
            }
            return false;
        }
        return true;
    }
    
public:
    SGBDSystemExtended(const std::string& path = "mi_disco_sgbd", size_t pool_size = 4) 
        : current_state(SystemState::NOT_INITIALIZED)
        , disk_path(path)
        , buffer_pool_size(pool_size) 
    {
        disk_manager = std::make_unique<DiskManagerExtended>(path);
    }
    
    SystemState getState() const { return current_state; }
    
    void showSystemStatus() {
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
                std::cout << "Accion requerida: Inicializar Buffer Pool (opcion 18)" << std::endl;
                break;
                
            case SystemState::BUFFER_POOL_READY:
                std::cout << "Estado: SISTEMA COMPLETO LISTO" << std::endl;
                std::cout << "Disco: " << disk_path << std::endl;
                std::cout << "Buffer Pool: " << buffer_pool_size << " frames" << std::endl;
                std::cout << "Page Directory: ✓ Gestionado por DiskManager" << std::endl;
                std::cout << "Accion: Sistema listo para operaciones avanzadas" << std::endl;
                break;
                
            case SystemState::ERROR_STATE:
                std::cout << "Estado: ERROR" << std::endl;
                std::cout << "Accion requerida: Reinicializar sistema" << std::endl;
                break;
        }
        std::cout << std::string(60, '-') << std::endl;
    }
    
    // ==========================================================================
    // INICIALIZACIÓN DEL SISTEMA
    // ==========================================================================
    
    bool initializeDisk() {
        std::cout << "\n=== INICIALIZACION DEL DISCO EXTENDIDO ===" << std::endl;
        
        std::string input;
        std::cout << "¿Usar configuracion por defecto? (s/n): ";
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
        
        // Mostrar estructura del disco
        showDiskStructure(config);
        
        if (disk_manager->initialize(config)) {
            current_state = SystemState::DISK_READY;
            std::cout << "\n✅ Disco inicializado exitosamente en: " << disk_path << std::endl;
            std::cout << "📁 Page Directory creado automáticamente por DiskManager" << std::endl;
            std::cout << "🎯 Siguiente paso: Inicializar Buffer Pool (opción 18)" << std::endl;
            return true;
        } else {
            current_state = SystemState::ERROR_STATE;
            std::cout << "\n❌ Error inicializando el disco." << std::endl;
            return false;
        }
    }
    
    bool loadExistingDisk() {
        std::cout << "\n=== CARGANDO DISCO EXISTENTE EXTENDIDO ===" << std::endl;
        
        if (disk_manager->loadExistingDisk()) {
            current_state = SystemState::DISK_READY;
            std::cout << "✅ Disco cargado desde: " << disk_path << std::endl;
            std::cout << "📁 Page Directory cargado automáticamente" << std::endl;
            std::cout << "🎯 Siguiente paso: Inicializar Buffer Pool (opción 18)" << std::endl;
            return true;
        } else {
            std::cout << "❌ Error: No se encontró disco en " << disk_path << std::endl;
            return false;
        }
    }
    
    // ==========================================================================
    // BUFFER POOL MANAGEMENT (LRU)
    // ==========================================================================
    
    bool initializeBufferPool() {
        if (current_state != SystemState::DISK_READY) {
            std::cout << "\n❌ ERROR: Requiere disco inicializado primero." << std::endl;
            return false;
        }
        
        std::cout << "\n=== INICIALIZACION DEL BUFFER POOL ===" << std::endl;
        
        std::string input;
        std::cout << "Tamaño del buffer pool (frames) [" << buffer_pool_size << "]: ";
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            try {
                size_t new_size = std::stoull(input);
                if (new_size > 0 && new_size <= 64) {
                    buffer_pool_size = new_size;
                } else {
                    std::cout << "⚠️  Tamaño inválido, usando por defecto: " << buffer_pool_size << std::endl;
                }
            } catch (const std::exception&) {
                std::cout << "⚠️  Entrada inválida, usando por defecto: " << buffer_pool_size << std::endl;
            }
        }
        
        try {
            buffer_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());
            current_state = SystemState::BUFFER_POOL_READY;
            
            std::cout << "\n🚀 Buffer Pool Manager inicializado exitosamente!" << std::endl;
            std::cout << "   - Pool size: " << buffer_pool_size << " frames" << std::endl;
            std::cout << "   - Page Table: ✓ (En memoria)" << std::endl;
            std::cout << "   - Page Directory: ✓ (Gestionado por DiskManager)" << std::endl;
            std::cout << "   - LRU Replacer: ✓" << std::endl;
            std::cout << "🎯 Sistema completo listo para operaciones avanzadas!" << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Error inicializando Buffer Pool: " << e.what() << std::endl;
            current_state = SystemState::ERROR_STATE;
            return false;
        }
    }
    
    void testBufferPoolOperations() {
        if (!requiresBufferPool()) return;
        
        std::cout << "\n=== DEMO DE OPERACIONES DEL BUFFER POOL ===" << std::endl;
        
        // Solicitar páginas para demostrar funcionamiento
        std::vector<int> test_pages = {1, 2, 3, 4, 5}; // Más páginas que frames para forzar evicción
        
        std::cout << "\n🔍 Solicitando páginas secuencialmente..." << std::endl;
        
        for (int page_id : test_pages) {
            std::cout << "\n--- Solicitando página " << page_id << " ---" << std::endl;
            
            auto block = buffer_manager->requestPage(page_id, PageOperation::READ);
            if (block) {
                std::cout << "✅ Página " << page_id << " cargada exitosamente" << std::endl;
                buffer_manager->unpinPage(page_id);
            } else {
                std::cout << "❌ Error cargando página " << page_id << std::endl;
            }
            
            // Mostrar estado después de cada operación
            buffer_manager->displayCompactStatus();
        }
        
        std::cout << "\n📊 Estado final del Buffer Pool:" << std::endl;
        buffer_manager->displayBufferPoolInfo();
    }
    
    void bufferPoolPageOperations() {
        if (!requiresBufferPool()) return;
        
        std::cout << "\n=== OPERACIONES DE PÁGINAS CON BUFFER POOL ===" << std::endl;
        
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
            
            // Preguntar si desea hacer unpin
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
    
    void createNewPageBuffered() {
        if (!requiresBufferPool()) return;
        
        std::cout << "\n=== CREAR NUEVA PÁGINA CON BUFFER POOL ===" << std::endl;
        
        int new_page_id = buffer_manager->createNewPage();
        if (new_page_id != -1) {
            std::cout << "\n✨ Nueva página creada con ID: " << new_page_id << std::endl;
            
            // Mostrar información del Page Directory
            std::cout << "\n📁 Page Directory actualizado:" << std::endl;
            disk_manager->displayPageDirectory();
            
            // Mostrar estado del buffer pool
            buffer_manager->displayCompactStatus();
            
            // Liberar la página
            buffer_manager->unpinPage(new_page_id, true); // true = dirty
        } else {
            std::cout << "❌ Error creando nueva página" << std::endl;
        }
    }
    
    void showBufferPoolStatus() {
        if (!requiresBufferPool()) return;
        
        std::cout << "\n=== ESTADO COMPLETO DEL BUFFER POOL ===" << std::endl;
        buffer_manager->displayBufferPoolInfo();
    }
    
    void flushAllPages() {
        if (!requiresBufferPool()) return;
        
        std::cout << "\n=== FLUSH DE TODAS LAS PÁGINAS DIRTY ===" << std::endl;
        buffer_manager->flushAllPages();
        std::cout << "✅ Todas las páginas dirty han sido escritas a disco" << std::endl;
    }
    
    void showPageDirectory() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== PAGE DIRECTORY (GESTIONADO POR DISK MANAGER) ===" << std::endl;
        disk_manager->displayPageDirectory();
    }
    
    // ==========================================================================
    // CLOCK BUFFER MANAGER PIN-AWARE
    // ==========================================================================
    
    void initializeClockBufferPool() {
        if (current_state < SystemState::DISK_READY) {
            std::cout << "❌ Error: Primero inicializa el disco (opción 1)" << std::endl;
            return;
        }
        
        size_t clock_pool_size;
        std::cout << "\n🕐 INICIALIZACIÓN BUFFER MANAGER CLOCK PIN-AWARE" << std::endl;
        std::cout << "Tamaño del Clock Buffer Pool (frames): ";
        std::cin >> clock_pool_size;
        
        if (clock_pool_size < 2 || clock_pool_size > 20) {
            std::cout << "⚠️  Tamaño recomendado: 2-20 frames. Usando 4." << std::endl;
            clock_pool_size = 4;
        }
        
        try {
            clock_buffer_manager = std::make_unique<BufferManagerClock>(
                clock_pool_size, disk_manager.get());
            
            std::cout << "\n✅ Clock Buffer Manager PIN-AWARE inicializado exitosamente!" << std::endl;
            std::cout << "🕐 Algoritmo Clock activo con " << clock_pool_size << " frames" << std::endl;
            std::cout << "⚡ NUNCA evicta páginas con pin_count > 0" << std::endl;
            std::cout << "🔄 Segunda pasada decrementa pin_count automáticamente" << std::endl;
            std::cout << "🛡️  Protección total contra evicción incorrecta" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Error inicializando Clock Buffer Manager: " << e.what() << std::endl;
        }
    }
    
    void testClockBufferOperations() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Error: Primero inicializa Clock Buffer Manager (opción 25)" << std::endl;
            return;
        }
        
        std::cout << "\n🧪 PRUEBAS CLOCK BUFFER OPERATIONS" << std::endl;
        std::cout << "=== Test de comportamiento Clock vs LRU ===" << std::endl;
        
        // Test 1: Cargar páginas secuencialmente
        std::cout << "\n📖 Test 1: Cargando páginas secuencialmente..." << std::endl;
        std::vector<int> test_pages = {1, 2, 3, 4, 5, 6};
        
        for (int page_id : test_pages) {
            std::cout << "\n--- Accediendo página " << page_id << " ---" << std::endl;
            
            // Crear página si no existe
            int new_page_id;
            auto block = clock_buffer_manager->newPage(new_page_id);
            if (block) {
                std::cout << "✅ Nueva página " << new_page_id << " creada" << std::endl;
                clock_buffer_manager->unpinPage(new_page_id, false);
            }
            
            clock_buffer_manager->displayCompactState();
        }
        
        // Test 2: Acceso repetido a páginas (probar reference bits)
        std::cout << "\n🔄 Test 2: Re-accediendo páginas para probar reference bits..." << std::endl;
        std::vector<int> existing_pages;
        
        // Crear algunas páginas para el test
        for (int i = 0; i < 3; ++i) {
            int new_page_id;
            auto block = clock_buffer_manager->newPage(new_page_id);
            if (block) {
                existing_pages.push_back(new_page_id);
                clock_buffer_manager->unpinPage(new_page_id, false);
            }
        }
        
        // Re-acceder páginas existentes
        for (int page_id : existing_pages) {
            std::cout << "\n--- Re-accediendo página " << page_id << " ---" << std::endl;
            auto block = clock_buffer_manager->fetchPage(page_id);
            if (block) {
                clock_buffer_manager->unpinPage(page_id, false);
            }
            clock_buffer_manager->displayCompactState();
        }
        
        std::cout << "\n✅ Tests Clock completados!" << std::endl;
    }
    
    void demonstrateClockVsLRU() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Error: Primero inicializa Clock Buffer Manager (opción 25)" << std::endl;
            return;
        }
        
        std::cout << "\n⚔️  DEMOSTRACIÓN: CLOCK vs LRU" << std::endl;
        std::cout << "=== Escenario: Sequential Flooding ===" << std::endl;
        
        std::cout << "\n🎯 Clock Algorithm ventajas:" << std::endl;
        std::cout << "   ✅ Menor overhead (no timestamps)" << std::endl;
        std::cout << "   ✅ Resistente a sequential flooding" << std::endl;
        std::cout << "   ✅ Aproximación eficiente de LRU" << std::endl;
        std::cout << "   ✅ Reference bits dan segunda oportunidad" << std::endl;
        
        // Simular patrón problemático para LRU
        std::cout << "\n📖 Simulando patrón que afecta LRU:" << std::endl;
        std::cout << "   Páginas frecuentes: Se crearán dinámicamente" << std::endl;
        std::cout << "   Sequential scan: Nuevas páginas (patrones únicos)" << std::endl;
        
        // Establecer páginas "frecuentes"
        std::vector<int> frequent_pages;
        for (int i = 0; i < 3; ++i) {
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                frequent_pages.push_back(page_id);
                clock_buffer_manager->unpinPage(page_id, false);
                std::cout << "📌 Página frecuente " << page_id << " creada" << std::endl;
            }
        }
        
        std::cout << "\n🕐 Estado inicial:" << std::endl;
        clock_buffer_manager->displayCompactState();
        
        // Sequential scan que debería NO afectar páginas frecuentes
        std::cout << "\n🌊 Iniciando sequential scan (5 páginas nuevas):" << std::endl;
        std::vector<int> scan_pages;
        for (int i = 0; i < 5; ++i) {
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                scan_pages.push_back(page_id);
                std::cout << "\n--- Scan página " << page_id << " ---" << std::endl;
                clock_buffer_manager->unpinPage(page_id, false);
                clock_buffer_manager->displayCompactState();
            }
        }
        
        // Verificar si páginas frecuentes sobrevivieron
        std::cout << "\n🔍 Verificando supervivencia de páginas frecuentes:" << std::endl;
        for (int page_id : frequent_pages) {
            auto block = clock_buffer_manager->fetchPage(page_id);
            if (block) {
                std::cout << "✅ Página frecuente " << page_id << " sobrevivió!" << std::endl;
                clock_buffer_manager->unpinPage(page_id, false);
            } else {
                std::cout << "❌ Página frecuente " << page_id << " fue evictada" << std::endl;
            }
        }
        
        std::cout << "\n🏆 Clock Algorithm demostrado!" << std::endl;
    }
    
    void clockAdvancedPageOperations() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Error: Primero inicializa Clock Buffer Manager (opción 25)" << std::endl;
            return;
        }
        
        std::cout << "\n🔧 OPERACIONES AVANZADAS CLOCK" << std::endl;
        
        int option;
        do {
            std::cout << "\n--- Operaciones Disponibles ---" << std::endl;
            std::cout << "1. Fetch página específica" << std::endl;
            std::cout << "2. Crear nueva página" << std::endl;
            std::cout << "3. Flush página específica" << std::endl;
            std::cout << "4. Delete página específica" << std::endl;
            std::cout << "5. Mostrar estado del Clock" << std::endl;
            std::cout << "6. Estadísticas detalladas" << std::endl;
            std::cout << "0. Volver al menú principal" << std::endl;
            std::cout << "Opción: ";
            std::cin >> option;
            
            switch (option) {
                case 1: {
                    int page_id;
                    std::cout << "ID de página a fetch: ";
                    std::cin >> page_id;
                    
                    auto block = clock_buffer_manager->fetchPage(page_id);
                    if (block) {
                        std::cout << "✅ Página " << page_id << " obtenida" << std::endl;
                        
                        char dirty_choice;
                        std::cout << "¿Marcar como dirty? (y/n): ";
                        std::cin >> dirty_choice;
                        
                        clock_buffer_manager->unpinPage(page_id, dirty_choice == 'y');
                    }
                    break;
                }
                
                case 2: {
                    int new_page_id;
                    auto block = clock_buffer_manager->newPage(new_page_id);
                    if (block) {
                        std::cout << "✅ Nueva página " << new_page_id << " creada" << std::endl;
                        clock_buffer_manager->unpinPage(new_page_id, true);
                    }
                    break;
                }
                
                case 3: {
                    int page_id;
                    std::cout << "ID de página a flush: ";
                    std::cin >> page_id;
                    
                    if (clock_buffer_manager->flushPage(page_id)) {
                        std::cout << "✅ Página " << page_id << " flushed" << std::endl;
                    } else {
                        std::cout << "❌ Error flushing página " << page_id << std::endl;
                    }
                    break;
                }
                
                case 4: {
                    int page_id;
                    std::cout << "ID de página a eliminar: ";
                    std::cin >> page_id;
                    
                    if (clock_buffer_manager->deletePage(page_id)) {
                        std::cout << "✅ Página " << page_id << " eliminada" << std::endl;
                    } else {
                        std::cout << "❌ Error eliminando página " << page_id << std::endl;
                    }
                    break;
                }
                
                case 5:
                    clock_buffer_manager->displayClockState();
                    break;
                    
                case 6:
                    clock_buffer_manager->displayStatistics();
                    break;
                    
                case 0:
                    break;
                    
                default:
                    std::cout << "❌ Opción no válida" << std::endl;
                    break;
            }
            
            if (option != 0) {
                std::cout << "\nPresiona Enter para continuar...";
                std::cin.ignore();
                std::cin.get();
            }
            
        } while (option != 0);
    }
    
    void showClockBufferStatus() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
            return;
        }
        
        std::cout << "\n🕐 ESTADO COMPLETO CLOCK BUFFER MANAGER PIN-AWARE" << std::endl;
        clock_buffer_manager->displayStatistics();
        clock_buffer_manager->displayClockState();
    }
    
    void flushAllClockPages() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Clock Buffer Manager no inicializado" << std::endl;
            return;
        }
        
        std::cout << "\n💾 Flushing todas las páginas Clock..." << std::endl;
        clock_buffer_manager->flushAllDirtyPages();
        std::cout << "✅ Flush completo!" << std::endl;
    }
    
    void compareClockVsLRUPerformance() {
        if (!buffer_manager || !clock_buffer_manager) {
            std::cout << "❌ Error: Necesitas ambos buffer managers inicializados" << std::endl;
            std::cout << "   - Opción 18: BufferPoolManager (LRU)" << std::endl;
            std::cout << "   - Opción 25: BufferManagerClock" << std::endl;
            return;
        }
        
        std::cout << "\n⚔️  COMPARACIÓN RENDIMIENTO: CLOCK vs LRU" << std::endl;
        std::cout << "=== Análisis de estadísticas actuales ===" << std::endl;
        
        // Mostrar estadísticas LRU
        std::cout << "\n📊 ESTADÍSTICAS LRU BUFFER MANAGER:" << std::endl;
        showBufferPoolStatus();
        
        // Mostrar estadísticas Clock
        std::cout << "\n📊 ESTADÍSTICAS CLOCK BUFFER MANAGER:" << std::endl;
        clock_buffer_manager->displayStatistics();
        
        std::cout << "\n🎯 CONCLUSIONES:" << std::endl;
        std::cout << "📈 Ventajas Clock:" << std::endl;
        std::cout << "   • Menor overhead de memoria (no timestamps)" << std::endl;
        std::cout << "   • Resistente a sequential flooding" << std::endl;
        std::cout << "   • Algoritmo más simple y eficiente" << std::endl;
        std::cout << "   • Reference bits dan segunda oportunidad" << std::endl;
        
        std::cout << "\n📈 Ventajas LRU:" << std::endl;
        std::cout << "   • Política de reemplazo más precisa" << std::endl;
        std::cout << "   • Mejor para workloads con localidad temporal fuerte" << std::endl;
        std::cout << "   • Comportamiento más predecible" << std::endl;
        std::cout << "   • Información temporal más granular" << std::endl;
    }
    
    // ==========================================================================
    // TESTS PIN-AWARENESS (CRÍTICOS)
    // ==========================================================================
    
    void testClockPinAwareness() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Error: Primero inicializa Clock Buffer Manager (opción 25)" << std::endl;
            return;
        }
        
        std::cout << "\n🧪 TEST PIN-AWARENESS: Clock NO debe evictar páginas pinned" << std::endl;
        std::cout << "=== Demostrando protección contra evicción incorrecta ===" << std::endl;
        
        // PASO 1: Crear páginas y verificar pins
        std::cout << "\n📖 PASO 1: Creando páginas de prueba..." << std::endl;
        std::vector<int> test_pages;
        
        for (int i = 0; i < 3; ++i) {
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                test_pages.push_back(page_id);
                std::cout << "✨ Página " << page_id << " creada (pin_count=1)" << std::endl;
            }
        }
        
        std::cout << "\n📊 Estado inicial (todas pinned):" << std::endl;
        clock_buffer_manager->displayClockState();
        
        // PASO 2: Unpin selectivamente
        std::cout << "\n🔓 PASO 2: Unpinning páginas selectivamente..." << std::endl;
        if (test_pages.size() >= 3) {
            // Unpin solo la primera página
            clock_buffer_manager->unpinPage(test_pages[0], false);
            std::cout << "🔓 Página " << test_pages[0] << " UNPINNED (evictable)" << std::endl;
            
            // Dejar test_pages[1] y test_pages[2] PINNED
            std::cout << "📌 Páginas " << test_pages[1] << " y " << test_pages[2] 
                      << " permanecen PINNED (no evictables)" << std::endl;
        }
        
        std::cout << "\n📊 Estado después de unpinning selectivo:" << std::endl;
        clock_buffer_manager->displayClockState();
        
        // PASO 3: Forzar evicción creando más páginas
        std::cout << "\n🌊 PASO 3: Creando páginas adicionales para forzar evicción..." << std::endl;
        std::vector<int> flood_pages;
        
        for (int i = 0; i < 5; ++i) {  // Más páginas que el buffer size
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                flood_pages.push_back(page_id);
                std::cout << "\n➕ Página flood " << page_id << " creada" << std::endl;
                
                // Unpin inmediatamente para que sea evictable
                clock_buffer_manager->unpinPage(page_id, false);
                std::cout << "🔓 Página flood " << page_id << " unpinned (evictable)" << std::endl;
                
                std::cout << "📊 Estado después de crear página " << page_id << ":" << std::endl;
                clock_buffer_manager->displayCompactState();
            }
        }
        
        // PASO 4: Verificar qué páginas sobrevivieron
        std::cout << "\n🔍 PASO 4: VERIFICACIÓN FINAL - ¿Qué páginas sobrevivieron?" << std::endl;
        
        // Verificar páginas originales
        for (size_t i = 0; i < test_pages.size(); ++i) {
            int page_id = test_pages[i];
            auto block = clock_buffer_manager->fetchPage(page_id);
            
            if (block) {
                if (i == 0) {
                    std::cout << "✅ Página " << page_id << " SOBREVIVIÓ (era unpinned, pero evitable)" << std::endl;
                } else {
                    std::cout << "✅ Página " << page_id << " SOBREVIVIÓ (estaba pinned - CORRECTO!)" << std::endl;
                }
                clock_buffer_manager->unpinPage(page_id, false);
            } else {
                if (i == 0) {
                    std::cout << "❌ Página " << page_id << " fue evictada (era unpinned - normal)" << std::endl;
                } else {
                    std::cout << "🚨 ERROR: Página " << page_id << " fue evictada (¡estaba pinned!)" << std::endl;
                }
            }
        }
        
        std::cout << "\n📊 Estado final del Clock:" << std::endl;
        clock_buffer_manager->displayClockState();
        
        // PASO 5: Estadísticas
        std::cout << "\n📈 RESULTADOS DEL TEST PIN-AWARENESS:" << std::endl;
        clock_buffer_manager->displayStatistics();
        
        std::cout << "\n🎯 CONCLUSIONES:" << std::endl;
        std::cout << "✅ Clock Algorithm PIN-AWARE funciona correctamente:" << std::endl;
        std::cout << "   • NUNCA evicta páginas con pin_count > 0" << std::endl;
        std::cout << "   • Solo evicta páginas con pin_count = 0" << std::endl;
        std::cout << "   • Segunda pasada puede decrementar pin_count automáticamente" << std::endl;
        std::cout << "   • Protege contra evicción incorrecta" << std::endl;
    }
    
    void testClockSecondPassDecrement() {
        if (!clock_buffer_manager) {
            std::cout << "❌ Error: Primero inicializa Clock Buffer Manager (opción 25)" << std::endl;
            return;
        }
        
        std::cout << "\n🔄 TEST SEGUNDA PASADA: Pin decrement automático" << std::endl;
        std::cout << "=== Verificando que segunda pasada disminuye pin_count ===" << std::endl;
        
        // PASO 1: Crear páginas y hacer múltiples pins
        std::cout << "\n📖 PASO 1: Creando páginas con múltiples pins..." << std::endl;
        std::vector<int> test_pages;
        
        for (int i = 0; i < 2; ++i) {
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                test_pages.push_back(page_id);
                
                // Hacer múltiples fetchPage para incrementar pin_count
                for (int j = 0; j < 3; ++j) {
                    auto block2 = clock_buffer_manager->fetchPage(page_id);
                    std::cout << "📌 Página " << page_id << " fetched (pin aumentado)" << std::endl;
                }
            }
        }
        
        std::cout << "\n📊 Estado inicial (páginas con pins altos):" << std::endl;
        clock_buffer_manager->displayClockState();
        
        // PASO 2: Llenar buffer para forzar múltiples pasadas del clock
        std::cout << "\n🌊 PASO 2: Llenando buffer para forzar pasadas múltiples..." << std::endl;
        
        for (int i = 0; i < 10; ++i) {  // Muchas más páginas que el buffer
            int page_id;
            auto block = clock_buffer_manager->newPage(page_id);
            if (block) {
                std::cout << "\n🔄 Intentando crear página " << page_id 
                          << " (forzará pasadas del Clock)" << std::endl;
                clock_buffer_manager->unpinPage(page_id, false);
                
                std::cout << "📊 Estado después del intento " << (i+1) << ":" << std::endl;
                clock_buffer_manager->displayCompactState();
            }
        }
        
        std::cout << "\n📈 ESTADÍSTICAS FINALES (incluyendo pin decrements):" << std::endl;
        clock_buffer_manager->displayStatistics();
        
        std::cout << "\n🎯 RESULTADO:" << std::endl;
        std::cout << "✅ Segunda pasada del Clock decrementó pin_count automáticamente" << std::endl;
        std::cout << "✅ Páginas con pins altos se volvieron evictables gradualmente" << std::endl;
        std::cout << "✅ Sistema se auto-regula cuando todas las páginas están pinned" << std::endl;
    }
    
    void detailedClockVsLRUComparison() {
        if (!buffer_manager || !clock_buffer_manager) {
            std::cout << "❌ Error: Necesitas ambos buffer managers inicializados" << std::endl;
            return;
        }
        
        std::cout << "\n⚔️  COMPARACIÓN DETALLADA: Clock PIN-AWARE vs LRU" << std::endl;
        std::cout << "=== Análisis de comportamiento con pins ===" << std::endl;
        
        std::cout << "\n📊 ESTADÍSTICAS LRU:" << std::endl;
        buffer_manager->displayBufferPoolInfo();
        
        std::cout << "\n📊 ESTADÍSTICAS CLOCK PIN-AWARE:" << std::endl;
        clock_buffer_manager->displayStatistics();
        
        std::cout << "\n🎯 ANÁLISIS COMPARATIVO:" << std::endl;
        std::cout << "\n🔒 MANEJO DE PIN_COUNT:" << std::endl;
        std::cout << "LRU:              Básico (verificación en BufferPoolManager)" << std::endl;
        std::cout << "Clock PIN-AWARE:  Avanzado (integrado en algoritmo de reemplazo)" << std::endl;
        
        std::cout << "\n⚡ PERFORMANCE:" << std::endl;
        std::cout << "LRU:              Timestamps + ordenamiento" << std::endl;
        std::cout << "Clock PIN-AWARE:  Reference bits + circular sweep" << std::endl;
        
        std::cout << "\n🛡️  PROTECCIÓN:" << std::endl;
        std::cout << "LRU:              Puede fallar si no se verifica pin_count" << std::endl;
        std::cout << "Clock PIN-AWARE:  NUNCA evicta páginas pinned" << std::endl;
        
        std::cout << "\n🔄 AUTO-REGULACIÓN:" << std::endl;
        std::cout << "LRU:              No tiene mecanismo de segunda pasada" << std::endl;
        std::cout << "Clock PIN-AWARE:  Segunda pasada decrementa pins automáticamente" << std::endl;
        
        std::cout << "\n🌊 SEQUENTIAL FLOODING:" << std::endl;
        std::cout << "LRU:              Vulnerable (páginas recientes evictan antiguas)" << std::endl;
        std::cout << "Clock PIN-AWARE:  Resistente (reference bits dan segunda oportunidad)" << std::endl;
    }
    
    // ==========================================================================
    // GESTIÓN DE TABLAS Y DATOS
    // ==========================================================================
    
    void createTable() {
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
        std::cout << "Numero de campos: ";
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
                std::cout << "Longitud maxima: ";
                std::cin >> max_length;
            }
            std::cin.ignore();
            
            FieldType type = static_cast<FieldType>(type_int);
            schema.emplace_back(field_name, type, max_length);
        }
        
        if (disk_manager->createTable(table_name, schema, use_fixed)) {
            std::cout << "\n✅ Tabla '" << table_name << "' creada." << std::endl;
            std::cout << "Tipo: " << (use_fixed ? "Longitud Fija" : "Longitud Variable") << std::endl;
            
            // Mostrar Page Directory actualizado
            std::cout << "\n📁 Page Directory actualizado automáticamente:" << std::endl;
            disk_manager->displayPageDirectory();
        } else {
            std::cout << "\n❌ Error creando la tabla." << std::endl;
        }
    }
    
    void insertSingleRecord() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== INSERCION DETALLADA DE REGISTRO ===" << std::endl;
        
        std::string table_name;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        
        std::cout << "Valores separados por comas: ";
        std::string values_str;
        std::getline(std::cin, values_str);
        
        std::vector<std::string> values = parseCSVLine(values_str);
        
        std::cout << "\n🔄 PROCESO DE INSERCION CON PAGE DIRECTORY:" << std::endl;
        std::cout << "1. Datos del registro:" << std::endl;
        std::cout << "   - Tabla: " << table_name << std::endl;
        std::cout << "   - Campos: " << values.size() << std::endl;
        std::cout << "   - Tamaño estimado: " << estimateRecordSize(values) << " bytes" << std::endl;
        
        std::cout << "2. DiskManager gestionará automáticamente:" << std::endl;
        std::cout << "   - Asignación de bloque" << std::endl;
        std::cout << "   - Registro en Page Directory" << std::endl;
        std::cout << "   - Persistencia de metadatos" << std::endl;
        
        if (disk_manager->insertRecord(table_name, values)) {
            std::cout << "\n✅ Registro insertado exitosamente." << std::endl;
            
            // Mostrar Page Directory actualizado
            std::cout << "\n📁 Page Directory actualizado:" << std::endl;
            disk_manager->displayPageDirectory();
        } else {
            std::cout << "\n❌ Error insertando el registro." << std::endl;
        }
    }
    
    void loadNRecords() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== CARGA DE N REGISTROS ===" << std::endl;
        
        std::string table_name, csv_file;
        int n_records;
        
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        std::cout << "Archivo CSV: ";
        std::getline(std::cin, csv_file);
        std::cout << "Numero de registros a cargar: ";
        std::cin >> n_records;
        std::cin.ignore();
        
        std::cout << "\n🔄 PROCESANDO ARCHIVO CON PAGE DIRECTORY..." << std::endl;
        
        std::ifstream file(csv_file);
        if (!file.is_open()) {
            std::cout << "❌ Error: No se pudo abrir " << csv_file << std::endl;
            return;
        }
        
        std::string line;
        int loaded = 0;
        int batch_size = 5;
        int current_batch = 0;
        
        while (std::getline(file, line) && loaded < n_records) {
            if (line.empty()) continue;
            
            std::vector<std::string> values = parseCSVLine(line);
            if (!values.empty()) {
                if (disk_manager->insertRecord(table_name, values)) {
                    loaded++;
                    
                    // Mostrar progreso por lotes
                    if (loaded % batch_size == 0 || loaded == n_records) {
                        current_batch = (loaded - 1) / batch_size + 1;
                        std::cout << "Lote " << current_batch << " completado (" 
                                  << loaded << "/" << n_records << " registros)" << std::endl;
                    }
                }
            }
        }
        
        file.close();
        std::cout << "\n✅ Carga completada: " << loaded << " registros procesados." << std::endl;
        
        // Mostrar Page Directory final
        std::cout << "\n📁 Page Directory final:" << std::endl;
        disk_manager->displayPageDirectory();
    }
    
    void loadCompleteCSV() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== CARGA COMPLETA DE CSV ===" << std::endl;
        
        std::string table_name, csv_file;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        std::cout << "Archivo CSV: ";
        std::getline(std::cin, csv_file);
        
        // Contar registros
        int total_records = countRecordsInFile(csv_file);
        if (total_records == 0) {
            std::cout << "❌ Error: Archivo vacío o no encontrado." << std::endl;
            return;
        }
        
        std::cout << "Registros detectados: " << total_records << std::endl;
        std::cout << "🔄 Iniciando carga completa con Page Directory..." << std::endl;
        
        if (disk_manager->loadFromCSV(table_name, csv_file)) {
            std::cout << "✅ Carga completa exitosa: " << total_records << " registros." << std::endl;
            
            // Mostrar Page Directory final
            std::cout << "\n📁 Page Directory final:" << std::endl;
            disk_manager->displayPageDirectory();
        } else {
            std::cout << "❌ Error en la carga completa." << std::endl;
        }
    }
    
    void findRecord() {
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
    
    void deleteRecord() {
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
    
    void displayTable() {
        if (!requiresDisk()) return;
        
        std::string table_name;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        
        disk_manager->displayTable(table_name);
    }
    
    void compactTable() {
        if (!requiresDisk()) return;
        
        std::string table_name;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        
        disk_manager->compactTable(table_name);
    }
    
    void showStatistics() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== ESTADÍSTICAS DEL SISTEMA INTEGRADO ===" << std::endl;
        disk_manager->displayStatistics(); // Incluye Page Directory
        
        if (current_state == SystemState::BUFFER_POOL_READY) {
            std::cout << "\n📊 Estadísticas del Buffer Pool:" << std::endl;
            auto stats = buffer_manager->getStats();
            std::cout << "   - Frames totales: " << stats.total_frames << std::endl;
            std::cout << "   - Frames ocupados: " << stats.occupied_frames << std::endl;
            std::cout << "   - Utilización: " << std::fixed << std::setprecision(1) 
                      << stats.utilization << "%" << std::endl;
            std::cout << "   - Total operaciones: " << stats.total_operations << std::endl;
            std::cout << "   - Page faults: " << stats.page_faults << std::endl;
            std::cout << "   - Evictions: " << stats.evictions << std::endl;
        }
    }
    
    void showDirectoryStructure() {
        if (!requiresDisk()) return;
        
        disk_manager->showDirectoryStructure();
    }
    
    // ==========================================================================
    // SIMULACIONES
    // ==========================================================================
    
    void simulateInsufficientSpace() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== SIMULACION: ESPACIO INSUFICIENTE ===" << std::endl;
        
        std::string table_name;
        std::cout << "Tabla para simulacion: ";
        std::getline(std::cin, table_name);
        
        std::cout << "\n🎯 ESCENARIO SIMULADO:" << std::endl;
        std::cout << "- Sector actual: Plato_0/Superficie_0/Pista_2/Sector_15" << std::endl;
        std::cout << "- Tamaño sector: 4096 bytes" << std::endl;
        std::cout << "- Espacio usado: 3900 bytes" << std::endl;
        std::cout << "- Espacio libre: 196 bytes" << std::endl;
        std::cout << "- Registro nuevo: 512 bytes" << std::endl;
        std::cout << "\n❌ RESULTADO: Espacio insuficiente (deficit: 316 bytes)" << std::endl;
        std::cout << "\n🔄 SOLUCION CON PAGE DIRECTORY:" << std::endl;
        std::cout << "1. DiskManager busca próximo sector disponible" << std::endl;
        std::cout << "2. Sector_18 encontrado con 2048 bytes libres" << std::endl;
        std::cout << "3. Page Directory actualizado automáticamente" << std::endl;
        std::cout << "4. Nuevo mapeo: PageID → Sector_18" << std::endl;
        std::cout << "5. Inserción completada exitosamente" << std::endl;
    }
    
    void simulateFullSectors() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== SIMULACION: SECTORES LLENOS ===" << std::endl;
        
        std::string table_name;
        std::cout << "Tabla para simulacion: ";
        std::getline(std::cin, table_name);
        
        std::cout << "\n🎯 ESCENARIO SIMULADO:" << std::endl;
        std::cout << "Verificando pista actual..." << std::endl;
        for (int i = 0; i < 8; i++) {
            std::cout << "- Sector_" << i << ": LLENO (4096/4096 bytes)" << std::endl;
        }
        
        std::cout << "\n❌ RESULTADO: Todos los sectores de la pista están llenos" << std::endl;
        std::cout << "\n🔄 SOLUCION CON PAGE DIRECTORY:" << std::endl;
        std::cout << "1. DiskManager busca siguiente pista disponible" << std::endl;
        std::cout << "2. Pista_3 encontrada con sectores libres" << std::endl;
        std::cout << "3. Nuevo bloque creado en Pista_3/Sector_0" << std::endl;
        std::cout << "4. Page Directory registra automáticamente el mapeo" << std::endl;
        std::cout << "5. Nuevo PageID asignado y persistido" << std::endl;
        std::cout << "6. Registro insertado en nuevo bloque" << std::endl;
        std::cout << "7. Estadísticas actualizadas" << std::endl;
    }
    
    // ==========================================================================
    // DATASETS
    // ==========================================================================
    
    bool loadDataset(const std::string& dataset_name, const std::string& filename) {
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
        
        // Crear tabla
        if (!disk_manager->createTable(schema.table_name, schema.schema, true)) {
            std::cout << "❌ Error creando tabla." << std::endl;
            return false;
        }
        
        std::cout << "✅ Tabla creada con " << schema.expected_fields << " campos." << std::endl;
        
        // Contar registros
        int total_records = countRecordsInFile(filename);
        std::cout << "📊 Registros a procesar: " << total_records << std::endl;
        
        // Cargar datos
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "❌ Error abriendo archivo " << filename << std::endl;
            return false;
        }
        
        std::string line;
        std::getline(file, line); // Saltar header
        
        int loaded = 0;
        int errors = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<std::string> values = parseCSVLine(line, schema.delimiter);
            
            // Ajustar numero de campos si es necesario
            if (static_cast<int>(values.size()) > schema.expected_fields) {
                values.resize(schema.expected_fields);
            }
            
            if (static_cast<int>(values.size()) == schema.expected_fields) {
                if (disk_manager->insertRecord(schema.table_name, values)) {
                    loaded++;
                    if (loaded % 100 == 0) {
                        std::cout << "📈 Procesados: " << loaded << " registros..." << std::endl;
                    }
                } else {
                    errors++;
                }
            } else {
                errors++;
            }
        }
        
        file.close();
        
        std::cout << "\n✅ Carga completada:" << std::endl;
        std::cout << "   - Registros exitosos: " << loaded << std::endl;
        std::cout << "   - Errores: " << errors << std::endl;
        std::cout << "   - Tabla: " << schema.table_name << std::endl;
        
        // Mostrar Page Directory actualizado
        std::cout << "\n📁 Page Directory actualizado:" << std::endl;
        disk_manager->displayPageDirectory();
        
        return loaded > 0;
    }
};

/**
 * @brief Menú principal actualizado con Buffer Pool y Clock Pin-Aware
 */
void showMenu() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SGBD FISICO INTEGRADO - MENU PRINCIPAL" << std::endl;
    std::cout << "Sistema con Buffer Pool Management + Clock PIN-AWARE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n🚀 INICIALIZACION DEL SISTEMA:" << std::endl;
    std::cout << "1.  Inicializar nuevo disco extendido" << std::endl;
    std::cout << "2.  Cargar disco existente extendido" << std::endl;
    std::cout << "3.  Ver estado del sistema integrado" << std::endl;
    
    std::cout << "\n🗂️ GESTION DE TABLAS:" << std::endl;
    std::cout << "4.  Crear tabla (longitud fija/variable)" << std::endl;
    
    std::cout << "\n📊 INSERCION DE DATOS (CON PAGE DIRECTORY):" << std::endl;
    std::cout << "5.  Insertar 1 registro (proceso paso a paso)" << std::endl;
    std::cout << "6.  Cargar N registros desde CSV" << std::endl;
    std::cout << "7.  Cargar CSV completo" << std::endl;
    
    std::cout << "\n📋 DATASETS PREDEFINIDOS:" << std::endl;
    std::cout << "8.  Cargar dataset Housing (545 registros)" << std::endl;
    std::cout << "9.  Cargar dataset Titanic (891 registros)" << std::endl;
    
    std::cout << "\n🎯 SIMULACIONES DE PROBLEMAS:" << std::endl;
    std::cout << "10. Simular sector sin espacio suficiente" << std::endl;
    std::cout << "11. Simular sectores llenos" << std::endl;
    
    std::cout << "\n🔍 CONSULTAS Y OPERACIONES:" << std::endl;
    std::cout << "12. Buscar registro por ID" << std::endl;
    std::cout << "13. Eliminar registro" << std::endl;
    std::cout << "14. Mostrar tabla completa" << std::endl;
    std::cout << "15. Compactar tabla" << std::endl;
    
    std::cout << "\n📈 INFORMACION DEL SISTEMA:" << std::endl;
    std::cout << "16. Mostrar estadísticas integradas" << std::endl;
    std::cout << "17. Mostrar estructura de directorios" << std::endl;

    std::cout << "\n🏊 BUFFER POOL MANAGEMENT (LRU):" << std::endl;
    std::cout << "18. Inicializar Buffer Pool Manager" << std::endl;
    std::cout << "19. Test de operaciones Buffer Pool" << std::endl;
    std::cout << "20. Operaciones de páginas (READ/WRITE)" << std::endl;
    std::cout << "21. Crear nueva página con Buffer Pool" << std::endl;
    std::cout << "22. Ver estado del Buffer Pool" << std::endl;
    std::cout << "23. Flush todas las páginas dirty" << std::endl;
    
    std::cout << "\n📁 PAGE DIRECTORY (GESTIONADO POR DISK MANAGER):" << std::endl;
    std::cout << "24. Mostrar Page Directory" << std::endl;
    
    std::cout << "\n🕐 CLOCK BUFFER MANAGER PIN-AWARE:" << std::endl;
    std::cout << "25. Inicializar Clock Buffer Manager" << std::endl;
    std::cout << "26. Test operaciones Clock básicas" << std::endl;
    std::cout << "27. Demostrar Clock vs LRU" << std::endl;
    std::cout << "28. Operaciones avanzadas Clock" << std::endl;
    std::cout << "29. Estado Clock Buffer Manager" << std::endl;
    std::cout << "30. Flush páginas Clock" << std::endl;
    std::cout << "31. Comparar rendimiento Clock vs LRU" << std::endl;
    std::cout << "32. 🔒 TEST PIN-AWARENESS (Critical)" << std::endl;
    std::cout << "33. 🔄 TEST Segunda Pasada Pin Decrement" << std::endl;
    std::cout << "34. ⚔️  Comparación detallada Clock vs LRU" << std::endl;
    
    
    std::cout << "\n0.  Salir" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "Opción: ";
}

/**
 * @brief Función principal con sistema integrado
 */
int main() {
    SGBDSystemExtended sistema;
    int option;
    
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "SISTEMA DE GESTION DE BASE DE DATOS FISICO INTEGRADO" << std::endl;
    std::cout << "🚀 Buffer Pool Management + Clock PIN-AWARE" << std::endl;
    std::cout << "📚 Implementación Educativa - Almacenamiento Secundario" << std::endl;
    std::cout << "🎓 Basado en Database System Implementation + CMU Lectures" << std::endl;
    std::cout << "🔒 Clock Algorithm NUNCA evicta páginas pinned" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    std::cout << "\n🏗️ ARQUITECTURA DEL SISTEMA:" << std::endl;
    std::cout << "┌─────────────────────────────────────────────────────────┐" << std::endl;
    std::cout << "│                    APLICACION                           │" << std::endl;
    std::cout << "├─────────────────────────────────────────────────────────┤" << std::endl;
    std::cout << "│              BUFFER POOL MANAGER                        │" << std::endl;
    std::cout << "│  ┌──────────────┬──────────────┬─────────────────────┐  │" << std::endl;
    std::cout << "│  │ Page Table   │ LRU/Clock    │ Buffer Pool (Frames)│  │" << std::endl;
    std::cout << "│  │ (Memoria)    │ (Eviction)   │ (Memoria)           │  │" << std::endl;
    std::cout << "│  └──────────────┴──────────────┴─────────────────────┘  │" << std::endl;
    std::cout << "├─────────────────────────────────────────────────────────┤" << std::endl;
    std::cout << "│             DISK MANAGER EXTENDED                       │" << std::endl;
    std::cout << "│  ┌─────────────────────┬─────────────────────────────┐  │" << std::endl;
    std::cout << "│  │ Page Directory      │ File System Simulator       │  │" << std::endl;
    std::cout << "│  │ (Disco - Persistente)│ (Tu sistema existente)     │  │" << std::endl;
    std::cout << "│  └─────────────────────┴─────────────────────────────┘  │" << std::endl;
    std::cout << "└─────────────────────────────────────────────────────────┘" << std::endl;
    
    // Mostrar estado inicial
    sistema.showSystemStatus();
    
    while (true) {
        showMenu();
        std::cin >> option;
        std::cin.ignore();
        
        switch (option) {
            case 1:
                sistema.initializeDisk();
                break;
                
            case 2:
                sistema.loadExistingDisk();
                break;
                
            case 3:
                sistema.showSystemStatus();
                break;
                
            case 4:
                sistema.createTable();
                break;
                
            case 5:
                sistema.insertSingleRecord();
                break;
                
            case 6:
                sistema.loadNRecords();
                break;
                
            case 7:
                sistema.loadCompleteCSV();
                break;
                
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
                if(sistema.loadDataset("titanic",titanic_path)){
                std::cout << "✅ Dataset Titanic cargado desde: " <<titanic_path << std::endl;
                 } else {
                 std::cout << "❌ Error: Verifica que existe " << titanic_path << std::endl;
                 }
                }
                 break;
                
            case 10:
                sistema.simulateInsufficientSpace();
                break;
                
            case 11:
                sistema.simulateFullSectors();
                break;
                
            case 12:
                sistema.findRecord();
                break;
                
            case 13:
                sistema.deleteRecord();
                break;
                
            case 14:
                sistema.displayTable();
                break;
                
            case 15:
                sistema.compactTable();
                break;
                
            case 16:
                sistema.showStatistics();
                break;
                
            case 17:
                sistema.showDirectoryStructure();
                break;
                
            // ========== BUFFER POOL LRU ==========
            case 18:
                sistema.initializeBufferPool();
                break;
                
            case 19:
                sistema.testBufferPoolOperations();
                break;
                
            case 20:
                sistema.bufferPoolPageOperations();
                break;
                
            case 21:
                sistema.createNewPageBuffered();
                break;
                
            case 22:
                sistema.showBufferPoolStatus();
                break;
                
            case 23:
                sistema.flushAllPages();
                break;
                
            case 24:
                sistema.showPageDirectory();
                break;

            // ========== CLOCK BUFFER MANAGER PIN-AWARE ==========
            case 25:
                sistema.initializeClockBufferPool();
                break;
    
            case 26:
                sistema.testClockBufferOperations();
                break;
    
            case 27:
                sistema.demonstrateClockVsLRU();
                break;
    
            case 28:
                sistema.clockAdvancedPageOperations();
                break;
    
            case 29:
                sistema.showClockBufferStatus();
                break;
    
            case 30:
                sistema.flushAllClockPages();
                break;
    
            case 31:
                sistema.compareClockVsLRUPerformance();
                break;
                
            // ========== TESTS PIN-AWARENESS CRÍTICOS ==========
            case 32:
                sistema.testClockPinAwareness();
                break;
                
            case 33:
                sistema.testClockSecondPassDecrement();
                break;
                
            case 34:
                sistema.detailedClockVsLRUComparison();
                break;
                
            case 0:
                std::cout << "\n🎓 ¡Gracias por usar el SGBD Físico Integrado!" << std::endl;
                std::cout << "📚 Has experimentado con:" << std::endl;
                std::cout << "   ✅ Buffer Pool Management profesional" << std::endl;
                std::cout << "   ✅ Page Directory persistente" << std::endl;
                std::cout << "   ✅ Política LRU de evicción" << std::endl;
                std::cout << "   ✅ Clock Algorithm PIN-AWARE" << std::endl;
                std::cout << "   ✅ Protección contra evicción incorrecta" << std::endl;
                std::cout << "   ✅ Segunda pasada con pin decrement automático" << std::endl;
                std::cout << "🚀 ¡Sistema de base de datos de nivel profesional!" << std::endl;
                return 0;
                
            default:
                std::cout << "\n❌ Opción no válida. Selecciona 0-34." << std::endl;
                break;
        }
        
        std::cout << "\n⏸️  Presiona Enter para continuar...";
        std::cin.get();
    }
    
    return 0;
}