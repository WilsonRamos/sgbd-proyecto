#ifndef RECORD_H
#define RECORD_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include "PhysicalAddress.h"

/**
 * @brief Tipos de datos soportados en los registros
 */
enum class FieldType {
    INTEGER,
    FLOAT,
    STRING,
    DATE
};

/**
 * @brief Estructura para definir un campo del registro
 */
struct FieldDefinition {
    std::string name;
    FieldType type;
    size_t max_length;  // Para strings, 0 para tipos de longitud fija
    bool is_nullable;

    FieldDefinition(const std::string& n, FieldType t, size_t len = 0, bool nullable = false)
        : name(n), type(t), max_length(len), is_nullable(nullable) {}
};

/**
 * @brief Clase base para registros
 */
class Record {
protected:
    int record_id;
    PhysicalAddress physical_address;
    bool is_deleted;  // Para eliminación lógica
    std::vector<std::string> field_values;
    std::vector<FieldDefinition> schema;

public:
    Record(int id = -1) : record_id(id), is_deleted(false) {}
    virtual ~Record() = default;

    // Getters básicos
    int getId() const { return record_id; }
    void setId(int id) { record_id = id; }
    
    const PhysicalAddress& getPhysicalAddress() const { return physical_address; }
    void setPhysicalAddress(const PhysicalAddress& addr) { physical_address = addr; }
    
    bool isDeleted() const { return is_deleted; }
    void markAsDeleted() { is_deleted = true; }
    void unmarkAsDeleted() { is_deleted = false; }

    // Manejo de esquema
    void setSchema(const std::vector<FieldDefinition>& s) { schema = s; }
    const std::vector<FieldDefinition>& getSchema() const { return schema; }

    // Manejo de valores
    void setFieldValues(const std::vector<std::string>& values) { field_values = values; }
    const std::vector<std::string>& getFieldValues() const { return field_values; }
    
    void setField(size_t index, const std::string& value) {
        if (index < field_values.size()) {
            field_values[index] = value;
        }
    }
    
    std::string getField(size_t index) const {
        if (index < field_values.size()) {
            return field_values[index];
        }
        return "";
    }

    /**
     * @brief Calcula el tamaño del registro en bytes
     */
    virtual size_t getSize() const = 0;

    /**
     * @brief Serializa el registro a string para almacenamiento
     */
    virtual std::string serialize() const = 0;

    /**
     * @brief Deserializa desde string
     */
    virtual bool deserialize(const std::string& data) = 0;

    /**
     * @brief Muestra el registro en formato legible
     */
    virtual void display() const {
        std::cout << "Record ID: " << record_id;
        if (is_deleted) std::cout << " [DELETED]";
        std::cout << " | Address: " << physical_address << std::endl;
        
        for (size_t i = 0; i < field_values.size() && i < schema.size(); ++i) {
            std::cout << "  " << schema[i].name << ": " << field_values[i] << std::endl;
        }
    }
};

/**
 * @brief Registro de longitud fija
 * 
 * Todos los campos tienen tamaño fijo, permitiendo acceso directo
 * y cálculos de offset simples (como Example 13.15 del documento)
 */
class FixedRecord : public Record {
private:
    size_t fixed_size;

public:
    FixedRecord(int id = -1, size_t size = 0) : Record(id), fixed_size(size) {}

    /**
     * @brief Establece el tamaño fijo del registro
     */
    void setFixedSize(size_t size) { fixed_size = size; }

    /**
     * @brief Calcula el tamaño fijo basado en el esquema
     */
    void calculateFixedSize() {
        fixed_size = sizeof(int) + sizeof(bool);  // record_id + is_deleted
        
        for (const auto& field : schema) {
            switch (field.type) {
                case FieldType::INTEGER:
                    fixed_size += sizeof(int);
                    break;
                case FieldType::FLOAT:
                    fixed_size += sizeof(float);
                    break;
                case FieldType::STRING:
                    fixed_size += field.max_length;  // Tamaño fijo para string
                    break;
                case FieldType::DATE:
                    fixed_size += 12;  // Formato "YYYY-MM-DD\0"
                    break;
            }
        }
        
        // Alineación a múltiplo de 4 bytes (como menciona el documento)
        fixed_size = (fixed_size + 3) & ~3;
    }

    size_t getSize() const override {
        return fixed_size;
    }

    std::string serialize() const override {
        std::ostringstream oss;
        oss << "FIXED|" << record_id << "|" << (is_deleted ? 1 : 0) << "|";
        oss << physical_address.toString() << "|";
        
        for (size_t i = 0; i < field_values.size(); ++i) {
            if (i > 0) oss << ",";
            oss << field_values[i];
        }
        
        return oss.str();
    }

