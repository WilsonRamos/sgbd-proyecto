#ifndef BUCKET_H
#define BUCKET_H

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <set>
#include "../Record.h"
#include "../RecordReference.h"

/**
 * @brief Bucket para Hash Extensible con soporte RecordReference
 * 
 * ✅ MEJORAS IMPLEMENTADAS:
 * - Almacena RecordReference en lugar de Record completo
 * - Optimiza uso de memoria en índices
 * - Soporte para resolución lazy de registros
 * - Manejo de profundidad local
 * - Estadísticas detalladas para análisis educativo
 */
class Bucket {
private:
    std::vector<std::pair<std::string, RecordReference>> entries; // Clave -> RecordReference (modo simple)
    std::unordered_map<std::string, std::vector<RecordReference>> entries_multiple; // Clave -> Vector de RecordReference (modo múltiple)
    int local_depth;                                              // Profundidad local del bucket
    int capacity;                                                 // Capacidad máxima
    size_t access_count;                                          // Contador de accesos (estadísticas)
    bool multiple_mode;                                           // Flag para modo múltiple

public:
    /**
     * @brief Constructor
     */
    Bucket(int cap = 4, int depth = 0, bool enable_multiple = false) 
        : local_depth(depth)
        , capacity(cap)
        , access_count(0)
        , multiple_mode(enable_multiple)
    {
        entries.reserve(capacity);
    }

    // ============================================================================
    // OPERACIONES BÁSICAS DEL BUCKET
    // ============================================================================
    
    /**
     * @brief ✅ Inserta entrada usando RecordReference
     */
    bool insert(const std::string& key, const RecordReference& record_ref) {
        if (isFull()) {
            return false;
        }

        // Verificar si la clave ya existe
        for (auto& entry : entries) {
            if (entry.first == key) {
                // Actualizar referencia existente
                entry.second = record_ref;
                return true;
            }
        }

        // Insertar nueva entrada
        entries.emplace_back(key, record_ref);
        return true;
    }

