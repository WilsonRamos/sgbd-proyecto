#ifndef EXTENDIBLE_HASH_H
#define EXTENDIBLE_HASH_H

#include "hash/Bucket.h"
#include "hash/HashConfig.h"
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <type_traits>

/**
 * @brief Implementación de Hashing Extensible modularizada
 * 
 * Características principales:
 * - Global Depth: Bits necesarios para indexar el directorio
 * - Local Depth: Bits usados para determinar pertenencia a bucket
 * - Split dinámico de buckets cuando se llenan
 * - Directorio que puede duplicarse cuando es necesario
 */
template<typename K, typename V>
class ExtendibleHash {
private:
    std::vector<std::shared_ptr<Bucket<K, V>>> directory;
    int global_depth;
    int bucket_capacity;
    HashConfig config;
    
    // Estadísticas
    mutable int total_insertions;
    mutable int total_splits;
    mutable int directory_expansions;
    
    // Funciones auxiliares
    int hash(const K& key) const;
    int getDirectoryIndex(const K& key) const;
    std::shared_ptr<Bucket<K, V>> splitBucket(std::shared_ptr<Bucket<K, V>> bucket);
    void doubleDirectory();
    bool needToDoubleDirectory(int local_depth) const;
    void redistributeBucketData(std::shared_ptr<Bucket<K, V>> old_bucket, 
                                std::shared_ptr<Bucket<K, V>> new_bucket);
    
public:
    /**
     * @brief Constructor
     */
    explicit ExtendibleHash(const HashConfig& cfg = HashConfig());
    
    /**
     * @brief Destructor
     */
    ~ExtendibleHash() = default;
    
    // Operaciones principales
    bool insert(const K& key, const V& value);
    bool remove(const K& key);
    bool find(const K& key, V& value) const;
    
    // Información del estado
    int getGlobalDepth() const { return global_depth; }
    int getDirectorySize() const { return directory.size(); }
    int getTotalElements() const;
    int getNumberOfBuckets() const;
    double getLoadFactor() const;
    
    // Estadísticas
    int getTotalInsertions() const { return total_insertions; }
    int getTotalSplits() const { return total_splits; }
    int getDirectoryExpansions() const { return directory_expansions; }
    
    // Visualización y debugging
    void displayStructure() const;
    void displayStatistics() const;
    void displayDirectory() const;
    
    // Para testing
    std::vector<std::pair<K, V>> getAllElements() const;
};

// =================================================================
// IMPLEMENTACIÓN DE EXTENDIBLE HASH
// =================================================================

template<typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(const HashConfig& cfg) 
    : global_depth(cfg.initial_global_depth), bucket_capacity(cfg.bucket_capacity), 
      config(cfg), total_insertions(0), total_splits(0), directory_expansions(0) {
    
    // Crear directorio inicial
    int initial_size = 1 << global_depth;
    if (initial_size == 0) initial_size = 1; // Para depth = 0
    
    auto initial_bucket = std::make_shared<Bucket<K, V>>(bucket_capacity, global_depth);
    
    for (int i = 0; i < initial_size; ++i) {
        directory.push_back(initial_bucket);
    }
}

template<typename K, typename V>
int ExtendibleHash<K, V>::hash(const K& key) const {
    // Función hash simple para diferentes tipos
    if constexpr (std::is_integral_v<K>) {
        return static_cast<int>(key);
    } else if constexpr (std::is_same_v<K, std::string>) {
        std::hash<std::string> hasher;
        return static_cast<int>(hasher(key));
    } else {
        // Para otros tipos, usar std::hash por defecto
        std::hash<K> hasher;
        return static_cast<int>(hasher(key));
    }
}

template<typename K, typename V>
int ExtendibleHash<K, V>::getDirectoryIndex(const K& key) const {
    int hash_value = hash(key);
    
    // Usar solo los últimos global_depth bits
    int mask = (1 << global_depth) - 1;
    return hash_value & mask;
}

template<typename K, typename V>
bool ExtendibleHash<K, V>::needToDoubleDirectory(int local_depth) const {
    return local_depth >= global_depth;
}