    bool deserialize(const std::string& data) override {
        std::istringstream iss(data);
        std::string type, id_str, deleted_str, addr_str, fields_str;
        
        if (!std::getline(iss, type, '|') || type != "FIXED") return false;
        if (!std::getline(iss, id_str, '|')) return false;
        if (!std::getline(iss, deleted_str, '|')) return false;
        if (!std::getline(iss, addr_str, '|')) return false;
        if (!std::getline(iss, fields_str)) return false;
        
        record_id = std::stoi(id_str);
        is_deleted = (deleted_str == "1");
        
        // Parsear campos
        field_values.clear();
        std::istringstream fields_stream(fields_str);
        std::string field;
        while (std::getline(fields_stream, field, ',')) {
            field_values.push_back(field);
        }
        
        return true;
    }
};

/**
 * @brief Registro de longitud variable
 * 
 * Los campos pueden tener tamaños variables, requiere header con
 * información de offsets (como Fig 13.23 del documento)
 */
class VariableRecord : public Record {
private:
    std::vector<size_t> field_offsets;  // Offsets de cada campo
    size_t total_size;

public:
    VariableRecord(int id = -1) : Record(id), total_size(0) {}

    /**
     * @brief Calcula offsets y tamaño total
     */
    void calculateOffsets() {
        field_offsets.clear();
        total_size = sizeof(int) + sizeof(bool) + sizeof(size_t);  // Header básico
        
        // Espacio para offsets
        total_size += schema.size() * sizeof(size_t);
        
        // Calcular offsets de cada campo
        for (size_t i = 0; i < field_values.size(); ++i) {
            field_offsets.push_back(total_size);
            
            if (i < schema.size()) {
                switch (schema[i].type) {
                    case FieldType::INTEGER:
                        total_size += sizeof(int);
                        break;
                    case FieldType::FLOAT:
                        total_size += sizeof(float);
                        break;
                    case FieldType::STRING:
                        // Longitud variable + null terminator
                        total_size += field_values[i].length() + 1;
                        break;
                    case FieldType::DATE:
                        total_size += 12;
                        break;
                }
            }
        }
    }

    size_t getSize() const override {
        return total_size;
    }

    const std::vector<size_t>& getFieldOffsets() const { return field_offsets; }

    std::string serialize() const override {
        std::ostringstream oss;
        oss << "VARIABLE|" << record_id << "|" << (is_deleted ? 1 : 0) << "|";
        oss << physical_address.toString() << "|" << total_size << "|";
        
        // Serializar offsets
        for (size_t i = 0; i < field_offsets.size(); ++i) {
            if (i > 0) oss << ",";
            oss << field_offsets[i];
        }
        oss << "|";
        
        // Serializar valores
        for (size_t i = 0; i < field_values.size(); ++i) {
            if (i > 0) oss << ",";
            oss << field_values[i];
        }
        
        return oss.str();
    }

    bool deserialize(const std::string& data) override {
        std::istringstream iss(data);
        std::string type, id_str, deleted_str, addr_str, size_str, offsets_str, fields_str;
        
        if (!std::getline(iss, type, '|') || type != "VARIABLE") return false;
        if (!std::getline(iss, id_str, '|')) return false;
        if (!std::getline(iss, deleted_str, '|')) return false;
        if (!std::getline(iss, addr_str, '|')) return false;
        if (!std::getline(iss, size_str, '|')) return false;
        if (!std::getline(iss, offsets_str, '|')) return false;
        if (!std::getline(iss, fields_str)) return false;
        
        record_id = std::stoi(id_str);
        is_deleted = (deleted_str == "1");
        total_size = std::stoull(size_str);
        
        // Parsear offsets
        field_offsets.clear();
        std::istringstream offsets_stream(offsets_str);
        std::string offset;
        while (std::getline(offsets_stream, offset, ',')) {
            field_offsets.push_back(std::stoull(offset));
        }
        
        // Parsear campos
        field_values.clear();
        std::istringstream fields_stream(fields_str);
        std::string field;
        while (std::getline(fields_stream, field, ',')) {
            field_values.push_back(field);
        }
        
        return true;
    }

    void display() const override {
        Record::display();
        std::cout << "  Total size: " << total_size << " bytes" << std::endl;
        std::cout << "  Field offsets: ";
        for (size_t offset : field_offsets) {
            std::cout << offset << " ";
        }
        std::cout << std::endl;
    }
};

#endif // RECORD_H