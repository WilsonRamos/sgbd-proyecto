#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include "Bucket.h"
#include "HashFunction.h"

/**
 * @brief Directorio para Hash Extensible
 * 
 * Implementación educativa del directorio que:
 * - Mantiene punteros a buckets
 * - Maneja la profundidad global
 * - Coordina las divisiones de buckets
 * - Proporciona acceso O(1) a buckets
 */
class Directory {
private:
    std::vector<std::shared_ptr<Bucket>> directory;  // Array de punteros a buckets
    int global_depth;                                // Profundidad global del directorio
    int bucket_capacity;                             // Capacidad de cada bucket
    size_t directory_size;                           // Tamaño actual del directorio

public:
    /**
     * @brief Constructor
     */
    Directory(int capacity = 4) : global_depth(0), bucket_capacity(capacity) {
        directory_size = 1 << global_depth; // 2^global_depth
        directory.resize(directory_size);
        
        // Crear bucket inicial
        auto initial_bucket = std::make_shared<Bucket>(bucket_capacity, 0);
        for (size_t i = 0; i < directory_size; i++) {
            directory[i] = initial_bucket;
        }
        
        std::cout << "📁 Directorio inicializado (profundidad: " << global_depth 
                  << ", tamaño: " << directory_size << ")" << std::endl;
    }

    // ============================================================================
    // OPERACIONES PRINCIPALES DEL DIRECTORIO
    // ============================================================================
    
    /**
     * @brief Obtiene bucket apropiado para una clave
     */
    std::shared_ptr<Bucket> getBucket(const std::string& key) {
        size_t hash_value = std::hash<std::string>{}(key);
        size_t index = hash_value & ((1 << global_depth) - 1); // Máscara con global_depth bits
        
        return directory[index];
    }
    
    /**
     * @brief Obtiene bucket por índice directo
     */
    std::shared_ptr<Bucket> getBucketByIndex(size_t index) {
        if (index < directory.size()) {
            return directory[index];
        }
        return nullptr;
    }
    
    /**
     * @brief Divide un bucket cuando está lleno
     */
    bool splitBucket(const std::string& key) {
        auto bucket = getBucket(key);
        
        if (!bucket || !bucket->isFull()) {
            return false;
        }
        
        int bucket_local_depth = bucket->getLocalDepth();
        
        std::cout << "🔄 INICIANDO SPLIT DE BUCKET:" << std::endl;
        std::cout << "   Profundidad local del bucket: " << bucket_local_depth << std::endl;
        std::cout << "   Profundidad global: " << global_depth << std::endl;
        
        // Si la profundidad local es igual a la global, necesitamos doblar el directorio
        if (bucket_local_depth == global_depth) {
            std::cout << "   📈 Doblando directorio..." << std::endl;
            doubleDirectory();
        }
        
        // Crear nuevo bucket
        auto new_bucket = std::make_shared<Bucket>(bucket_capacity, bucket_local_depth + 1);
        bucket->setLocalDepth(bucket_local_depth + 1);
        
        std::cout << "   🆕 Nuevo bucket creado con profundidad local: " << (bucket_local_depth + 1) << std::endl;
        
        // Redistribuir registros entre buckets
        redistributeRecords(bucket, new_bucket);
        
        // Actualizar punteros del directorio
        updateDirectoryPointers(bucket, new_bucket);
        
        std::cout << "   ✅ Split completado exitosamente" << std::endl;
        return true;
    }
    
    /**
     * @brief Dobla el tamaño del directorio
     */
    void doubleDirectory() {
        global_depth++;
        size_t new_size = 1 << global_depth;
        
        std::cout << "   📊 Directorio anterior: " << directory.size() << " entradas" << std::endl;
        std::cout << "   📊 Directorio nuevo: " << new_size << " entradas" << std::endl;
        
        // Duplicar las entradas existentes
        std::vector<std::shared_ptr<Bucket>> new_directory(new_size);
        
        for (size_t i = 0; i < directory.size(); i++) {
            new_directory[i] = directory[i];
            new_directory[i + directory.size()] = directory[i];
        }
        
        directory = std::move(new_directory);
        directory_size = new_size;
        
        std::cout << "   ✅ Directorio doblado: nueva profundidad global = " << global_depth << std::endl;
    }

    // ============================================================================
    // VISUALIZACIÓN Y ESTADÍSTICAS
    // ============================================================================
    