template<typename K, typename V>
void ExtendibleHash<K, V>::doubleDirectory() {
    int old_size = directory.size();
    directory.resize(old_size * 2);
    
    // Copiar punteros existentes
    for (int i = 0; i < old_size; ++i) {
        directory[old_size + i] = directory[i];
    }
    
    global_depth++;
    directory_expansions++;
    
    if (config.enable_statistics) {
        std::cout << "📂 Directorio duplicado. Global Depth: " << global_depth 
                  << ", Tamaño: " << directory.size() << std::endl;
    }
}

template<typename K, typename V>
void ExtendibleHash<K, V>::redistributeBucketData(
    std::shared_ptr<Bucket<K, V>> old_bucket, 
    std::shared_ptr<Bucket<K, V>> new_bucket) {
    
    auto all_data = old_bucket->data;
    old_bucket->data.clear();
    
    // Redistribuir datos basado en el nuevo local depth
    int new_local_depth = old_bucket->getLocalDepth();
    
    for (const auto& pair : all_data) {
        int index = getDirectoryIndex(pair.first);
        int bit_position = 1 << (new_local_depth - 1);
        
        if (index & bit_position) {
            new_bucket->data.push_back(pair);
        } else {
            old_bucket->data.push_back(pair);
        }
    }
}

template<typename K, typename V>
std::shared_ptr<Bucket<K, V>> ExtendibleHash<K, V>::splitBucket(
    std::shared_ptr<Bucket<K, V>> bucket) {
    
    // Crear nuevo bucket
    auto new_bucket = std::make_shared<Bucket<K, V>>(bucket_capacity, 
                                                     bucket->getLocalDepth() + 1);
    
    // Incrementar local depth del bucket original
    bucket->setLocalDepth(bucket->getLocalDepth() + 1);
    
    // Redistribuir datos
    redistributeBucketData(bucket, new_bucket);
    
    // Actualizar directorio
    int old_local_depth = bucket->getLocalDepth() - 1;
    int step = 1 << old_local_depth;
    
    for (size_t i = 0; i < directory.size(); i += step) {
        if (directory[i] == bucket) {
            int bit_check = 1 << old_local_depth;
            if (static_cast<int>(i) & bit_check) {
                // Actualizar punteros para el nuevo bucket
                for (int j = 0; j < (1 << (global_depth - bucket->getLocalDepth())); ++j) {
                    size_t index = i + j * (1 << bucket->getLocalDepth());
                    if (index < directory.size()) {
                        directory[index] = new_bucket;
                    }
                }
            }
        }
    }
    
    total_splits++;
    return new_bucket;
}

template<typename K, typename V>
bool ExtendibleHash<K, V>::insert(const K& key, const V& value) {
    total_insertions++;
    
    int index = getDirectoryIndex(key);
    auto bucket = directory[index];
    
    // Intentar insertar en el bucket
    if (bucket->insert(key, value)) {
        return true;
    }
    
    // El bucket está lleno, necesita split
    if (config.enable_statistics) {
        std::cout << "🔄 Bucket lleno, iniciando split..." << std::endl;
    }
    
    // Verificar si necesitamos duplicar el directorio
    if (needToDoubleDirectory(bucket->getLocalDepth())) {
        doubleDirectory();
    }
    
    // Split del bucket
    auto new_bucket = splitBucket(bucket);
    
    if (config.enable_statistics) {
        std::cout << "✅ Split completado. Buckets creados con Local Depth: " 
                  << bucket->getLocalDepth() << std::endl;
    }
    
    // Intentar insertar nuevamente
    index = getDirectoryIndex(key);
    return directory[index]->insert(key, value);
}

template<typename K, typename V>
bool ExtendibleHash<K, V>::remove(const K& key) {
    int index = getDirectoryIndex(key);
    return directory[index]->remove(key);
}

template<typename K, typename V>
bool ExtendibleHash<K, V>::find(const K& key, V& value) const {
    int index = getDirectoryIndex(key);
    return directory[index]->find(key, value);
}

template<typename K, typename V>
int ExtendibleHash<K, V>::getTotalElements() const {
    int total = 0;
    std::unordered_map<Bucket<K, V>*, bool> counted;
    
    for (const auto& bucket_ptr : directory) {
        if (counted.find(bucket_ptr.get()) == counted.end()) {
            total += static_cast<int>(bucket_ptr->size());
            counted[bucket_ptr.get()] = true;
        }
    }
    return total;
}

