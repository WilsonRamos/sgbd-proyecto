#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include "DiskConfig.h"
#include "FileSystemSimulator.h"
#include "Block.h"
#include "Record.h"
#include "PhysicalAddress.h"

/**
 * @brief Gestor principal del SGBD físico
 * 
 * Coordina todas las operaciones del disco, incluyendo:
 * - Gestión de bloques y registros
 * - Asignación de espacio
 * - Simulación de tiempos de acceso
 * - Operaciones CRUD básicas
 */
class DiskManager {
private:
    DiskConfig config;
    FileSystemSimulator filesystem;
    std::map<PhysicalAddress, std::shared_ptr<Block>> block_cache;  // Cache de bloques
    std::map<std::string, std::vector<PhysicalAddress>> relation_blocks;  // Bloques por relación
    PhysicalAddress next_free_address;
    int next_record_id;
    
    // Estadísticas
    size_t total_reads;
    size_t total_writes;
    double total_access_time;

public:
    /**
     * @brief Constructor
     */
    DiskManager(const std::string& disk_path = "./disk_simulation") 
        : filesystem(disk_path)
        , next_free_address(0, 0, 0, 0)
        , next_record_id(1)
        , total_reads(0)
        , total_writes(0)
        , total_access_time(0.0)
    {
    }

    /**
     * @brief Inicializa el disco con configuración personalizada
     */
    bool initialize(const DiskConfig& disk_config) {
        config = disk_config;
        
        if (!filesystem.initialize(config)) {
            return false;
        }
        
        std::cout << "Disco inicializado correctamente." << std::endl;
        config.displayConfig();
        
        return true;
    }

    /**
     * @brief Carga un disco existente
     */
    bool loadExistingDisk() {
        if (!filesystem.loadExisting()) {
            return false;
        }
        
        config = filesystem.getDiskConfig();
        loadBlockIndex();
        
        std::cout << "Disco cargado correctamente." << std::endl;
        return true;
    }

    /**
     * @brief Crea una nueva tabla/relación
     */
    bool createTable(const std::string& table_name, 
                     const std::vector<FieldDefinition>& schema,
                     bool use_fixed_records = true) {
        
        if (relation_blocks.find(table_name) != relation_blocks.end()) {
            std::cout << "La tabla '" << table_name << "' ya existe." << std::endl;
            return false;
        }
        
        // Crear primer bloque para la tabla
        PhysicalAddress addr = allocateNewBlock();
        auto block = std::make_shared<Block>(addr, config.getBytesPerSector());
        block->setRelationName(table_name);
        
        // Guardar información del esquema en metadatos
        saveTableSchema(table_name, schema, use_fixed_records);
        
        // Registrar el bloque
        block_cache[addr] = block;
        relation_blocks[table_name].push_back(addr);
        
        // Escribir bloque vacío al disco
        filesystem.writeBlock(addr, *block);
        
        std::cout << "Tabla '" << table_name << "' creada exitosamente." << std::endl;
        return true;
    }

    /**
     * @brief Inserta un registro en una tabla
     */
    bool insertRecord(const std::string& table_name, 
                      const std::vector<std::string>& values) {
        
        auto schema = loadTableSchema(table_name);
        if (schema.empty()) {
            std::cout << "Tabla '" << table_name << "' no encontrada." << std::endl;
            return false;
        }
        
        // Crear el registro apropiado
        std::shared_ptr<Record> record;
        bool use_fixed = isTableFixedRecord(table_name);
        
        if (use_fixed) {
            auto fixed_record = std::make_shared<FixedRecord>(next_record_id++);
            fixed_record->setSchema(schema);
            fixed_record->setFieldValues(values);
            fixed_record->calculateFixedSize();
            record = fixed_record;
        } else {
            auto variable_record = std::make_shared<VariableRecord>(next_record_id++);
            variable_record->setSchema(schema);
            variable_record->setFieldValues(values);
            variable_record->calculateOffsets();
            record = variable_record;
        }
        
        // Encontrar bloque con espacio disponible
        auto block = findBlockWithSpace(table_name, record->getSize());
        if (!block) {
            // Crear nuevo bloque
            PhysicalAddress addr = allocateNewBlock();
            block = std::make_shared<Block>(addr, config.getBytesPerSector());
            block->setRelationName(table_name);
            block_cache[addr] = block;
            relation_blocks[table_name].push_back(addr);
        }
        
        // Insertar el registro
        if (block->addRecord(record)) {
            // Simular tiempo de escritura
            double access_time = simulateAccessTime(block->getAddress());
            total_access_time += access_time;
            total_writes++;
            
            // Escribir bloque al disco
            filesystem.writeBlock(block->getAddress(), *block);
            
            std::cout << "Registro insertado en tabla '" << table_name 
                      << "' (ID: " << record->getId() << ", Tiempo: " 
                      << access_time << " ms)" << std::endl;
            return true;
        }
        
        std::cout << "Error: No se pudo insertar el registro." << std::endl;
        return false;
    }