    /**
     * @brief Muestra la estructura completa del directorio
     */
    void display() const {
        std::cout << "\n📁 ESTRUCTURA DEL DIRECTORIO:" << std::endl;
        std::cout << "Profundidad global: " << global_depth << std::endl;
        std::cout << "Tamaño del directorio: " << directory.size() << std::endl;
        std::cout << "Capacidad por bucket: " << bucket_capacity << std::endl;
        
        std::cout << "\n📋 MAPEO DIRECTORIO → BUCKETS:" << std::endl;
        
        // Agrupar entradas que apuntan al mismo bucket
        std::map<Bucket*, std::vector<size_t>> bucket_map;
        
        for (size_t i = 0; i < directory.size(); i++) {
            bucket_map[directory[i].get()].push_back(i);
        }
        
        int bucket_id = 0;
        for (const auto& pair : bucket_map) {
            Bucket* bucket = pair.first;
            const auto& indices = pair.second;
            
            std::cout << "🪣 Bucket " << bucket_id << " (prof. local: " << bucket->getLocalDepth() 
                      << ", registros: " << bucket->getRecordCount() << "/" << bucket_capacity << ")" << std::endl;
            
            std::cout << "   Entradas del directorio: ";
            for (size_t i = 0; i < indices.size(); i++) {
                std::cout << indices[i];
                if (i < indices.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
            
            // Mostrar algunas claves del bucket
            auto keys = bucket->getSampleKeys(3);
            if (!keys.empty()) {
                std::cout << "   Claves ejemplo: ";
                for (size_t i = 0; i < keys.size(); i++) {
                    std::cout << keys[i].substr(0, 15) << "...";
                    if (i < keys.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }
            
            bucket_id++;
            std::cout << std::endl;
        }
    }
    
    /**
     * @brief Obtiene estadísticas del directorio
     */
    std::string getStatistics() const {
        std::stringstream ss;
        
        // Contar buckets únicos
        std::set<Bucket*> unique_buckets;
        for (const auto& bucket : directory) {
            unique_buckets.insert(bucket.get());
        }
        
        ss << "Directory Statistics:\n";
        ss << "  Global Depth: " << global_depth << "\n";
        ss << "  Directory Size: " << directory.size() << "\n";
        ss << "  Unique Buckets: " << unique_buckets.size() << "\n";
        ss << "  Bucket Capacity: " << bucket_capacity << "\n";
        
        // Calcular factor de carga
        int total_records = 0;
        for (Bucket* bucket : unique_buckets) {
            total_records += bucket->getRecordCount();
        }
        
        double load_factor = (double)total_records / (unique_buckets.size() * bucket_capacity);
        ss << "  Load Factor: " << std::fixed << std::setprecision(2) << load_factor << "\n";
        
        return ss.str();
    }

    // ============================================================================
    // GETTERS
    // ============================================================================
    
    int getGlobalDepth() const { return global_depth; }
    size_t getSize() const { return directory.size(); }
    int getBucketCapacity() const { return bucket_capacity; }
    
    /**
     * @brief Obtiene todos los buckets únicos
     */
    std::vector<std::shared_ptr<Bucket>> getUniqueBuckets() const {
        std::set<std::shared_ptr<Bucket>> unique_set;
        for (const auto& bucket : directory) {
            unique_set.insert(bucket);
        }
        
        return std::vector<std::shared_ptr<Bucket>>(unique_set.begin(), unique_set.end());
    }

private:
    // ============================================================================
    // MÉTODOS AUXILIARES PRIVADOS
    // ============================================================================
    
    /**
     * @brief Redistribuye registros entre bucket original y nuevo
     */
    void redistributeRecords(std::shared_ptr<Bucket> old_bucket, 
                            std::shared_ptr<Bucket> new_bucket) {
        
        std::cout << "   🔄 Redistribuyendo registros..." << std::endl;
        
        auto records = old_bucket->getAllRecords();
        old_bucket->clear();
        
        int records_in_old = 0;
        int records_in_new = 0;
        
        for (const auto& entry : records) {
            // Recalcular hash con nueva profundidad local
            size_t hash_value = std::hash<std::string>{}(entry.key);
            int new_local_depth = old_bucket->getLocalDepth();
            size_t mask = (1 << new_local_depth) - 1;
            size_t bucket_index = hash_value & mask;
            
            // El bit más significativo determina a cuál bucket va
            if (bucket_index & (1 << (new_local_depth - 1))) {
                new_bucket->insertRecord(entry.key, entry.record);
                records_in_new++;
            } else {
                old_bucket->insertRecord(entry.key, entry.record);
                records_in_old++;
            }
        }
        
        std::cout << "   📊 Distribución: Bucket original=" << records_in_old 
                  << ", Bucket nuevo=" << records_in_new << std::endl;
    }
    
    /**
     * @brief Actualiza punteros del directorio después del split
     */
    void updateDirectoryPointers(std::shared_ptr<Bucket> old_bucket, 
                                std::shared_ptr<Bucket> new_bucket) {
        
        std::cout << "   🔗 Actualizando punteros del directorio..." << std::endl;
        
        int local_depth = old_bucket->getLocalDepth();
        size_t step = 1 << local_depth;
        
        for (size_t i = 0; i < directory.size(); i += step) {
            if (directory[i] == old_bucket) {
                // Determinar si debe apuntar al bucket original o al nuevo
                if (i & (1 << (local_depth - 1))) {
                    directory[i] = new_bucket;
                }
                // Las entradas que no tienen el bit set siguen apuntando al bucket original
            }
        }
        
        std::cout << "   ✅ Punteros actualizados" << std::endl;
    }
};

#endif // DIRECTORY_H