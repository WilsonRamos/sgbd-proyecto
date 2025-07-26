#ifndef EXTENSIBLE_HASH_H
#define EXTENSIBLE_HASH_H

#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Directory.h"
#include "Bucket.h"
#include "HashFunction.h"
#include "../Record.h"
#include "../RecordReference.h"

/**
 * @brief Hash Extensible con soporte para persistencia
 * 
 * Implementación educativa de Extendible Hashing con:
 * - Directorio dinámico
 * - Buckets con capacidad configurable
 * - División automática de buckets
 * - Estadísticas detalladas
 * - Soporte para persistencia (nuevo)
 */
class ExtensibleHash {
private:
    std::unique_ptr<Directory> directory;
    int bucket_capacity;
    size_t total_records;
    
    // Estadísticas
    size_t insert_operations;
    size_t search_operations;
    size_t split_operations;

public:
    /**
     * @brief Constructor
     */
    ExtensibleHash(int capacity = 4) 
        : bucket_capacity(capacity)
        , total_records(0)
        , insert_operations(0)
        , search_operations(0)
        , split_operations(0) {
        
        directory = std::make_unique<Directory>(capacity);
        std::cout << "🔗 Hash Extendible inicializado (capacidad: " << capacity << ")" << std::endl;
    }
    
    // ============================================================================
    // OPERACIONES BÁSICAS DE HASH
    // ============================================================================
    