    /**
     * @brief Carga registros desde un archivo CSV
     */
    bool loadFromCSV(const std::string& table_name, const std::string& csv_file) {
        std::ifstream file(csv_file);
        if (!file.is_open()) {
            std::cout << "Error: No se pudo abrir el archivo " << csv_file << std::endl;
            return false;
        }
        
        std::string line;
        int records_loaded = 0;
        
        // Saltar header si existe
        if (std::getline(file, line)) {
            // Procesar primera línea como datos
        }
        
        // Procesar cada línea
        do {
            std::vector<std::string> values = parseCSVLine(line);
            if (!values.empty()) {
                if (insertRecord(table_name, values)) {
                    records_loaded++;
                }
            }
        } while (std::getline(file, line));
        
        file.close();
        
        std::cout << "Cargados " << records_loaded << " registros desde " 
                  << csv_file << std::endl;
        return records_loaded > 0;
    }

    /**
     * @brief Busca un registro por ID
     */
    std::shared_ptr<Record> findRecord(const std::string& table_name, int record_id) {
        auto it = relation_blocks.find(table_name);
        if (it == relation_blocks.end()) {
            return nullptr;
        }
        
        // Buscar en todos los bloques de la tabla
        for (const auto& addr : it->second) {
            auto block = getBlock(addr);
            if (block) {
                auto record = block->findRecord(record_id);
                if (record) {
                    // Simular tiempo de lectura
                    double access_time = simulateAccessTime(addr);
                    total_access_time += access_time;
                    total_reads++;
                    
                    return record;
                }
            }
        }
        
        return nullptr;
    }

    /**
     * @brief Elimina un registro lógicamente
     */
    bool deleteRecord(const std::string& table_name, int record_id) {
        auto it = relation_blocks.find(table_name);
        if (it == relation_blocks.end()) {
            return false;
        }
        
        // Buscar en todos los bloques de la tabla
        for (const auto& addr : it->second) {
            auto block = getBlock(addr);
            if (block && block->deleteRecord(record_id)) {
                // Simular tiempo de escritura
                double access_time = simulateAccessTime(addr);
                total_access_time += access_time;
                total_writes++;
                
                // Escribir bloque modificado
                filesystem.writeBlock(addr, *block);
                
                std::cout << "Registro " << record_id << " eliminado lógicamente." << std::endl;
                return true;
            }
        }
        
        return false;
    }

    /**
     * @brief Compacta una tabla eliminando registros marcados como eliminados
     */
    void compactTable(const std::string& table_name) {
        auto it = relation_blocks.find(table_name);
        if (it == relation_blocks.end()) {
            std::cout << "Tabla '" << table_name << "' no encontrada." << std::endl;
            return;
        }
        
        int compacted_blocks = 0;
        for (const auto& addr : it->second) {
            auto block = getBlock(addr);
            if (block) {
                size_t old_count = block->getRecordCount();
                block->compactBlock();
                size_t new_count = block->getRecordCount();
                
                if (old_count != new_count) {
                    filesystem.writeBlock(addr, *block);
                    compacted_blocks++;
                }
            }
        }
        
        std::cout << "Compactación completada. " << compacted_blocks 
                  << " bloques procesados." << std::endl;
    }

