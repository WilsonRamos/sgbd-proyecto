#ifndef EXTENSIBLE_HASH_H
#define EXTENSIBLE_HASH_H

#include <string>
#include <memory>
#include <iostream>
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
     * @brief Buscar y obtener RecordReference (para integración con SGBD)
     */
    bool searchReference(const std::string& key, RecordReference& record_ref) {
        search_operations++;
        
        // FLUJO EDUCATIVO: Mostrar proceso de búsqueda
        std::cout << "\n🔍 FLUJO DE BÚSQUEDA HASH EXTENSIBLE:" << std::endl;
        std::cout << "1️⃣ Clave buscada: " << key << std::endl;
        
        // Calcular hash
        size_t hash_value = std::hash<std::string>{}(key);
        int global_depth = directory->getGlobalDepth();
        size_t mask = (1 << global_depth) - 1;
        size_t bucket_index = hash_value & mask;
        
        std::cout << "2️⃣ Hash calculado: " << hash_value << std::endl;
        std::cout << "3️⃣ Profundidad global: " << global_depth << std::endl;
        std::cout << "4️⃣ Índice de bucket: " << bucket_index << std::endl;
        
        auto bucket = directory->getBucket(key);
        std::cout << "5️⃣ Bucket localizado, buscando clave..." << std::endl;
        
        // Simular búsqueda en bucket
        bool found = bucket->searchReference(key, record_ref);
        
        if (found) {
            std::cout << "6️⃣ ✅ Registro encontrado!" << std::endl;
            std::cout << "   RecordReference: " << record_ref << std::endl;
        } else {
            std::cout << "6️⃣ ❌ Registro no encontrado en bucket" << std::endl;
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
     * @brief Verifica si una inserción causará split (para mostrar progreso)
     */
    bool willCauseSplit(const std::string& key) {
        auto bucket = directory->getBucket(key);
        return bucket->isFull();
    }
    
    // ============================================================================
    // ESTADÍSTICAS Y VISUALIZACIÓN
    // ============================================================================
    
    /**
     * @brief Muestra estadísticas detalladas
     */
    void displayStatistics() const {
        std::cout << "\n📊 ESTADÍSTICAS HASH EXTENDIBLE 📊" << std::endl;
        std::cout << "Registros totales: " << total_records << std::endl;
        std::cout << "Operaciones de inserción: " << insert_operations << std::endl;
        std::cout << "Operaciones de búsqueda: " << search_operations << std::endl;
        std::cout << "Divisiones de bucket: " << split_operations << std::endl;
        std::cout << "Profundidad global: " << directory->getGlobalDepth() << std::endl;
        std::cout << "Entradas en directorio: " << directory->getSize() << std::endl;
        
        if (insert_operations > 0) {
            std::cout << "Tasa de división: " << (double)split_operations / insert_operations * 100 << "%" << std::endl;
        }
        
        // Factor de carga
        int directory_size = directory->getSize();
        double load_factor = (double)total_records / (directory_size * bucket_capacity);
        std::cout << "Factor de carga: " << load_factor << std::endl;
    }
    
    /**
     * @brief Muestra estructura del directorio (educativo)
     */
    void displayStructure() const {
        std::cout << "\n🏗️ ESTRUCTURA DEL HASH EXTENSIBLE:" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        
        directory->display();
        
        std::cout << "\n📋 RESUMEN DE BUCKETS:" << std::endl;
        int directory_size = directory->getSize();
        for (int i = 0; i < directory_size; i++) {
            auto bucket = directory->getBucketByIndex(i);
            std::cout << "Entrada " << i << " -> Bucket con " 
                      << bucket->getRecordCount() << "/" << bucket_capacity << " registros" << std::endl;
        }
    }
    
    // ============================================================================
    // GETTERS PARA PERSISTENCIA
    // ============================================================================
    
    size_t getTotalRecords() const { return total_records; }
    size_t getSplitOperations() const { return split_operations; }
    size_t getSearchOperations() const { return search_operations; }
    int getGlobalDepth() const { return directory->getGlobalDepth(); }
    int getBucketCapacity() const { return bucket_capacity; }
    
    /**
     * @brief Obtiene todas las claves para exportación (simplificado)
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        
        // Recorrer todos los buckets
        int directory_size = directory->getSize();
        for (int i = 0; i < directory_size; i++) {
            auto bucket = directory->getBucketByIndex(i);
            auto bucket_keys = bucket->getAllKeys();
            keys.insert(keys.end(), bucket_keys.begin(), bucket_keys.end());
        }
        
        return keys;
    }
    
    /**
     * @brief Información de distribución de buckets
     */
    std::string getBucketDistribution() const {
        std::stringstream ss;
        int directory_size = directory->getSize();
        
        ss << "Directory Size: " << directory_size << "\n";
        ss << "Global Depth: " << directory->getGlobalDepth() << "\n";
        ss << "Bucket Capacity: " << bucket_capacity << "\n";
        
        // Estadísticas por bucket
        std::map<int, int> bucket_loads;
        for (int i = 0; i < directory_size; i++) {
            auto bucket = directory->getBucketByIndex(i);
            int load = bucket->getRecordCount();
            bucket_loads[load]++;
        }
        
        ss << "Load Distribution:\n";
        for (const auto& pair : bucket_loads) {
            ss << "  " << pair.first << " records: " << pair.second << " buckets\n";
        }
        
        return ss.str();
    }
    
    // ============================================================================
    // BÚSQUEDAS ESPECIALES PARA INTEGRACIÓN CON SGBD
    // ============================================================================
    
    /**
     * @brief Búsqueda con seguimiento de accesos al disco
     */
    bool searchWithDiskAccess(const std::string& key, RecordReference& record_ref,
                              std::function<bool(const RecordReference&)> disk_accessor) {
        
        std::cout << "\n🎯 BÚSQUEDA INTEGRADA HASH → DISCO:" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;
        
        // Paso 1: Búsqueda en índice
        if (!searchReference(key, record_ref)) {
            std::cout << "❌ Clave no encontrada en índice hash" << std::endl;
            return false;
        }
        
        // Paso 2: Acceso al disco usando RecordReference
        std::cout << "7️⃣ Accediendo al disco..." << std::endl;
        std::cout << "   Page ID: " << record_ref.toPageId() << std::endl;
        std::cout << "   Physical Address: " << record_ref.getPhysicalAddress() << std::endl;
        std::cout << "   Slot ID: " << record_ref.getSlotId() << std::endl;
        
        // Llamar al accessor del disco
        bool disk_success = disk_accessor(record_ref);
        
        if (disk_success) {
            std::cout << "8️⃣ ✅ Registro recuperado exitosamente desde disco" << std::endl;
        } else {
            std::cout << "8️⃣ ❌ Error accediendo al registro en disco" << std::endl;
        }
        
        return disk_success;
    }
};

#endif // EXTENSIBLE_HASH_H