    /**
     * @brief Insertar registro en el hash
     */
    bool insert(const std::string& key, std::unique_ptr<Record> record) {
        insert_operations++;
        
        auto bucket = directory->getBucket(key);
        
        // Crear copia para el caso de división
        auto record_copy = record->clone();
        
        // Intentar insertar directamente
        if (bucket->insert(key, std::move(record))) {
            total_records++;
            return true;
        }
        
        // Bucket lleno - necesita división
        if (bucket->isFull()) {
            split_operations++;
            std::cout << "🔄 Dividiendo bucket para clave: " << key.substr(0, 20) << "..." << std::endl;
            
            if (directory->splitBucket(key)) {
                // Reintentamos después de la división
                auto new_bucket = directory->getBucket(key);
                if (new_bucket->insert(key, std::move(record_copy))) {
                    total_records++;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * @brief Buscar registro por clave
     */
    bool search(const std::string& key, Record& record) {
        search_operations++;
        
        auto bucket = directory->getBucket(key);
        return bucket->search(key, record);
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Buscar y obtener RecordReference
     */
    bool searchReference(const std::string& key, RecordReference& record_ref) {
        search_operations++;
        
        std::cout << "\n🔍 BÚSQUEDA HASH EXTENSIBLE:" << std::endl;
        std::cout << "Clave: " << key.substr(0, 20) << "..." << std::endl;
        
        auto bucket = directory->getBucket(key);
        bool found = bucket->searchReference(key, record_ref);
        
        if (found) {
            std::cout << "✅ Registro encontrado" << std::endl;
        } else {
            std::cout << "❌ Registro no encontrado" << std::endl;
        }
        
        return found;
    }
    
    /**
     * @brief Eliminar registro
     */
    bool remove(const std::string& key) {
        auto bucket = directory->getBucket(key);
        if (bucket->remove(key)) {
            total_records--;
            return true;
        }
        return false;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Búsqueda con acceso a disco simulado
     */
    bool searchWithDiskAccess(const std::string& key, RecordReference& record_ref, 
                              std::function<bool(const RecordReference&)> disk_loader = nullptr) {
        
        bool found = searchReference(key, record_ref);
        
        if (found && disk_loader) {
            std::cout << "🔍 Acceso a disco para cargar registro completo..." << std::endl;
            std::cout << "   Page ID: " << record_ref.getPhysicalAddress().toString() << std::endl;
            
            // Simular carga desde disco
            bool loaded = disk_loader(record_ref);
            std::cout << "📀 Carga desde disco: " << (loaded ? "✅ Exitosa" : "❌ Falló") << std::endl;
        }
        
        return found;
    }
    
    // ============================================================================
    // ✅ FUNCIONES AGREGADAS - ESTADÍSTICAS Y VISUALIZACIÓN
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas detalladas como string
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        ss << "=== ESTADÍSTICAS HASH EXTENSIBLE ===\n";
        ss << "Registros totales: " << total_records << "\n";
        ss << "Operaciones de inserción: " << insert_operations << "\n";
        ss << "Operaciones de búsqueda: " << search_operations << "\n";
        ss << "Divisiones de bucket: " << split_operations << "\n";
        ss << "Profundidad global: " << directory->getGlobalDepth() << "\n";
        ss << "Entradas en directorio: " << directory->getSize() << "\n";
        
        if (insert_operations > 0) {
            double split_rate = (double)split_operations / insert_operations * 100;
            ss << "Tasa de división: " << std::fixed << std::setprecision(2) << split_rate << "%\n";
        }
        
        // Factor de carga
        auto unique_buckets = directory->getUniqueBuckets();
        if (!unique_buckets.empty()) {
            double load_factor = (double)total_records / (unique_buckets.size() * bucket_capacity);
            ss << "Factor de carga: " << std::fixed << std::setprecision(2) << load_factor << "\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Muestra estadísticas detalladas
     */
    void displayStatistics() const {
        std::cout << getStatistics() << std::endl;
    }
    
    /**
     * @brief Muestra estructura del directorio (educativo)
     */
    void displayStructure() const {
        std::cout << "\n🏗️ ESTRUCTURA DEL HASH EXTENSIBLE:" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        
        directory->display();
        
        std::cout << "\n📋 RESUMEN DE BUCKETS:" << std::endl;
        auto unique_buckets = directory->getUniqueBuckets();
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            std::cout << "Bucket " << i << " -> " 
                      << unique_buckets[i]->getRecordCount() << "/" << bucket_capacity << " registros" << std::endl;
        }
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Obtiene todas las claves
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        auto unique_buckets = directory->getUniqueBuckets();
        
        for (const auto& bucket : unique_buckets) {
            auto bucket_keys = bucket->getAllKeys();
            keys.insert(keys.end(), bucket_keys.begin(), bucket_keys.end());
        }
        
        return keys;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Información de distribución de buckets
     */
    std::string getBucketDistribution() const {
        std::ostringstream ss;
        auto unique_buckets = directory->getUniqueBuckets();
        
        ss << "Directory Size: " << directory->getSize() << "\n";
        ss << "Global Depth: " << directory->getGlobalDepth() << "\n";
        ss << "Unique Buckets: " << unique_buckets.size() << "\n";
        ss << "Bucket Capacity: " << bucket_capacity << "\n";
        
        // Estadísticas por bucket
        std::map<int, int> bucket_loads;
        for (const auto& bucket : unique_buckets) {
            int load = bucket->getRecordCount();
            bucket_loads[load]++;
        }
        
        ss << "Load Distribution:\n";
        for (const auto& pair : bucket_loads) {
            ss << "  " << pair.first << " records: " << pair.second << " buckets\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Verifica si una inserción causará split (para mostrar progreso)
     */
    bool willCauseSplit(const std::string& key) {
        auto bucket = directory->getBucket(key);
        return bucket->isFull();
    }
    
    // ============================================================================
    // GETTERS PARA PERSISTENCIA E INTEGRACIÓN
    // ============================================================================
    
    size_t getTotalRecords() const { return total_records; }
    size_t getSplitOperations() const { return split_operations; }
    size_t getSearchOperations() const { return search_operations; }
    size_t getInsertOperations() const { return insert_operations; }
    int getGlobalDepth() const { return directory->getGlobalDepth(); }
    int getBucketCapacity() const { return bucket_capacity; }
    
    /**
     * @brief Obtiene referencia al directorio (para persistencia avanzada)
     */
    const Directory& getDirectory() const { return *directory; }
};

#endif // EXTENSIBLE_HASH_H