    /**
     * @brief Muestra todos los registros de una tabla
     */
    void displayTable(const std::string& table_name) {
        auto it = relation_blocks.find(table_name);
        if (it == relation_blocks.end()) {
            std::cout << "Tabla '" << table_name << "' no encontrada." << std::endl;
            return;
        }
        
        std::cout << "\n=== TABLA: " << table_name << " ===" << std::endl;
        
        int total_records = 0;
        int active_records = 0;
        
        for (const auto& addr : it->second) {
            auto block = getBlock(addr);
            if (block) {
                std::cout << "\n--- Bloque " << addr << " ---" << std::endl;
                block->displayInfo();
                
                auto active_recs = block->getActiveRecords();
                for (const auto& record : active_recs) {
                    record->display();
                    std::cout << "---" << std::endl;
                }
                
                total_records += block->getRecordCount();
                active_records += active_recs.size();
            }
        }
        
        std::cout << "\nResumen: " << active_records << " registros activos de " 
                  << total_records << " totales." << std::endl;
    }

    /**
     * @brief Muestra estadísticas del disco
     */
    void displayStatistics() {
        std::cout << "\n=== ESTADÍSTICAS DEL DISCO ===" << std::endl;
        
        config.displayConfig();
        filesystem.displayUsageStatistics();
        
        std::cout << "\n=== ESTADÍSTICAS DE ACCESO ===" << std::endl;
        std::cout << "Total de lecturas: " << total_reads << std::endl;
        std::cout << "Total de escrituras: " << total_writes << std::endl;
        std::cout << "Tiempo total de acceso: " << total_access_time << " ms" << std::endl;
        
        if (total_reads + total_writes > 0) {
            std::cout << "Tiempo promedio de acceso: " 
                      << (total_access_time / (total_reads + total_writes)) 
                      << " ms" << std::endl;
        }
        
        std::cout << "\n=== TABLAS ===" << std::endl;
        for (const auto& table : relation_blocks) {
            std::cout << "- " << table.first << ": " << table.second.size() 
                      << " bloques" << std::endl;
        }
    }

    /**
     * @brief Muestra la estructura de directorios
     */
    void showDirectoryStructure() {
        filesystem.displayDirectoryStructure();
    }

private:
    /**
     * @brief Asigna una nueva dirección de bloque
     */
    PhysicalAddress allocateNewBlock() {
        PhysicalAddress addr = next_free_address;
        
        // Avanzar a la siguiente dirección
        next_free_address.setSector(next_free_address.getSector() + 1);
        if (next_free_address.getSector() >= config.getSectorsPerTrack()) {
            next_free_address.setSector(0);
            next_free_address.setTrack(next_free_address.getTrack() + 1);
            
            if (next_free_address.getTrack() >= config.getTracksPerSurface()) {
                next_free_address.setTrack(0);
                next_free_address.setSurface(next_free_address.getSurface() + 1);
                
                if (next_free_address.getSurface() >= config.getSurfacesPerPlatter()) {
                    next_free_address.setSurface(0);
                    next_free_address.setPlatter(next_free_address.getPlatter() + 1);
                }
            }
        }
        
        return addr;
    }

    /**
     * @brief Encuentra un bloque con espacio suficiente
     */
    std::shared_ptr<Block> findBlockWithSpace(const std::string& table_name, size_t record_size) {
        auto it = relation_blocks.find(table_name);
        if (it == relation_blocks.end()) {
            return nullptr;
        }
        
        for (const auto& addr : it->second) {
            auto block = getBlock(addr);
            if (block && block->getFreeSpace() >= (record_size + sizeof(size_t))) {
                return block;
            }
        }
        
        return nullptr;
    }

    /**
     * @brief Obtiene un bloque (desde cache o disco)
     */
    std::shared_ptr<Block> getBlock(const PhysicalAddress& addr) {
        // Buscar en cache
        auto it = block_cache.find(addr);
        if (it != block_cache.end()) {
            return it->second;
        }
        
        // Cargar desde disco
        auto block = std::make_shared<Block>(addr, config.getBytesPerSector());
        if (filesystem.readBlock(addr, *block)) {
            block_cache[addr] = block;
            return block;
        }
        
        return nullptr;
    }