template<typename K, typename V>
int ExtendibleHash<K, V>::getNumberOfBuckets() const {
    std::unordered_map<Bucket<K, V>*, bool> unique_buckets;
    
    for (const auto& bucket_ptr : directory) {
        unique_buckets[bucket_ptr.get()] = true;
    }
    return unique_buckets.size();
}

template<typename K, typename V>
double ExtendibleHash<K, V>::getLoadFactor() const {
    int total_capacity = getNumberOfBuckets() * bucket_capacity;
    return total_capacity > 0 ? static_cast<double>(getTotalElements()) / total_capacity : 0.0;
}

template<typename K, typename V>
void ExtendibleHash<K, V>::displayStructure() const {
    std::cout << "\n=== ESTRUCTURA DEL HASH EXTENSIBLE ===" << std::endl;
    std::cout << "Global Depth: " << global_depth << std::endl;
    std::cout << "Tamaño del Directorio: " << directory.size() << std::endl;
    std::cout << "Número de Buckets: " << getNumberOfBuckets() << std::endl;
    std::cout << "Total de Elementos: " << getTotalElements() << std::endl;
    std::cout << "Factor de Carga: " << std::fixed << std::setprecision(2) 
              << getLoadFactor() * 100 << "%" << std::endl;
    
    std::cout << "\n--- DIRECTORIO ---" << std::endl;
    for (size_t i = 0; i < directory.size(); ++i) {
        std::cout << "Dir[" << std::setw(3) << i << "] -> Bucket ";
        directory[i]->display();
        std::cout << std::endl;
    }
}

template<typename K, typename V>
void ExtendibleHash<K, V>::displayStatistics() const {
    std::cout << "\n=== ESTADÍSTICAS DE RENDIMIENTO ===" << std::endl;
    std::cout << "Inserciones totales: " << total_insertions << std::endl;
    std::cout << "Splits realizados: " << total_splits << std::endl;
    std::cout << "Expansiones de directorio: " << directory_expansions << std::endl;
    
    if (total_insertions > 0) {
        std::cout << "Splits por inserción: " << std::fixed << std::setprecision(3)
                  << static_cast<double>(total_splits) / total_insertions << std::endl;
    }
    
    // Estadísticas por bucket único
    std::unordered_map<Bucket<K, V>*, int> bucket_usage;
    std::unordered_map<Bucket<K, V>*, int> bucket_references;
    
    for (const auto& bucket_ptr : directory) {
        bucket_usage[bucket_ptr.get()] = static_cast<int>(bucket_ptr->size());
        bucket_references[bucket_ptr.get()]++;
    }
    
    std::cout << "\nBuckets únicos: " << bucket_usage.size() << std::endl;
    std::cout << "Referencias totales en directorio: " << directory.size() << std::endl;
    
    int bucket_id = 0;
    for (const auto& pair : bucket_usage) {
        auto* bucket_ptr = pair.first;
        int usage = pair.second;
        double bucket_load = static_cast<double>(usage) / bucket_capacity * 100;
        std::cout << "Bucket " << bucket_id++ << ": " 
                  << usage << "/" << bucket_capacity 
                  << " (" << std::fixed << std::setprecision(1) << bucket_load << "%) "
                  << "Local Depth: " << bucket_ptr->getLocalDepth()
                  << ", Referencias: " << bucket_references[bucket_ptr] << std::endl;
    }
}

template<typename K, typename V>
std::vector<std::pair<K, V>> ExtendibleHash<K, V>::getAllElements() const {
    std::vector<std::pair<K, V>> all_elements;
    std::unordered_map<Bucket<K, V>*, bool> processed;
    
    for (const auto& bucket_ptr : directory) {
        if (processed.find(bucket_ptr.get()) == processed.end()) {
            auto bucket_data = bucket_ptr->getAllPairs();
            all_elements.insert(all_elements.end(), bucket_data.begin(), bucket_data.end());
            processed[bucket_ptr.get()] = true;
        }
    }
    return all_elements;
}

#endif // EXTENDIBLE_HASH_H