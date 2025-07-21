#ifndef RECORD_REFERENCE_H
#define RECORD_REFERENCE_H

#include "PhysicalAddress.h"
#include <iostream>

/**
 * @brief Referencia a un registro en el almacenamiento físico
 * 
 * Contiene la información necesaria para localizar un registro:
 * - Dirección física del bloque (sector)
 * - Slot ID dentro del bloque
 * - Metadatos adicionales para optimización
 */
class RecordReference {
private:
    PhysicalAddress physical_address;    // Dirección del sector/bloque
    int slot_id;                        // ID del slot en el offset table
    size_t record_size;                 // Tamaño del registro (para optimización)
    bool is_valid;                      // Validez de la referencia

public:
    /**
     * @brief Constructor por defecto (referencia inválida)
     */
    RecordReference() 
        : slot_id(-1), record_size(0), is_valid(false) {}
    
    /**
     * @brief Constructor con dirección física y slot
     */
    RecordReference(const PhysicalAddress& addr, int slot, size_t size = 0)
        : physical_address(addr), slot_id(slot), record_size(size), is_valid(true) {}
    
    // Getters
    const PhysicalAddress& getPhysicalAddress() const { return physical_address; }
    int getSlotId() const { return slot_id; }
    size_t getRecordSize() const { return record_size; }
    bool isValid() const { return is_valid; }
    
    // Setters
    void setPhysicalAddress(const PhysicalAddress& addr) { 
        physical_address = addr; 
        is_valid = true;
    }
    void setSlotId(int slot) { slot_id = slot; }
    void setRecordSize(size_t size) { record_size = size; }
    void invalidate() { is_valid = false; }
    
    /**
     * @brief Convierte a page_id para BufferManager
     * Usa la dirección física para generar un ID único
     */
    int toPageId() const {
        if (!is_valid) return -1;
        
        // Combinar componentes de la dirección física en un ID único
        return (physical_address.getPlatter() * 1000000) +
               (physical_address.getSurface() * 10000) +
               (physical_address.getTrack() * 100) +
               physical_address.getSector();
    }
    
    /**
     * @brief Crea referencia desde page_id
     */
    static RecordReference fromPageId(int page_id, int slot_id) {
        // Descomponer page_id en componentes de dirección física
        int platter = page_id / 1000000;
        int surface = (page_id % 1000000) / 10000;
        int track = (page_id % 10000) / 100;
        int sector = page_id % 100;
        
        PhysicalAddress addr(platter, surface, track, sector);
        return RecordReference(addr, slot_id);
    }
    
    /**
     * @brief Serialización para almacenamiento en índice
     */
    std::string serialize() const {
        std::ostringstream oss;
        oss << physical_address.toString() << "|" 
            << slot_id << "|" 
            << record_size << "|" 
            << (is_valid ? 1 : 0);
        return oss.str();
    }
    
    /**
     * @brief Deserialización desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string addr_str, slot_str, size_str, valid_str;
        
        if (!std::getline(iss, addr_str, '|') ||
            !std::getline(iss, slot_str, '|') ||
            !std::getline(iss, size_str, '|') ||
            !std::getline(iss, valid_str)) {
            return false;
        }
        
        // Parsear PhysicalAddress desde string
        // Formato esperado: "P0_S0_T0_SEC0"
        size_t p_pos = addr_str.find('P');
        size_t s_pos = addr_str.find("_S");
        size_t t_pos = addr_str.find("_T");
        size_t sec_pos = addr_str.find("_SEC");
        
        if (p_pos == std::string::npos || s_pos == std::string::npos || 
            t_pos == std::string::npos || sec_pos == std::string::npos) {
            return false;
        }
        
        try {
            int platter = std::stoi(addr_str.substr(p_pos + 1, s_pos - p_pos - 1));
            int surface = std::stoi(addr_str.substr(s_pos + 2, t_pos - s_pos - 2));
            int track = std::stoi(addr_str.substr(t_pos + 2, sec_pos - t_pos - 2));
            int sector = std::stoi(addr_str.substr(sec_pos + 4));
            
            physical_address = PhysicalAddress(platter, surface, track, sector);
            slot_id = std::stoi(slot_str);
            record_size = std::stoull(size_str);
            is_valid = (valid_str == "1");
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    /**
     * @brief Operadores de comparación
     */
    bool operator==(const RecordReference& other) const {
        return physical_address == other.physical_address && 
               slot_id == other.slot_id;
    }
    
    bool operator<(const RecordReference& other) const {
        if (physical_address < other.physical_address) return true;
        if (other.physical_address < physical_address) return false;
        return slot_id < other.slot_id;
    }
    
    /**
     * @brief Visualización para debugging
     */
    friend std::ostream& operator<<(std::ostream& os, const RecordReference& ref) {
        if (!ref.is_valid) {
            os << "RecordRef[INVALID]";
        } else {
            os << "RecordRef[" << ref.physical_address 
               << ", slot=" << ref.slot_id 
               << ", size=" << ref.record_size << "]";
        }
        return os;
    }
};

#endif // RECORD_REFERENCE_H
