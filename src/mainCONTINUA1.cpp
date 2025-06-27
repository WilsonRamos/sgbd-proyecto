#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <map>
#include "DiskManager.h"

/**
 * @brief Estado del sistema
 */
enum class SystemState {
    NOT_INITIALIZED,
    DISK_READY,
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
 * @brief Clase principal del sistema sin animaciones
 */
class SGBDSystem {
private:
    DiskManager disk_manager;
    SystemState current_state;
    std::string disk_path;
    
public:
    SGBDSystem(const std::string& path = "./mi_disco_sgbd") 
        : disk_manager(path), current_state(SystemState::NOT_INITIALIZED), disk_path(path) {}
    
    SystemState getState() const { return current_state; }
    
    void showSystemStatus() {
        std::cout << "\n" << std::string(50, '-') << std::endl;
        std::cout << "ESTADO DEL SISTEMA:" << std::endl;
        
        switch (current_state) {
            case SystemState::NOT_INITIALIZED:
                std::cout << "Estado: NO INICIALIZADO" << std::endl;
                std::cout << "Disco: No creado" << std::endl;
                std::cout << "Accion requerida: Inicializar disco (opcion 1)" << std::endl;
                break;
                
            case SystemState::DISK_READY:
                std::cout << "Estado: LISTO" << std::endl;
                std::cout << "Disco: " << disk_path << std::endl;
                std::cout << "Accion: Sistema listo para operaciones" << std::endl;
                break;
                
            case SystemState::ERROR_STATE:
                std::cout << "Estado: ERROR" << std::endl;
                std::cout << "Accion requerida: Reinicializar sistema" << std::endl;
                break;
        }
        std::cout << std::string(50, '-') << std::endl;
    }
    
    bool initializeDisk() {
        std::cout << "\n=== INICIALIZACION DEL DISCO ===" << std::endl;
        
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
        
        if (disk_manager.initialize(config)) {
            current_state = SystemState::DISK_READY;
            std::cout << "\nDisco inicializado exitosamente en: " << disk_path << std::endl;
            return true;
        } else {
            current_state = SystemState::ERROR_STATE;
            std::cout << "\nError inicializando el disco." << std::endl;
            return false;
        }
    }
    
    bool loadExistingDisk() {
        std::cout << "\n=== CARGANDO DISCO EXISTENTE ===" << std::endl;
        
        if (disk_manager.loadExistingDisk()) {
            current_state = SystemState::DISK_READY;
            std::cout << "Disco cargado desde: " << disk_path << std::endl;
            return true;
        } else {
            std::cout << "Error: No se encontro disco en " << disk_path << std::endl;
            return false;
        }
    }
    
    void showDiskStructure(const DiskConfig& config) {
        std::cout << "\n=== ESTRUCTURA DEL DISCO ===" << std::endl;
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
        
        std::cout << "\nJerarquia fisica:" << std::endl;
        std::cout << "DISCO" << std::endl;
        std::cout << "  |- PLATO_0" << std::endl;
        std::cout << "      |- SUPERFICIE_0" << std::endl;
        std::cout << "          |- PISTA_0 (" << config.getSectorsPerTrack() << " sectores)" << std::endl;
        std::cout << "          |- PISTA_1" << std::endl;
        std::cout << "          |- ..." << std::endl;
        if (config.getNumPlatters() > 1) {
            std::cout << "  |- PLATO_1..." << std::endl;
        }
    }
    
    bool requiresDisk() {
        if (current_state != SystemState::DISK_READY) {
            std::cout << "\nERROR: Operacion requiere disco inicializado." << std::endl;
            std::cout << "Ejecuta primero la opcion 1 o 2." << std::endl;
            return false;
        }
        return true;
    }
    
    // Inserción detallada de 1 registro
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
        
        std::cout << "\nPROCESO DE INSERCION:" << std::endl;
        std::cout << "1. Datos del registro:" << std::endl;
        std::cout << "   - Tabla: " << table_name << std::endl;
        std::cout << "   - Campos: " << values.size() << std::endl;
        std::cout << "   - Tamaño estimado: " << estimateRecordSize(values) << " bytes" << std::endl;
        
        std::cout << "2. Buscando bloque con espacio..." << std::endl;
        std::cout << "3. Verificando capacidad del sector..." << std::endl;
        std::cout << "4. Serializando registro..." << std::endl;
        std::cout << "5. Escribiendo a disco..." << std::endl;
        
        if (disk_manager.insertRecord(table_name, values)) {
            std::cout << "\nRegistro insertado exitosamente." << std::endl;
        } else {
            std::cout << "\nError insertando el registro." << std::endl;
        }
    }
    
    // Carga N registros
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
        
        std::cout << "\nPROCESANDO ARCHIVO..." << std::endl;
        