    /**
     * @brief Busca una clave y retorna su RecordReference
     */
    bool search(const std::string& key, RecordReference& record_ref) {
        access_count++;
        
        for (const auto& entry : entries) {
            if (entry.first == key) {
                record_ref = entry.second;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Elimina una entrada por clave
     */
    bool remove(const std::string& key) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [&key](const auto& entry) { return entry.first == key; });
        
        if (it != entries.end()) {
            entries.erase(it);
            return true;
        }
        return false;
    }

    // ============================================================================
    // GESTIÓN DE SPLITS
    // ============================================================================
    
    /**
     * @brief Divide el bucket en dos cuando está lleno
     */
    std::shared_ptr<Bucket> split() {
        if (!isFull()) {
            return nullptr;
        }

        // Crear nuevo bucket con profundidad local incrementada
        auto new_bucket = std::make_shared<Bucket>(capacity, local_depth + 1);
        
        // Incrementar profundidad local del bucket actual
        local_depth++;

        // Redistribuir entradas basado en el nuevo bit
        std::vector<std::pair<std::string, RecordReference>> old_entries = entries;
        entries.clear();

        size_t bit_mask = 1ULL << (local_depth - 1);

        for (const auto& entry : old_entries) {
            size_t hash_val = std::hash<std::string>{}(entry.first);
            
            if (hash_val & bit_mask) {
                // Va al nuevo bucket
                new_bucket->entries.push_back(entry);
            } else {
                // Se queda en el bucket actual
                entries.push_back(entry);
            }
        }

        std::cout << "🔄 Bucket split: " << entries.size() << " + " 
                  << new_bucket->entries.size() << " entradas" << std::endl;

        return new_bucket;
    }

    /**
     * @brief Verifica si el bucket necesita split para nueva entrada
     */
    bool needsSplit(const std::string& key) const {
        if (!isFull()) {
            return false;
        }

        // Verificar si la clave ya existe (no necesita split)
        for (const auto& entry : entries) {
            if (entry.first == key) {
                return false;
            }
        }

        return true; // Bucket lleno y clave nueva
    }

    // ============================================================================
    // ESTADO Y PROPIEDADES
    // ============================================================================
    
    bool isFull() const { return entries.size() >= static_cast<size_t>(capacity); }
    bool isEmpty() const { return entries.empty(); }
    size_t size() const { return entries.size(); }
    int getCapacity() const { return capacity; }
    int getLocalDepth() const { return local_depth; }
    void setLocalDepth(int depth) { local_depth = depth; }
    size_t getAccessCount() const { return access_count; }

    /**
     * @brief Factor de ocupación del bucket
     */
    double getOccupancyFactor() const {
        return static_cast<double>(entries.size()) / capacity;
    }

    // ============================================================================
    // ACCESO A DATOS
    // ============================================================================
    
    /**
     * @brief Obtiene todas las claves en el bucket
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        keys.reserve(entries.size());
        
        for (const auto& entry : entries) {
            keys.push_back(entry.first);
        }
        
        return keys;
    }

    /**
     * @brief Obtiene todas las referencias en el bucket
     */
    std::vector<RecordReference> getAllReferences() const {
        std::vector<RecordReference> refs;
        refs.reserve(entries.size());
        
        for (const auto& entry : entries) {
            refs.push_back(entry.second);
        }
        
        return refs;
    }

    /**
     * @brief Obtiene todas las entradas (clave, referencia)
     */
    const std::vector<std::pair<std::string, RecordReference>>& getAllEntries() const {
        return entries;
    }

    /**
     * @brief Cuenta registros válidos
     */
    size_t getRecordCount() const {
        size_t valid_count = 0;
        for (const auto& entry : entries) {
            if (entry.second.isValid()) {
                valid_count++;
            }
        }
        return valid_count;
    }

    // ============================================================================
    // SERIALIZACIÓN PARA PERSISTENCIA
    // ============================================================================
    
    /**
     * @brief Serializa el bucket para guardar en disco
     */
    std::string serialize() const {
        std::ostringstream oss;
        
        oss << "BUCKET|" << local_depth << "|" << capacity << "|" << entries.size() << std::endl;
        
        for (const auto& entry : entries) {
            oss << "ENTRY|" << entry.first << "|" << entry.second.serialize() << std::endl;
        }
        
        return oss.str();
    }

    /**
     * @brief Deserializa bucket desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string line;
        
        entries.clear();
        
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            std::istringstream line_stream(line);
            std::string type;
            std::getline(line_stream, type, '|');
            
            if (type == "BUCKET") {
                std::string depth_str, cap_str, size_str;
                std::getline(line_stream, depth_str, '|');
                std::getline(line_stream, cap_str, '|');
                std::getline(line_stream, size_str, '|');
                
                local_depth = std::stoi(depth_str);
                capacity = std::stoi(cap_str);
                
            } else if (type == "ENTRY") {
                std::string key, ref_data;
                std::getline(line_stream, key, '|');
                std::getline(line_stream, ref_data);
                
                RecordReference record_ref;
                if (record_ref.deserialize(ref_data)) {
                    entries.emplace_back(key, record_ref);
                }
            }
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra contenido del bucket (educativo)
     */
    void display() const {
        std::cout << "  📦 Bucket (Local Depth: " << local_depth 
                  << ", Size: " << entries.size() << "/" << capacity << ")" << std::endl;
        
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& entry = entries[i];
            std::cout << "    [" << i << "] " << entry.first 
                      << " -> " << entry.second.toString() << std::endl;
        }
        
        if (entries.empty()) {
            std::cout << "    (vacío)" << std::endl;
        }
    }

    /**
     * @brief Información detallada del bucket
     */
    void displayDetailed() const {
        std::cout << "\n📦 BUCKET DETALLADO:" << std::endl;
        std::cout << "   Profundidad local: " << local_depth << std::endl;
        std::cout << "   Capacidad: " << capacity << std::endl;
        std::cout << "   Entradas: " << entries.size() << std::endl;
        std::cout << "   Ocupación: " << std::fixed << std::setprecision(1) 
                  << (getOccupancyFactor() * 100) << "%" << std::endl;
        std::cout << "   Accesos: " << access_count << std::endl;
        std::cout << "   Estado: " << (isFull() ? "LLENO" : (isEmpty() ? "VACÍO" : "PARCIAL")) << std::endl;

        if (!entries.empty()) {
            std::cout << "\n   Entradas:" << std::endl;
            for (size_t i = 0; i < entries.size(); i++) {
                const auto& entry = entries[i];
                size_t hash_val = std::hash<std::string>{}(entry.first);
                
                std::cout << "   [" << i << "] Key: " << entry.first 
                          << " | Hash: " << hash_val 
                          << " | Valid: " << (entry.second.isValid() ? "✓" : "✗") << std::endl;
            }
        }
    }

