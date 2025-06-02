#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "Record.h"
#include "PhysicalAddress.h"


class Block {
private:
    PhysicalAddress address;                              // Dirección física del bloque
    size_t block_size;                                   // Tamaño del bloque en bytes
    size_t header_size;                                  // Tamaño del header
    std::vector<std::shared_ptr<Record>> records;        // Registros en el bloque
    std::vector<size_t> offset_table;                    // Tabla de offsets (Fig 13.19)
    size_t used_space;                                   // Espacio utilizado
    int next_record_id;                                  // ID del próximo registro
    
    // Metadatos del bloque
    std::string relation_name;                           // Relación a la que pertenece
    bool is_dirty;                                       // Si necesita ser escrito a disco
    
public:
    /**
     * @brief Constructor
     */
    Block(const PhysicalAddress& addr, size_t size = 4096) 
        : address(addr)
        , block_size(size)
        , header_size(64)  // Header fijo de 64 bytes
        , used_space(header_size)
        , next_record_id(1)
        , is_dirty(false)
    {
    }

    // Getters básicos
    const PhysicalAddress& getAddress() const { return address; }
    size_t getBlockSize() const { return block_size; }
    size_t getUsedSpace() const { return used_space; }
    size_t getFreeSpace() const { return block_size - used_space; }
    size_t getRecordCount() const { return records.size(); }
    bool isDirty() const { return is_dirty; }
    void markDirty() { is_dirty = true; }
    void markClean() { is_dirty = false; }

    const std::string& getRelationName() const { return relation_name; }
    void setRelationName(const std::string& name) { relation_name = name; }

    /**
     * @brief Calcula el porcentaje de ocupación del bloque
     */
    double getOccupancyPercentage() const {
        return (static_cast<double>(used_space) / block_size) * 100.0;
    }

    /**
     * @brief Verifica si un registro cabe en el bloque
     */
    bool canFit(const std::shared_ptr<Record>& record) const {
        size_t record_size = record->getSize();
        size_t offset_entry_size = sizeof(size_t);  // Para la tabla de offsets
        
        return (used_space + record_size + offset_entry_size) <= block_size;
    }

    /**
     * @brief Añade un registro al bloque
     */
    bool addRecord(std::shared_ptr<Record> record) {
        if (!canFit(record)) {
            return false;
        }

        // Asignar ID si no lo tiene
        if (record->getId() == -1) {
            record->setId(next_record_id++);
        }

        // Asignar dirección física
        record->setPhysicalAddress(address);

        // Calcular offset para el nuevo registro
        size_t offset = used_space;
        offset_table.push_back(offset);

        // Añadir el registro
        records.push_back(record);
        used_space += record->getSize() + sizeof(size_t);  // +offset entry
        
        markDirty();
        return true;
    }

