#ifndef RECORD_REFERENCE_H
#define RECORD_REFERENCE_H

#include <iostream>
#include <string>
#include <memory>
#include "PhysicalAddress.h"
#include "Record.h"
#include "Block.h"


/**
 * @brief RecordReference - Puente ligero entre índices y almacenamiento físico
 * 
 * Esta clase actúa como referencia indirecta que:
 * - ✅ Es almacenada en índices (Hash Extensible / B+ Tree)
 * - ✅ Apunta a ubicación física real en disco
 * - ✅ Permite resolución lazy de registros vía DiskManager
 * - ✅ Optimiza memoria de índices (solo referencias, no registros completos)
 * - ✅ Integra con Buffer Pool para acceso eficiente
 * 
 * Arquitectura:
 * Index → RecordReference → DiskManager → Buffer Pool → Block → Record
 */
class RecordReference {
private:
    PhysicalAddress physical_address;    // Ubicación física en disco
    int slot_id;                        // ID del slot dentro del bloque
    int page_id;                        // Page ID para Buffer Pool integration
    bool is_valid;                      // Validez de la referencia
    
    // Metadatos de optimización
    mutable bool is_cached;             // Si está en buffer pool
    mutable std::string cached_key;     // Clave cacheada para comparaciones rápidas

public:
    /**
     * @brief Constructor por defecto (referencia inválida)
     */
    RecordReference() 
        : slot_id(-1)
        , page_id(-1)
        , is_valid(false)
        , is_cached(false)
    {
    }

    /**
     * @brief Constructor principal - USADO POR DISKMANAGER
     */
    RecordReference(const PhysicalAddress& addr, int slot, int pid = -1)
        : physical_address(addr)
        , slot_id(slot)
        , page_id(pid)
        , is_valid(true)
        , is_cached(false)
    {
    }

    // ============================================================================
    // GETTERS BÁSICOS
    // ============================================================================
    
    const PhysicalAddress& getPhysicalAddress() const { return physical_address; }
    int getSlotId() const { return slot_id; }
    int getPageId() const { return page_id; }
    bool isValid() const { return is_valid; }
    bool isCached() const { return is_cached; }
    const std::string& getCachedKey() const { return cached_key; }

    // ============================================================================
    // SETTERS Y MODIFICADORES
    // ============================================================================
    
    void setPageId(int pid) { page_id = pid; }
    void markCached(bool cached = true) const { is_cached = cached; }
    void setCachedKey(const std::string& key) const { cached_key = key; }
    void invalidate() { is_valid = false; }

    // ============================================================================
    // OPERACIONES DE COMPARACIÓN - CRÍTICAS PARA ÍNDICES
    // ============================================================================
    
    /**
     * @brief Igualdad basada en dirección física
     */
    bool operator==(const RecordReference& other) const {
        return is_valid && other.is_valid &&
               physical_address == other.physical_address &&
               slot_id == other.slot_id;
    }

    bool operator!=(const RecordReference& other) const {
        return !(*this == other);
    }

    /**
     * @brief Ordenamiento para B+ Tree
     */
    bool operator<(const RecordReference& other) const {
        if (!is_valid) return other.is_valid;
        if (!other.is_valid) return false;
        
        if (physical_address != other.physical_address) {
            return physical_address < other.physical_address;
        }
        return slot_id < other.slot_id;
    }

    // ============================================================================
    // RESOLUCIÓN DE REGISTROS - INTEGRACIÓN CON DISKMANAGER
    // ============================================================================
    
    /**
     * @brief Resuelve la referencia a registro completo
     * NOTA: Este método será llamado por DiskManagerExtended::resolveRecordReference()
     */
    template<typename DiskManagerType>
    std::shared_ptr<Record> resolve(DiskManagerType* disk_manager) const {
        if (!is_valid || !disk_manager) {
            return nullptr;
        }

        // Intentar desde buffer pool primero
        if (page_id != -1) {
            // TODO: Integrar con Buffer Pool cuando esté disponible
            // auto block = buffer_manager->getPage(page_id);
        }

        // Fallback: leer desde disco directamente
        Block block(physical_address, 4096);
        if (!disk_manager->readBlock(physical_address, block)) {
            return nullptr;
        }

        // Buscar registro por slot_id
        auto records = block.getAllRecords();
        for (const auto& record : records) {
            if (record->getId() == slot_id && !record->isDeleted()) {
                return record;
            }
        }

        return nullptr;
    }

