#ifndef BUCKET_H
#define BUCKET_H

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <set>  
#include <sstream>
#include <iomanip>
#include "../Record.h"
#include "../RecordReference.h"

/**
 * @brief Entrada en el bucket
 */
struct BucketEntry {
    std::string key;                           // Clave hash (ej: IMEI)
    std::unique_ptr<Record> record;           // Registro completo
    RecordReference record_ref;               // Referencia al disco
    
    BucketEntry(const std::string& k, std::unique_ptr<Record> r) 
        : key(k), record(std::move(r)) {
        // Crear RecordReference simulado
        PhysicalAddress addr(0, 0, 0, rand() % 100);
        record_ref = RecordReference(addr, rand() % 10);
    }
    
    BucketEntry(const std::string& k, std::unique_ptr<Record> r, const RecordReference& ref)
        : key(k), record(std::move(r)), record_ref(ref) {}
    
    // Constructor de movimiento
    BucketEntry(BucketEntry&& other) noexcept 
        : key(std::move(other.key)), record(std::move(other.record)), record_ref(other.record_ref) {}
    
    // Operador de asignación de movimiento
    BucketEntry& operator=(BucketEntry&& other) noexcept {
        if (this != &other) {
            key = std::move(other.key);
            record = std::move(other.record);
            record_ref = other.record_ref;
        }
        return *this;
    }
    
    // Eliminar constructor de copia y asignación (unique_ptr no se puede copiar)
    BucketEntry(const BucketEntry&) = delete;
    BucketEntry& operator=(const BucketEntry&) = delete;
};

/**
 * @brief Bucket para Hash Extensible
 * 
 * Implementación educativa del bucket que:
 * - Almacena hasta 'capacity' registros
 * - Mantiene profundidad local
 * - Soporta búsqueda, inserción y eliminación
 * - Proporciona funciones para split
 */
class Bucket {
private:
    std::vector<BucketEntry> entries;         // Entradas en el bucket
    int capacity;                             // Capacidad máxima
    int local_depth;                          // Profundidad local del bucket
    size_t record_count;                      // Número actual de registros

public:
    /**
     * @brief Constructor
     */
    Bucket(int cap = 4, int depth = 0) 
        : capacity(cap), local_depth(depth), record_count(0) {
        entries.reserve(capacity);
    }

    // ============================================================================
    // OPERACIONES BÁSICAS DEL BUCKET
    // ============================================================================
    
    /**
     * @brief Insertar registro en el bucket
     */
    bool insertRecord(const std::string& key, std::unique_ptr<Record> record) {
        if (isFull()) {
            std::cout << "   ❌ Bucket lleno (capacidad: " << capacity << ")" << std::endl;
            return false;
        }
        
        // Verificar si la clave ya existe
        for (const auto& entry : entries) {
            if (entry.key == key) {
                std::cout << "   ⚠️ Clave duplicada: " << key.substr(0, 15) << "..." << std::endl;
                return false;
            }
        }
        
        // Usar std::move para transferir ownership
        entries.emplace_back(key, std::move(record));
        record_count++;
        
        std::cout << "   ✅ Registro insertado: " << key.substr(0, 15) 
                  << "... (Total: " << record_count << "/" << capacity << ")" << std::endl;
        
        return true;
    }
    
    /**
     * @brief Insertar registro (sobrecarga para compatibilidad)
     */
    bool insert(const std::string& key, std::unique_ptr<Record> record) {
        return insertRecord(key, std::move(record));
    }
    
