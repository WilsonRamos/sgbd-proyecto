#ifndef RECORD_REFERENCE_H
#define RECORD_REFERENCE_H

#include "PhysicalAddress.h"
#include <iostream>
#include <string>
#include <sstream>

/**
 * @brief Referencia ligera a un registro en disco
 * 
 * En lugar de cargar registros completos en índices,
 * utilizamos referencias que apuntan a la ubicación física
 */
class RecordReference {
private:
    PhysicalAddress physical_address;
    int slot_id;                    // ID del slot dentro del bloque
    bool is_valid;                  // Indica si la referencia es válida

public:
    /**
     * @brief Constructor por defecto - referencia inválida
     */
    RecordReference() : slot_id(-1), is_valid(false) {}
    
    /**
     * @brief Constructor con dirección física y slot
     */
    RecordReference(const PhysicalAddress& addr, int slot) 
        : physical_address(addr), slot_id(slot), is_valid(true) {}
    
    // ============================================================================
    // GETTERS
    // ============================================================================
    
    const PhysicalAddress& getPhysicalAddress() const { return physical_address; }
    int getSlotId() const { return slot_id; }
    bool isValid() const { return is_valid; }
    
    // ============================================================================
    // ✅ FUNCIÓN AGREGADA - CONVERSIÓN A PAGE ID
    // ============================================================================
    
    /**
     * @brief Convierte la dirección física a un Page ID único
     */
    int toPageId() const {
        if (!is_valid) {
            return -1;
        }
        
        // Combinar componentes de la dirección física para crear un ID único
        // Esto es una simplificación educativa
        int page_id = physical_address.getPlatter() * 1000000 +
                      physical_address.getSurface() * 100000 +
                      physical_address.getTrack() * 1000 +
                      physical_address.getSector();
        
        return page_id;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Convierte a Page ID con offset
     */
    std::pair<int, int> toPageIdWithOffset() const {
        int page_id = toPageId();
        return std::make_pair(page_id, slot_id);
    }
    
    // ============================================================================
    // OPERACIONES
    // ============================================================================
    
    /**
     * @brief Marca la referencia como inválida
     */
    void invalidate() { is_valid = false; }
    
    /**
     * @brief Actualiza la referencia
     */
    void update(const PhysicalAddress& addr, int slot) {
        physical_address = addr;
        slot_id = slot;
        is_valid = true;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Actualiza desde Page ID
     */
    void updateFromPageId(int page_id, int slot) {
        if (page_id < 0) {
            invalidate();
            return;
        }
        
        // Descomponer Page ID en componentes de dirección física
        int platter = page_id / 1000000;
        int surface = (page_id % 1000000) / 100000;
        int track = (page_id % 100000) / 1000;
        int sector = page_id % 1000;
        
        physical_address = PhysicalAddress(platter, surface, track, sector);
        slot_id = slot;
        is_valid = true;
    }
    
    // ============================================================================
    // OPERADORES
    // ============================================================================
    
    bool operator==(const RecordReference& other) const {
        return is_valid && other.is_valid && 
               physical_address.toString() == other.physical_address.toString() && 
               slot_id == other.slot_id;
    }
    
    bool operator!=(const RecordReference& other) const {
        return !(*this == other);
    }
    
    bool operator<(const RecordReference& other) const {
        if (!is_valid && !other.is_valid) return false;
        if (!is_valid) return true;
        if (!other.is_valid) return false;
        
        // ✅ USAR toString() EN LUGAR DE operator!= para evitar problemas
        if (physical_address.toString() != other.physical_address.toString()) {
            return physical_address.toString() < other.physical_address.toString();
        }
        return slot_id < other.slot_id;
    }
    
    // ============================================================================
    // SERIALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Convierte a string para depuración
     */
    std::string toString() const {
        if (!is_valid) {
            return "RecordReference(INVALID)";
        }
        
        std::ostringstream ss;
        ss << "RecordReference(PageID:" << toPageId() 
           << ", " << physical_address.toString() 
           << ", slot:" << slot_id << ")";
        return ss.str();
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Información detallada
     */
    std::string getDetailedInfo() const {
        std::ostringstream ss;
        ss << "=== RECORD REFERENCE INFO ===\n";
        ss << "Valid: " << (is_valid ? "Yes" : "No") << "\n";
        
        if (is_valid) {
            ss << "Page ID: " << toPageId() << "\n";
            ss << "Physical Address: " << physical_address.toString() << "\n";
            ss << "Slot ID: " << slot_id << "\n";
            ss << "Platter: " << physical_address.getPlatter() << "\n";
            ss << "Surface: " << physical_address.getSurface() << "\n";
            ss << "Track: " << physical_address.getTrack() << "\n";
            ss << "Sector: " << physical_address.getSector() << "\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Operador de salida para debug
     */
    friend std::ostream& operator<<(std::ostream& os, const RecordReference& ref) {
        os << ref.toString();
        return os;
    }
    
    // ============================================================================
    // ✅ FUNCIONES AGREGADAS - UTILIDADES PARA BUFFER POOL
    // ============================================================================
    
    /**
     * @brief Verifica si la referencia apunta a la misma página que otra
     */
    bool samePage(const RecordReference& other) const {
        return is_valid && other.is_valid && 
               physical_address.toString() == other.physical_address.toString();
    }
    
    /**
     * @brief Calcula distancia en disco respecto a otra referencia
     */
    int diskDistance(const RecordReference& other) const {
        if (!is_valid || !other.is_valid) {
            return -1;
        }
        
        // Simplificación: distancia basada en diferencia de sectores
        return abs(toPageId() - other.toPageId());
    }
    
    /**
     * @brief Verifica si la referencia está en un rango específico
     */
    bool inRange(const RecordReference& start, const RecordReference& end) const {
        if (!is_valid || !start.is_valid || !end.is_valid) {
            return false;
        }
        
        std::string my_addr = physical_address.toString();
        std::string start_addr = start.physical_address.toString();
        std::string end_addr = end.physical_address.toString();
        
        return my_addr >= start_addr && my_addr <= end_addr;
    }
    
    /**
     * @brief Serializa la referencia para persistencia
     */
    std::string serialize() const {
        std::ostringstream ss;
        ss << is_valid << "|" 
           << physical_address.getPlatter() << "|"
           << physical_address.getSurface() << "|"
           << physical_address.getTrack() << "|"
           << physical_address.getSector() << "|"
           << slot_id;
        return ss.str();
    }
    
    /**
     * @brief Deserializa la referencia desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream ss(data);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(ss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() != 6) {
            return false;
        }
        
        try {
            is_valid = (tokens[0] == "1");
            int platter = std::stoi(tokens[1]);
            int surface = std::stoi(tokens[2]);
            int track = std::stoi(tokens[3]);
            int sector = std::stoi(tokens[4]);
            slot_id = std::stoi(tokens[5]);
            
            physical_address = PhysicalAddress(platter, surface, track, sector);
            return true;
            
        } catch (const std::exception&) {
            is_valid = false;
            return false;
        }
    }
};

#endif // RECORD_REFERENCE_H