        std::ifstream file(csv_file);
        if (!file.is_open()) {
            std::cout << "Error: No se pudo abrir " << csv_file << std::endl;
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
                if (disk_manager.insertRecord(table_name, values)) {
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
        std::cout << "\nCarga completada: " << loaded << " registros procesados." << std::endl;
    }
    
    // Carga completa CSV
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
            std::cout << "Error: Archivo vacio o no encontrado." << std::endl;
            return;
        }
        
        std::cout << "Registros detectados: " << total_records << std::endl;
        std::cout << "Iniciando carga completa..." << std::endl;
        
        if (disk_manager.loadFromCSV(table_name, csv_file)) {
            std::cout << "Carga completa exitosa: " << total_records << " registros." << std::endl;
        } else {
            std::cout << "Error en la carga completa." << std::endl;
        }
    }
    
    // Simulación simple de espacio insuficiente
    void simulateInsufficientSpace() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== SIMULACION: ESPACIO INSUFICIENTE ===" << std::endl;
        
        std::string table_name;
        std::cout << "Tabla para simulacion: ";
        std::getline(std::cin, table_name);
        
        std::cout << "\nESCENARIO SIMULADO:" << std::endl;
        std::cout << "- Sector actual: Plato_0/Superficie_0/Pista_2/Sector_15" << std::endl;
        std::cout << "- Tamaño sector: 4096 bytes" << std::endl;
        std::cout << "- Espacio usado: 3900 bytes" << std::endl;
        std::cout << "- Espacio libre: 196 bytes" << std::endl;
        std::cout << "- Registro nuevo: 512 bytes" << std::endl;
        std::cout << "\nRESULTADO: Espacio insuficiente (deficit: 316 bytes)" << std::endl;
        std::cout << "\nSOLUCION:" << std::endl;
        std::cout << "1. Buscar proximo sector disponible" << std::endl;
        std::cout << "2. Sector_18 encontrado con 2048 bytes libres" << std::endl;
        std::cout << "3. Registro asignado al nuevo sector" << std::endl;
        std::cout << "4. Insercion completada exitosamente" << std::endl;
    }
    
    // Simulación simple de sectores llenos
    void simulateFullSectors() {
        if (!requiresDisk()) return;
        
        std::cout << "\n=== SIMULACION: SECTORES LLENOS ===" << std::endl;
        
        std::string table_name;
        std::cout << "Tabla para simulacion: ";
        std::getline(std::cin, table_name);
        
        std::cout << "\nESCENARIO SIMULADO:" << std::endl;
        std::cout << "Verificando pista actual..." << std::endl;
        for (int i = 0; i < 8; i++) {
            std::cout << "- Sector_" << i << ": LLENO (4096/4096 bytes)" << std::endl;
        }
        
        std::cout << "\nRESULTADO: Todos los sectores de la pista estan llenos" << std::endl;
        std::cout << "\nSOLUCION:" << std::endl;
        std::cout << "1. Buscar siguiente pista disponible" << std::endl;
        std::cout << "2. Pista_3 encontrada con sectores libres" << std::endl;
        std::cout << "3. Nuevo bloque creado en Pista_3/Sector_0" << std::endl;
        std::cout << "4. Registro insertado en nuevo bloque" << std::endl;
        std::cout << "5. Estadisticas actualizadas" << std::endl;
    }
    
    // Cargar datasets predefinidos
    bool loadDataset(const std::string& dataset_name, const std::string& filename) {
        if (!requiresDisk()) return false;
        
        auto datasets = getDatasetSchemas();
        auto it = datasets.find(dataset_name);
        
        if (it == datasets.end()) {
            std::cout << "Dataset " << dataset_name << " no encontrado." << std::endl;
            return false;
        }
        
        const DatasetSchema& schema = it->second;
        
        std::cout << "\n=== CARGANDO DATASET " << dataset_name << " ===" << std::endl;
        std::cout << "Descripcion: " << schema.description << std::endl;
        std::cout << "Tabla destino: " << schema.table_name << std::endl;
        
        // Crear tabla
        if (!disk_manager.createTable(schema.table_name, schema.schema, true)) {
            std::cout << "Error creando tabla." << std::endl;
            return false;
        }
        
        std::cout << "Tabla creada con " << schema.expected_fields << " campos." << std::endl;
        
        // Contar registros
        int total_records = countRecordsInFile(filename);
        std::cout << "Registros a procesar: " << total_records << std::endl;
        
        // Cargar datos
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error abriendo archivo " << filename << std::endl;
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
                if (disk_manager.insertRecord(schema.table_name, values)) {
                    loaded++;
                    if (loaded % 100 == 0) {
                        std::cout << "Procesados: " << loaded << " registros..." << std::endl;
                    }
                } else {
                    errors++;
                }
            } else {
                errors++;
            }
        }
        
        file.close();
        
        std::cout << "\nCarga completada:" << std::endl;
        std::cout << "- Registros exitosos: " << loaded << std::endl;
        std::cout << "- Errores: " << errors << std::endl;
        std::cout << "- Tabla: " << schema.table_name << std::endl;
        
        return loaded > 0;
    }
    
    // Operaciones del DiskManager
    void findRecord() {
        if (!requiresDisk()) return;
        
        std::string table_name;
        int record_id;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        std::cout << "ID del registro: ";
        std::cin >> record_id;
        std::cin.ignore();
        
        auto record = disk_manager.findRecord(table_name, record_id);
        if (record) {
            std::cout << "\nRegistro encontrado:" << std::endl;
            record->display();
        } else {
            std::cout << "\nRegistro no encontrado." << std::endl;
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
        
        if (disk_manager.deleteRecord(table_name, record_id)) {
            std::cout << "Registro eliminado exitosamente." << std::endl;
        } else {
            std::cout << "Error eliminando el registro." << std::endl;
        }
    }
    
    void displayTable() {
        if (!requiresDisk()) return;
        
        std::string table_name;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        
        disk_manager.displayTable(table_name);
    }
    
    void compactTable() {
        if (!requiresDisk()) return;
        
        std::string table_name;
        std::cout << "Nombre de la tabla: ";
        std::getline(std::cin, table_name);
        
        disk_manager.compactTable(table_name);
    }
    
    void showStatistics() {
        if (!requiresDisk()) return;
        
        disk_manager.displayStatistics();
    }
    
    void showDirectoryStructure() {
        if (!requiresDisk()) return;
        
        disk_manager.showDirectoryStructure();
    }
    
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
        
        if (disk_manager.createTable(table_name, schema, use_fixed)) {
            std::cout << "\nTabla '" << table_name << "' creada." << std::endl;
            std::cout << "Tipo: " << (use_fixed ? "Longitud Fija" : "Longitud Variable") << std::endl;
        } else {
            std::cout << "\nError creando la tabla." << std::endl;
        }
    }

