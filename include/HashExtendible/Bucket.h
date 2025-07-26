#ifndef BUCKET_H
#define BUCKET_H

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
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
     * @brief Inserta un registro en el bucket
     */
    bool insert(const std::string& key, std::unique_ptr<Record> record) {
        if (isFull()) {
            return false;
        }
        
        // Verificar si la clave ya existe
        for (auto& entry : entries) {
            if (entry.key == key) {
                // Actualizar registro existente
                entry.record = std::move(record);
                return true;
            }
        }
        
        // Insertar nuevo registro
        entries.emplace_back(key, std::move(record));
        record_count++;
        
        return true;
    }
    
    /**
     * @brief Inserta un registro con RecordReference específico
     */
    bool insertRecord(const std::string& key, std::unique_ptr<Record> record) {
        if (isFull()) {
            return false;
        }
        
        entries.emplace_back(key, std::move(record));
        record_count++;
        return true;
    }
    
    /**
     * @brief Busca un registro por clave
     */
    bool search(const std::string& key, Record& record) {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                // Copiar datos del registro encontrado
                record = *(entry.record);
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Busca y retorna RecordReference
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
     * @brief Elimina un registro
     */
    bool remove(const std::string& key) {
        auto it = std::find_if(entries.begin(), entries.end(),
                              [&key](const BucketEntry& entry) {
                                  return entry.key == key;
                              });
        
        if (it != entries.end()) {
            entries.erase(it);
            record_count--;
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Limpia todo el bucket
     */
    void clear() {
        entries.clear();
        record_count = 0;
    }

    // ============================================================================
    // ESTADO Y INFORMACIÓN DEL BUCKET
    // ============================================================================
    
    /**
     * @brief Verifica si el bucket está lleno
     */
    bool isFull() const {
        return record_count >= capacity;
    }
    
    /**
     * @brief Verifica si el bucket está vacío
     */
    bool isEmpty() const {
        return record_count == 0;
    }
    
    /**
     * @brief Obtiene el número de registros
     */
    size_t getRecordCount() const {
        return record_count;
    }
    
    /**
     * @brief Obtiene la capacidad del bucket
     */
    int getCapacity() const {
        return capacity;
    }
    
    /**
     * @brief Obtiene la profundidad local
     */
    int getLocalDepth() const {
        return local_depth;
    }
    
    /**
     * @brief Establece la profundidad local
     */
    void setLocalDepth(int depth) {
        local_depth = depth;
    }
    
    /**
     * @brief Obtiene factor de carga del bucket
     */
    double getLoadFactor() const {
        return (double)record_count / capacity;
    }

    // ============================================================================
    // FUNCIONES PARA SPLIT Y REDISTRIBUCIÓN
    // ============================================================================
    
    /**
     * @brief Obtiene todas las entradas (para redistribución)
     */
    std::vector<BucketEntry> getAllRecords() {
        std::vector<BucketEntry> result;
        
        // Mover todas las entradas
        for (auto& entry : entries) {
            result.emplace_back(entry.key, std::move(entry.record), entry.record_ref);
        }
        
        return result;
    }
    
    /**
     * @brief Obtiene todas las claves
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        for (const auto& entry : entries) {
            keys.push_back(entry.key);
        }
        return keys;
    }
    
    /**
     * @brief Obtiene muestra de claves para visualización
     */
    std::vector<std::string> getSampleKeys(size_t max_keys = 3) const {
        std::vector<std::string> keys;
        size_t count = std::min(max_keys, entries.size());
        
        for (size_t i = 0; i < count; i++) {
            keys.push_back(entries[i].key);
        }
        
        return keys;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra el contenido del bucket
     */
    void display() const {
        std::cout << "🪣 BUCKET (Prof. Local: " << local_depth 
                  << ", Registros: " << record_count << "/" << capacity << ")" << std::endl;
        
        if (isEmpty()) {
            std::cout << "   [Vacío]" << std::endl;
            return;
        }
        
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& entry = entries[i];
            std::cout << "   [" << i << "] Clave: " << entry.key.substr(0, 20) << "..." 
                      << " | Ref: " << entry.record_ref << std::endl;
        }
        
        std::cout << "   Factor de carga: " << std::fixed << std::setprecision(2) 
                  << getLoadFactor() * 100 << "%" << std::endl;
    }
    
    /**
     * @brief Obtiene información detallada del bucket
     */
    std::string getDetailedInfo() const {
        std::stringstream ss;
        
        ss << "Bucket Details:\n";
        ss << "  Local Depth: " << local_depth << "\n";
        ss << "  Capacity: " << capacity << "\n";
        ss << "  Current Records: " << record_count << "\n";
        ss << "  Load Factor: " << std::fixed << std::setprecision(2) 
           << getLoadFactor() * 100 << "%\n";
        ss << "  Status: " << (isFull() ? "FULL" : (isEmpty() ? "EMPTY" : "PARTIAL")) << "\n";
        
        if (!isEmpty()) {
            ss << "  Sample Keys:\n";
            for (size_t i = 0; i < std::min((size_t)3, entries.size()); i++) {
                ss << "    " << entries[i].key.substr(0, 30) << "...\n";
            }
        }
        
        return ss.str();
    }
    
    /**
     * @brief Valida la integridad del bucket
     */
    bool validateIntegrity() const {
        // Verificar que el conteo coincida
        if (record_count != entries.size()) {
            std::cout << "❌ Error: record_count (" << record_count 
                      << ") no coincide con entries.size() (" << entries.size() << ")" << std::endl;
            return false;
        }
        
        // Verificar que no exceda la capacidad
        if (record_count > capacity) {
            std::cout << "❌ Error: record_count (" << record_count 
                      << ") excede la capacidad (" << capacity << ")" << std::endl;
            return false;
        }
        
        // Verificar que no hay claves duplicadas
        std::set<std::string> unique_keys;
        for (const auto& entry : entries) {
            if (unique_keys.count(entry.key) > 0) {
                std::cout << "❌ Error: Clave duplicada encontrada: " << entry.key << std::endl;
                return false;
            }
            unique_keys.insert(entry.key);
        }
        
        // Verificar que todos los registros existen
        for (const auto& entry : entries) {
            if (!entry.record) {
                std::cout << "❌ Error: Registro nulo para clave: " << entry.key << std::endl;
                return false;
            }
        }
        
        return true;
    }

    // ============================================================================
    // ESTADÍSTICAS PARA ANÁLISIS
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas de distribución de hash
     */
    std::map<std::string, int> getHashDistribution() const {
        std::map<std::string, int> distribution;
        
        for (const auto& entry : entries) {
            size_t hash_value = std::hash<std::string>{}(entry.key);
            std::string hash_prefix = std::to_string(hash_value).substr(0, 3);
            distribution[hash_prefix]++;
        }
        
        return distribution;
    }
    
    /**
     * @brief Calcula eficiencia del bucket
     */
    double getEfficiency() const {
        if (capacity == 0) return 0.0;
        return (double)record_count / capacity;
    }
    
    /**
     * @brief Predice si necesitará split pronto
     */
    bool needsSplitSoon() const {
        return getLoadFactor() > 0.75; // Más del 75% lleno
    }
};

#endif // BUCKET_H