    /**
     * @brief Estadísticas del bucket
     */
    struct BucketStats {
        size_t entry_count;
        int local_depth;
        int capacity;
        double occupancy_factor;
        size_t access_count;
        bool is_full;
        bool is_empty;
        size_t valid_references;
    };

    BucketStats getStats() const {
        BucketStats stats;
        stats.entry_count = entries.size();
        stats.local_depth = local_depth;
        stats.capacity = capacity;
        stats.occupancy_factor = getOccupancyFactor();
        stats.access_count = access_count;
        stats.is_full = isFull();
        stats.is_empty = isEmpty();
        stats.valid_references = getRecordCount();
        
        return stats;
    }

    // ============================================================================
    // OPERACIONES AVANZADAS
    // ============================================================================
    
    /**
     * @brief Reorganiza entradas por clave (optimización)
     */
    void sortEntries() {
        std::sort(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            });
    }

    /**
     * @brief Limpia referencias inválidas
     */
    size_t cleanInvalidReferences() {
        size_t removed = 0;
        
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [&removed](const auto& entry) {
                    if (!entry.second.isValid()) {
                        removed++;
                        return true;
                    }
                    return false;
                }),
            entries.end()
        );
        
        return removed;
    }

    // ============================================================================
    // ✅ MODO MÚLTIPLE - NUEVAS FUNCIONALIDADES PARA GPS
    // ============================================================================
    
    /**
     * @brief ✅ Activar/desactivar modo múltiple
     */
    void setMultipleMode(bool enabled) {
        multiple_mode = enabled;
        if (enabled) {
            std::cout << "🔄 Bucket configurado en modo múltiple" << std::endl;
        }
    }

    bool isMultipleMode() const {
        return multiple_mode;
    }

    /**
     * @brief ✅ Inserción múltiple - permite varios registros por clave
     */
    bool insertMultiple(const std::string& key, const RecordReference& record_ref) {
        access_count++;
        
        if (!multiple_mode) {
            // Modo de compatibilidad - usar inserción simple
            return insert(key, record_ref);
        }

        // ✅ MODO MÚLTIPLE: Agregar a la lista del IMEI
        entries_multiple[key].push_back(record_ref);
        
        std::cout << "📝 Bucket múltiple: IMEI '" << key << "' ahora tiene " 
                  << entries_multiple[key].size() << " registros" << std::endl;
        
        return true;
    }

    /**
     * @brief ✅ Búsqueda múltiple - retorna todos los registros de una clave
     */
    bool searchMultiple(const std::string& key, std::vector<RecordReference>& results) {
        access_count++;
        
        if (!multiple_mode) {
            // Modo de compatibilidad
            RecordReference single_ref;
            if (search(key, single_ref)) {
                results.push_back(single_ref);
                return true;
            }
            return false;
        }

        auto it = entries_multiple.find(key);
        if (it != entries_multiple.end()) {
            results = it->second;  // Copiar todo el vector
            return !results.empty();
        }
        return false;
    }

    /**
     * @brief ✅ Verificar si necesita split en modo múltiple
     */
    bool needsSplitMultiple(const std::string& key) {
        if (!multiple_mode) {
            return needsSplit(key);
        }

        // En modo múltiple, el split se basa en número de IMEIs únicos, no registros totales
        size_t unique_keys = entries_multiple.size();
        
        // Solo hacer split si ya existe el IMEI Y tenemos muchos IMEIs únicos
        if (entries_multiple.find(key) == entries_multiple.end()) {
            // IMEI nuevo: verificar si cabe
            return (unique_keys >= static_cast<size_t>(capacity));
        }
        
        // IMEI existente: siempre cabe (solo agregar a la lista)
        return false;
    }

    /**
     * @brief ✅ Split para modo múltiple
     */
    std::shared_ptr<Bucket> splitMultiple() {
        if (!multiple_mode) {
            return split();  // Usar split tradicional
        }

        auto new_bucket = std::make_shared<Bucket>(capacity, local_depth + 1, true);
        
        // ✅ REDISTRIBUIR POR HASH DE IMEI
        auto it = entries_multiple.begin();
        while (it != entries_multiple.end()) {
            const std::string& key = it->first;
            auto& record_list = it->second;
            
            // Calcular a qué bucket debe ir cada IMEI
            size_t hash_value = std::hash<std::string>{}(key);
            size_t bit_mask = 1ULL << local_depth;
            
            if (hash_value & bit_mask) {
                // Va al bucket nuevo
                new_bucket->entries_multiple[key] = std::move(record_list);
                it = entries_multiple.erase(it);
            } else {
                // Se queda en bucket actual
                ++it;
            }
        }
        
        // Incrementar profundidad local
        local_depth++;
        
        std::cout << "🔄 Split múltiple completado: " 
                  << entries_multiple.size() << " IMEIs en bucket original, "
                  << new_bucket->entries_multiple.size() << " en bucket nuevo" << std::endl;
        
        return new_bucket;
    }

    /**
     * @brief ✅ Obtener total de registros incluyendo múltiples
     */
    size_t getTotalRecordsMultiple() const {
        if (!multiple_mode) {
            return getRecordCount();
        }

        size_t total = 0;
        for (const auto& pair : entries_multiple) {
            total += pair.second.size();
        }
        return total;
    }

    /**
     * @brief ✅ Obtener número de claves únicas en modo múltiple
     */
    size_t getUniqueKeysCount() const {
        if (multiple_mode) {
            return entries_multiple.size();
        } else {
            return entries.size();
        }
    }

    /**
     * @brief ✅ Estadísticas del modo múltiple
     */
    void displayMultipleStatistics() const {
        if (!multiple_mode) {
            std::cout << "⚠️ Bucket en modo simple" << std::endl;
            return;
        }

        std::cout << "\n📊 ESTADÍSTICAS BUCKET MÚLTIPLE:" << std::endl;
        std::cout << "   Claves únicas (IMEIs): " << entries_multiple.size() << std::endl;
        
        size_t total_records = 0;
        size_t max_records_per_key = 0;
        std::string most_active_key;
        
        for (const auto& pair : entries_multiple) {
            size_t records_count = pair.second.size();
            total_records += records_count;
            
            if (records_count > max_records_per_key) {
                max_records_per_key = records_count;
                most_active_key = pair.first;
            }
        }
        
        std::cout << "   Total registros GPS: " << total_records << std::endl;
        if (!entries_multiple.empty()) {
            std::cout << "   Promedio por IMEI: " << (total_records / entries_multiple.size()) << std::endl;
            std::cout << "   IMEI más activo: " << most_active_key.substr(0, 12) << "... (" 
                      << max_records_per_key << " registros)" << std::endl;
        }
    }

    // ============================================================================
    // VALIDACIÓN Y CONSISTENCIA
    // ============================================================================

    /**
     * @brief Verifica consistencia interna del bucket (incluyendo modo múltiple)
     */
    bool validateConsistency() const {
        if (multiple_mode) {
            // Verificar modo múltiple
            if (entries_multiple.size() > static_cast<size_t>(capacity)) {
                std::cout << "❌ Bucket múltiple excede capacidad: " << entries_multiple.size() 
                          << " > " << capacity << std::endl;
                return false;
            }
            
            // Verificar que no hay listas vacías
            for (const auto& pair : entries_multiple) {
                if (pair.second.empty()) {
                    std::cout << "❌ Lista vacía para clave: " << pair.first << std::endl;
                    return false;
                }
            }
        } else {
            // Verificar modo simple
            if (entries.size() > static_cast<size_t>(capacity)) {
                std::cout << "❌ Bucket excede capacidad: " << entries.size() << " > " << capacity << std::endl;
                return false;
            }

            // Verificar claves únicas
            std::set<std::string> unique_keys;
            for (const auto& entry : entries) {
                if (unique_keys.find(entry.first) != unique_keys.end()) {
                    std::cout << "❌ Clave duplicada en bucket: " << entry.first << std::endl;
                    return false;
                }
                unique_keys.insert(entry.first);
            }
        }

        return true;
    }
};

#endif // BUCKET_H