    /**
     * @brief Simula el tiempo de acceso a disco
     */
    double simulateAccessTime(const PhysicalAddress& addr) {
        // Simular seek time, rotational latency y transfer time
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // Seek time (variable según distancia)
        std::uniform_real_distribution<> seek_dist(0.0, config.getSeekTime() * 2);
        double seek_time = seek_dist(gen);
        
        // Rotational latency (0 a máximo)
        std::uniform_real_distribution<> rot_dist(0.0, config.getRotationalLatency() * 2);
        double rot_latency = rot_dist(gen);
        
        // Transfer time (fijo)
        double transfer_time = config.getTransferTime();
        
        return seek_time + rot_latency + transfer_time;
    }

    /**
     * @brief Parsea una línea CSV
     */
    std::vector<std::string> parseCSVLine(const std::string& line) {
        std::vector<std::string> values;
        std::istringstream iss(line);
        std::string value;
        
        while (std::getline(iss, value, ',')) {
            // Remover espacios en blanco
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            values.push_back(value);
        }
        
        return values;
    }

    /**
     * @brief Guarda el esquema de una tabla
     */
    void saveTableSchema(const std::string& table_name, 
                         const std::vector<FieldDefinition>& schema,
                         bool use_fixed) {
        std::string schema_path = filesystem.getBasePath() + "/metadata/schema_" + table_name + ".txt";
        std::ofstream file(schema_path);
        
        if (file.is_open()) {
            file << "# Esquema de la tabla: " << table_name << std::endl;
            file << "record_type=" << (use_fixed ? "FIXED" : "VARIABLE") << std::endl;
            file << "field_count=" << schema.size() << std::endl;
            
            for (const auto& field : schema) {
                file << field.name << "|";
                file << static_cast<int>(field.type) << "|";
                file << field.max_length << "|";
                file << (field.is_nullable ? 1 : 0) << std::endl;
            }
            
            file.close();
        }
    }

    /**
     * @brief Carga el esquema de una tabla
     */
    std::vector<FieldDefinition> loadTableSchema(const std::string& table_name) {
        std::vector<FieldDefinition> schema;
        std::string schema_path = filesystem.getBasePath() + "/metadata/schema_" + table_name + ".txt";
        
        std::ifstream file(schema_path);
        if (!file.is_open()) {
            return schema;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("field_count=") == 0 || line.find("record_type=") == 0) {
                continue;
            }
            
            // Parsear definición de campo
            std::istringstream iss(line);
            std::string name, type_str, length_str, nullable_str;
            
            if (std::getline(iss, name, '|') &&
                std::getline(iss, type_str, '|') &&
                std::getline(iss, length_str, '|') &&
                std::getline(iss, nullable_str)) {
                
                FieldType type = static_cast<FieldType>(std::stoi(type_str));
                size_t length = std::stoull(length_str);
                bool nullable = (nullable_str == "1");
                
                schema.emplace_back(name, type, length, nullable);
            }
        }
        
        file.close();
        return schema;
    }

    /**
     * @brief Verifica si una tabla usa registros fijos
     */
    bool isTableFixedRecord(const std::string& table_name) {
        std::string schema_path = filesystem.getBasePath() + "/metadata/schema_" + table_name + ".txt";
        std::ifstream file(schema_path);
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("record_type=") == 0) {
                    return line.find("FIXED") != std::string::npos;
                }
            }
            file.close();
        }
        
        return true;  // Por defecto, usar registros fijos
    }

    /**
     * @brief Carga el índice de bloques existentes
     */
    void loadBlockIndex() {
        auto occupied = filesystem.getOccupiedSectors();
        
        for (const auto& addr : occupied) {
            auto block = std::make_shared<Block>(addr, config.getBytesPerSector());
            if (filesystem.readBlock(addr, *block)) {
                block_cache[addr] = block;
                
                std::string table_name = block->getRelationName();
                if (!table_name.empty()) {
                    relation_blocks[table_name].push_back(addr);
                }
                
                // Actualizar next_record_id
                for (const auto& record : block->getAllRecords()) {
                    if (record->getId() >= next_record_id) {
                        next_record_id = record->getId() + 1;
                    }
                }
            }
        }
        
        // Actualizar next_free_address
        if (!occupied.empty()) {
            auto last_addr = *std::max_element(occupied.begin(), occupied.end());
            next_free_address = last_addr;
            next_free_address.setSector(next_free_address.getSector() + 1);
        }
    }
};

#endif // DISK_MANAGER_H