private:
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
};

/**
 * @brief Menú principal limpio
 */
void showMenu() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "SGBD FISICO - MENU PRINCIPAL" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\nINICIALIZACION DEL SISTEMA:" << std::endl;
    std::cout << "1.  Inicializar nuevo disco" << std::endl;
    std::cout << "2.  Cargar disco existente" << std::endl;
    std::cout << "3.  Ver estado del sistema" << std::endl;
    
    std::cout << "\nGESTION DE TABLAS:" << std::endl;
    std::cout << "4.  Crear tabla (longitud fija/variable)" << std::endl;
    
    std::cout << "\nINSERCION DE DATOS (CON PROCESOS DETALLADOS):" << std::endl;
    std::cout << "5.  Insertar 1 registro (proceso paso a paso)" << std::endl;
    std::cout << "6.  Cargar N registros desde CSV" << std::endl;
    std::cout << "7.  Cargar CSV completo" << std::endl;
    
    std::cout << "\nDATASETS PREDEFINIDOS:" << std::endl;
    std::cout << "8.  Cargar dataset Housing (545 registros)" << std::endl;
    std::cout << "9.  Cargar dataset Titanic (891 registros)" << std::endl;
    
    std::cout << "\nSIMULACIONES DE PROBLEMAS:" << std::endl;
    std::cout << "10. Simular sector sin espacio suficiente" << std::endl;
    std::cout << "11. Simular sectores llenos" << std::endl;
    
    std::cout << "\nCONSULTAS Y OPERACIONES:" << std::endl;
    std::cout << "12. Buscar registro por ID" << std::endl;
    std::cout << "13. Eliminar registro" << std::endl;
    std::cout << "14. Mostrar tabla completa" << std::endl;
    std::cout << "15. Compactar tabla" << std::endl;
    
    std::cout << "\nINFORMACION DEL SISTEMA:" << std::endl;
    std::cout << "16. Mostrar estadisticas del disco" << std::endl;
    std::cout << "17. Mostrar estructura de directorios" << std::endl;
    
    std::cout << "\n0.  Salir" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Opcion: ";
}

/**
 * @brief Función principal con flujo coherente
 */
int main() {
    SGBDSystem sistema;
    int option;
    
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "SISTEMA DE GESTION DE BASE DE DATOS FISICO" << std::endl;
    std::cout << "Implementacion Educativa - Almacenamiento Secundario" << std::endl;
    std::cout << "Basado en Database System Implementation - Capitulo 13" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
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
                sistema.loadDataset("housing", "Housing.csv");
                break;
                
            case 9:
                sistema.loadDataset("titanic", "titanic.csv");
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
                
            case 0:
                std::cout << "\nGracias por usar el SGBD Fisico!" << std::endl;
                return 0;
                
            default:
                std::cout << "\nOpcion no valida. Selecciona 0-17." << std::endl;
                break;
        }
        
        std::cout << "\nPresiona Enter para continuar...";
        std::cin.ignore();
    }
    
    return 0;
}