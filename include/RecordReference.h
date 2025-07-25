#ifndef RECORD_REFERENCE_H
#define RECORD_REFERENCE_H

#include "PhysicalAddress.h"
#include <iostream>

/**
 * @brief Referencia ligera a un registro almacenado en disco
 * 
 * Utilizada por los índices para evitar almacenar registros completos.
 * Contiene información suficiente para localizar y cargar el registro desde disco.
 */
class RecordReference {
private:
    PhysicalAddress physical_address;  // Dirección física en disco
    int slot_id;                      // ID del slot dentro de la página
    size_t record_size;               // Tamaño del registro (para validación)
    bool is_valid;                    // Flag de validez

public:
    /**
     * @brief Constructor por defecto
     */
    RecordReference() : slot_id(-1), record_size(0), is_valid(false) {}
    
    /**
     * @brief Constructor con parámetros
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
     * 
     * Necesario para mapear entre PhysicalAddress y el sistema de páginas del BufferManager
     */
    int toPageId() const {
        if (!is_valid) return -1;
        
        // Generar page_id único basado en la dirección física
        return physical_address.getPlatter() * 10000 + 
               physical_address.getSurface() * 1000 + 
               physical_address.getTrack() * 100 + 
               physical_address.getSector();
    }
    
    /**
     * @brief Crea RecordReference desde page_id
     */
    static RecordReference fromPageId(int page_id, int slot_id) {
        // Decodificar page_id de vuelta a componentes físicos
        int platter = page_id / 10000;
        int surface = (page_id % 10000) / 1000;
        int track = (page_id % 1000) / 100;
        int sector = page_id % 100;
        
        PhysicalAddress addr(platter, surface, track, sector);
        return RecordReference(addr, slot_id);
    }
    
    /**
     * @brief Serialización para persistencia
     */
    std::string serialize() const {
        return physical_address.toString() + "|" + 
               std::to_string(slot_id) + "|" + 
               std::to_string(record_size) + "|" + 
               (is_valid ? "1" : "0");
    }
    
    /**
     * @brief Deserialización
     */
    bool deserialize(const std::string& data) {
        size_t pos1 = data.find('|');
        size_t pos2 = data.find('|', pos1 + 1);
        size_t pos3 = data.find('|', pos2 + 1);
        
        if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
            return false;
        }
        
        // No implementamos physical_address.fromString() por simplicidad
        slot_id = std::stoi(data.substr(pos1 + 1, pos2 - pos1 - 1));
        record_size = std::stoull(data.substr(pos2 + 1, pos3 - pos2 - 1));
        is_valid = (data.substr(pos3 + 1) == "1");
        
        return true;
    }
    
    /**
     * @brief Operador de comparación para ordenamiento
     */
    bool operator<(const RecordReference& other) const {
        if (physical_address.toString() != other.physical_address.toString()) {
            return physical_address.toString() < other.physical_address.toString();
        }
        return slot_id < other.slot_id;
    }
    
    bool operator==(const RecordReference& other) const {
        return physical_address.toString() == other.physical_address.toString() && 
               slot_id == other.slot_id;
    }
    
    /**
     * @brief Display para debugging
     */
    void display() const {
        std::cout << "RecordRef[" << physical_address.toString() 
                  << ", slot=" << slot_id 
                  << ", size=" << record_size 
                  << ", valid=" << (is_valid ? "YES" : "NO") << "]";
    }
    
    friend std::ostream& operator<<(std::ostream& os, const RecordReference& ref) {
        os << "RecordRef[" << ref.physical_address.toString() 
           << ", slot=" << ref.slot_id << "]";
        return os;
    }
};

#endif // RECORD_REFERENCE_H