    /**
     * @brief Elimina un registro lógicamente (tombstone)
     */
    bool deleteRecord(int record_id) {
        for (auto& record : records) {
            if (record->getId() == record_id) {
                record->markAsDeleted();
                markDirty();
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Elimina físicamente los registros marcados como eliminados
     */
    void compactBlock() {
        // Filtrar registros no eliminados
        auto new_end = std::remove_if(records.begin(), records.end(),
            [](const std::shared_ptr<Record>& record) {
                return record->isDeleted();
            });
        
        records.erase(new_end, records.end());
        
        // Recalcular offsets y espacio usado
        recalculateOffsets();
        markDirty();
    }

    /**
     * @brief Busca un registro por ID
     */
    std::shared_ptr<Record> findRecord(int record_id) const {
        for (const auto& record : records) {
            if (record->getId() == record_id && !record->isDeleted()) {
                return record;
            }
        }
        return nullptr;
    }

    /**
     * @brief Obtiene todos los registros activos
     */
    std::vector<std::shared_ptr<Record>> getActiveRecords() const {
        std::vector<std::shared_ptr<Record>> active_records;
        for (const auto& record : records) {
            if (!record->isDeleted()) {
                active_records.push_back(record);
            }
        }
        return active_records;
    }

    /**
     * @brief Obtiene todos los registros (incluyendo eliminados)
     */
    const std::vector<std::shared_ptr<Record>>& getAllRecords() const {
        return records;
    }

    /**
     * @brief Serializa el bloque completo para almacenamiento
     */
    std::string serialize() const {
        std::ostringstream oss;
        
        // Header del bloque
        oss << "BLOCK_HEADER|" << address.toString() << "|" << block_size 
            << "|" << used_space << "|" << relation_name << "|" << records.size() << std::endl;
        
        // Tabla de offsets
        oss << "OFFSET_TABLE|";
        for (size_t i = 0; i < offset_table.size(); ++i) {
            if (i > 0) oss << ",";
            oss << offset_table[i];
        }
        oss << std::endl;
        
        // Registros
        for (const auto& record : records) {
            oss << "RECORD|" << record->serialize() << std::endl;
        }
        
        return oss.str();
    }

    /**
     * @brief Deserializa un bloque desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string line;
        
        records.clear();
        offset_table.clear();
        
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            std::istringstream line_stream(line);
            std::string type;
            std::getline(line_stream, type, '|');
            
            if (type == "BLOCK_HEADER") {
                // Parsear header del bloque
                std::string addr_str, size_str, used_str, rel_str, count_str;
                std::getline(line_stream, addr_str, '|');
                std::getline(line_stream, size_str, '|');
                std::getline(line_stream, used_str, '|');
                std::getline(line_stream, rel_str, '|');
                std::getline(line_stream, count_str, '|');
                
                block_size = std::stoull(size_str);
                used_space = std::stoull(used_str);
                relation_name = rel_str;
                
            } else if (type == "OFFSET_TABLE") {
                // Parsear tabla de offsets
                std::string offsets_str;
                std::getline(line_stream, offsets_str);
                
                std::istringstream offsets_stream(offsets_str);
                std::string offset;
                while (std::getline(offsets_stream, offset, ',')) {
                    offset_table.push_back(std::stoull(offset));
                }
                
            } else if (type == "RECORD") {
                // Parsear registro
                std::string record_data;
                std::getline(line_stream, record_data);
                
                // Determinar tipo de registro
                if (record_data.find("FIXED|") == 0) {
                    auto record = std::make_shared<FixedRecord>();
                    if (record->deserialize(record_data)) {
                        records.push_back(record);
                    }
                } else if (record_data.find("VARIABLE|") == 0) {
                    auto record = std::make_shared<VariableRecord>();
                    if (record->deserialize(record_data)) {
                        records.push_back(record);
                    }
                }
            }
        }
        
        return true;
    }

    /**
     * @brief Muestra información del bloque
     */
    void displayInfo() const {
        std::cout << "\n=== INFORMACIÓN DEL BLOQUE ===" << std::endl;
        std::cout << "Dirección: " << address << std::endl;
        std::cout << "Relación: " << relation_name << std::endl;
        std::cout << "Tamaño del bloque: " << block_size << " bytes" << std::endl;
        std::cout << "Espacio usado: " << used_space << " bytes (" 
                  << getOccupancyPercentage() << "%)" << std::endl;
        std::cout << "Espacio libre: " << getFreeSpace() << " bytes" << std::endl;
        std::cout << "Número de registros: " << getRecordCount() << std::endl;
        
        // Contar registros eliminados
        int deleted_count = 0;
        for (const auto& record : records) {
            if (record->isDeleted()) deleted_count++;
        }
        if (deleted_count > 0) {
            std::cout << "Registros eliminados: " << deleted_count << std::endl;
        }
    }

    /**
     * @brief Muestra todos los registros del bloque
     */
    void displayRecords() const {
        std::cout << "\n=== REGISTROS EN EL BLOQUE ===" << std::endl;
        for (const auto& record : records) {
            record->display();
            std::cout << "---" << std::endl;
        }
    }

private:
    /**
     * @brief Recalcula offsets después de eliminar registros
     */
    void recalculateOffsets() {
        offset_table.clear();
        used_space = header_size;
        
        for (const auto& record : records) {
            offset_table.push_back(used_space);
            used_space += record->getSize() + sizeof(size_t);
        }
    }
};

#endif // BLOCK_H