    /**
     * @brief Resuelve solo la clave (optimización para índices)
     */
    template<typename DiskManagerType>
    std::string resolveKey(DiskManagerType* disk_manager, int key_field_index = 1) const {
        if (!cached_key.empty()) {
            return cached_key;
        }

        auto record = resolve(disk_manager);
        if (!record) {
            return "";
        }

        // Extraer clave según tipo de registro
        if (auto var_record = std::dynamic_pointer_cast<VariableRecord>(record)) {
            auto values = var_record->getFieldValues();
            if (key_field_index < static_cast<int>(values.size())) {
                cached_key = values[key_field_index];
                return cached_key;
            }
        }

        return "";
    }

    // ============================================================================
    // SERIALIZACIÓN PARA PERSISTENCIA DE ÍNDICES
    // ============================================================================
    
    /**
     * @brief Serializa la referencia para guardar en índices
     */
    std::string serialize() const {
        if (!is_valid) {
            return "INVALID_REF";
        }

        return "REF|" + physical_address.toString() + "|" + 
               std::to_string(slot_id) + "|" + std::to_string(page_id);
    }

    /**
     * @brief Deserializa desde string
     */
    bool deserialize(const std::string& data) {
        if (data == "INVALID_REF") {
            is_valid = false;
            return true;
        }

        // Parsear: REF|addr|slot|page
        size_t pos1 = data.find('|');
        if (pos1 == std::string::npos) return false;

        size_t pos2 = data.find('|', pos1 + 1);
        if (pos2 == std::string::npos) return false;

        size_t pos3 = data.find('|', pos2 + 1);
        if (pos3 == std::string::npos) return false;

        try {
            std::string addr_str = data.substr(pos1 + 1, pos2 - pos1 - 1);
            slot_id = std::stoi(data.substr(pos2 + 1, pos3 - pos2 - 1));
            page_id = std::stoi(data.substr(pos3 + 1));
            
            // TODO: Parsear PhysicalAddress desde addr_str
            is_valid = true;
            return true;
        } catch (...) {
            return false;
        }
    }

    // ============================================================================
    // UTILIDADES Y DEBUG
    // ============================================================================
    
    /**
     * @brief Información para debug
     */
    std::string toString() const {
        if (!is_valid) {
            return "RecordReference[INVALID]";
        }

        std::ostringstream oss;
        oss << "RecordReference[" << physical_address.toString() 
            << ", slot=" << slot_id 
            << ", page=" << page_id
            << ", cached=" << (is_cached ? "yes" : "no") << "]";
        return oss.str();
    }

    /**
     * @brief Operador de salida para cout
     */
    friend std::ostream& operator<<(std::ostream& os, const RecordReference& ref) {
        os << ref.toString();
        return os;
    }

    // ============================================================================
    // FACTORY METHODS - PARA DISKMANAGER
    // ============================================================================
    
    /**
     * @brief Crea RecordReference inválido
     */
    static RecordReference invalid() {
        return RecordReference();
    }

    /**
     * @brief Crea RecordReference válido
     */
    static RecordReference create(const PhysicalAddress& addr, int slot, int page = -1) {
        return RecordReference(addr, slot, page);
    }

    // ============================================================================
    // INTEGRACIÓN CON BUFFER POOL (FUTURO)
    // ============================================================================
    
    /**
     * @brief Hash para usar en std::unordered_map
     */
    struct Hash {
        size_t operator()(const RecordReference& ref) const {
            if (!ref.is_valid) return 0;
            
            // Hash basado en dirección física
            auto addr_hash = std::hash<std::string>{}(ref.physical_address.toString());
            auto slot_hash = std::hash<int>{}(ref.slot_id);
            return addr_hash ^ (slot_hash << 1);
        }
    };

    /**
     * @brief Estadísticas de la referencia
     */
    struct Stats {
        bool is_valid;
        bool is_cached;
        bool has_cached_key;
        std::string physical_location;
        int slot_id;
        int page_id;
    };

    Stats getStats() const {
        Stats stats;
        stats.is_valid = is_valid;
        stats.is_cached = is_cached;
        stats.has_cached_key = !cached_key.empty();
        stats.physical_location = physical_address.toString();
        stats.slot_id = slot_id;
        stats.page_id = page_id;
        return stats;
    }
};

#endif // RECORD_REFERENCE_H