    /**
     * @brief Buscar registro por clave
     */
    bool search(const std::string& key, Record& result) {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                // Crear copia del registro para el resultado
                result = *entry.record;
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Buscar y obtener RecordReference
     */
    bool searchReference(const std::string& key, RecordReference& record_ref) {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                record_ref = entry.record_ref;
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Eliminar registro por clave
     */
    bool remove(const std::string& key) {
        auto it = std::remove_if(entries.begin(), entries.end(),
            [&key](const BucketEntry& entry) {
                return entry.key == key;
            });
        
        if (it != entries.end()) {
            entries.erase(it, entries.end());
            record_count--;
            std::cout << "   🗑️ Registro eliminado: " << key.substr(0, 15) << "..." << std::endl;
            return true;
        }
        
        return false;
    }
    
    // ============================================================================
    // OPERACIONES PARA SPLIT Y EXTRACCIÓN
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Obtiene todas las claves del bucket
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        keys.reserve(entries.size());
        
        for (const auto& entry : entries) {
            keys.push_back(entry.key);
        }
        
        return keys;
    }
    
    /**
     * @brief Obtiene todos los registros (para redistribución)
     */
    std::vector<std::reference_wrapper<BucketEntry>> getAllRecords() {
        std::vector<std::reference_wrapper<BucketEntry>> refs;
        for (auto& entry : entries) {
            refs.push_back(std::ref(entry));
        }
        return refs;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Extrae todas las entradas (para redistribución)
     */
    std::vector<BucketEntry> extractAllEntries() {
        std::vector<BucketEntry> extracted_entries;
        extracted_entries.reserve(entries.size());
        
        // Mover todas las entradas al vector de extracción
        for (auto& entry : entries) {
            extracted_entries.emplace_back(std::move(entry));
        }
        
        // Limpiar el bucket
        entries.clear();
        record_count = 0;
        
        return extracted_entries;
    }
    
    /**
     * @brief Limpia el bucket
     */
    void clear() {
        entries.clear();
        record_count = 0;
        std::cout << "   🧹 Bucket limpiado" << std::endl;
    }
    
    /**
     * @brief Incrementa la profundidad local
     */
    void incrementLocalDepth() {
        local_depth++;
        std::cout << "   📈 Profundidad local incrementada a: " << local_depth << std::endl;
    }
    
    // ============================================================================
    // GETTERS Y ESTADO
    // ============================================================================
    
    bool isFull() const { return record_count >= capacity; }
    bool isEmpty() const { return record_count == 0; }
    size_t getRecordCount() const { return record_count; }
    int getLocalDepth() const { return local_depth; }
    int getCapacity() const { return capacity; }
    
    /**
     * @brief Obtiene factor de carga del bucket
     */
    double getLoadFactor() const {
        return capacity > 0 ? (double)record_count / capacity : 0.0;
    }
    
    // ============================================================================
    // VALIDACIÓN Y DEPURACIÓN
    // ============================================================================
    
    /**
     * @brief Valida integridad del bucket
     */
    bool validateIntegrity() const {
        // Verificar que no hay claves duplicadas
        std::set<std::string> unique_keys;
        for (const auto& entry : entries) {
            if (unique_keys.find(entry.key) != unique_keys.end()) {
                std::cout << "❌ Clave duplicada encontrada: " << entry.key << std::endl;
                return false;
            }
            unique_keys.insert(entry.key);
        }
        
        // Verificar consistencia de contadores
        if (entries.size() != record_count) {
            std::cout << "❌ Inconsistencia en contadores: entries=" << entries.size() 
                      << ", record_count=" << record_count << std::endl;
            return false;
        }
        
        // Verificar capacidad
        if (record_count > capacity) {
            std::cout << "❌ Bucket sobrecargado: " << record_count << "/" << capacity << std::endl;
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Muestra contenido del bucket
     */
    void display() const {
        std::cout << "📦 Bucket (Profundidad local: " << local_depth 
                  << ", Registros: " << record_count << "/" << capacity << ")" << std::endl;
        
        for (size_t i = 0; i < entries.size(); i++) {
            std::cout << "   [" << i << "] " << entries[i].key.substr(0, 20) 
                      << "... → " << entries[i].record_ref.toString() << std::endl;
        }
    }
    
    /**
     * @brief Obtiene estadísticas del bucket
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        ss << "Bucket Statistics:\n";
        ss << "  Local Depth: " << local_depth << "\n";
        ss << "  Records: " << record_count << "/" << capacity << "\n";
        ss << "  Load Factor: " << std::fixed << std::setprecision(2) << getLoadFactor() << "\n";
        ss << "  Is Full: " << (isFull() ? "Yes" : "No") << "\n";
        ss << "  Is Empty: " << (isEmpty() ? "Yes" : "No") << "\n";
        return ss.str();
    }
    
    // ============================================================================
    // ✅ FUNCIONES AGREGADAS - ANÁLISIS Y BÚSQUEDA AVANZADA
    // ============================================================================
    
    /**
     * @brief Busca múltiples claves de una vez
     */
    std::vector<RecordReference> searchMultiple(const std::vector<std::string>& keys) {
        std::vector<RecordReference> results;
        results.reserve(keys.size());
        
        for (const auto& key : keys) {
            RecordReference ref;
            if (searchReference(key, ref)) {
                results.push_back(ref);
            }
        }
        
        return results;
    }
    
    /**
     * @brief Obtiene estadísticas de distribución de claves
     */
    std::map<char, int> getKeyPrefixDistribution() const {
        std::map<char, int> distribution;
        
        for (const auto& entry : entries) {
            if (!entry.key.empty()) {
                char prefix = entry.key[0];
                distribution[prefix]++;
            }
        }
        
        return distribution;
    }
    
    /**
     * @brief Verifica si el bucket contiene una clave específica
     */
    bool containsKey(const std::string& key) const {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Obtiene información detallada de una entrada específica
     */
    std::string getEntryInfo(const std::string& key) const {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                std::ostringstream ss;
                ss << "Entry Info for key: " << key << "\n";
                ss << "  Record Reference: " << entry.record_ref.toString() << "\n";
                ss << "  Record Size: " << entry.record->getSize() << " bytes\n";
                ss << "  Bucket Local Depth: " << local_depth << "\n";
                return ss.str();
            }
        }
        return "Entry not found for key: " + key;
    }
};

#endif // BUCKET_H