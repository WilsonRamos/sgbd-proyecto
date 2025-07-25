#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <vector>
#include <memory>
#include <iostream>
#include "Bucket.h"
#include "HashFunction.h"

class Directory {
private:
    int global_depth;
    std::vector<std::shared_ptr<Bucket>> buckets;

public:
    Directory(int initial_capacity = 4) : global_depth(0) {
        // Inicializar con un bucket
        auto initial_bucket = std::make_shared<Bucket>(initial_capacity);
        buckets.push_back(initial_bucket);
    }
    
    // Getters
    int getGlobalDepth() const { return global_depth; }
    int getSize() const { return buckets.size(); }
    
    // Obtener bucket para una clave
    std::shared_ptr<Bucket> getBucket(const std::string& key) {
        uint32_t hash_val = HashFunction::hashString(key);
        uint32_t bucket_index = HashFunction::getBits(hash_val, global_depth);
        
        if (bucket_index >= buckets.size()) {
            bucket_index = 0; // Fallback de seguridad
        }
        
        return buckets[bucket_index];
    }
    
    // Duplicar directorio cuando se necesita más profundidad
    void doubleDirectory() {
        int old_size = buckets.size();
        buckets.resize(old_size * 2);
        
        // Copiar punteros existentes
        for (int i = 0; i < old_size; i++) {
            buckets[old_size + i] = buckets[i];
        }
        
        global_depth++;
        std::cout << "📁 Directorio duplicado. Nueva profundidad global: " << global_depth << std::endl;
    }
    
    // Dividir bucket y actualizar directorio
    bool splitBucket(const std::string& key) {
        uint32_t hash_val = HashFunction::hashString(key);
        uint32_t bucket_index = HashFunction::getBits(hash_val, global_depth);
        
        auto bucket = buckets[bucket_index];
        
        // Si la profundidad local es igual a la global, duplicar directorio
        if (bucket->getLocalDepth() == global_depth) {
            doubleDirectory();
        }
        
        // Incrementar profundidad local y dividir
        bucket->incrementDepth();
        auto new_bucket = bucket->split(bucket->getLocalDepth() - 1);
        new_bucket->setLocalDepth(bucket->getLocalDepth());
        
        // Actualizar punteros en el directorio
        updateDirectoryPointers(bucket, new_bucket);
        
        return true;
    }
    
private:
    void updateDirectoryPointers(std::shared_ptr<Bucket> old_bucket, 
                                std::shared_ptr<Bucket> new_bucket) {
        int local_depth = old_bucket->getLocalDepth();
        //int step = 1 << local_depth;
        
        for (size_t i = 0; i < buckets.size(); i++) {
            if (buckets[i] == old_bucket) {
                // Determinar si este índice debería apuntar al nuevo bucket
                if ((i >> (local_depth - 1)) & 1) {
                    buckets[i] = new_bucket;
                }
            }
        }
    }

public:
    void display() const {
        std::cout << "\n=== DIRECTORIO HASH EXTENDIBLE ===" << std::endl;
        std::cout << "Profundidad Global: " << global_depth << std::endl;
        std::cout << "Número de entradas: " << buckets.size() << std::endl;
        
        for (size_t i = 0; i < buckets.size(); i++) {
            std::cout << "Dir[" << i << "] -> ";
            buckets[i]->display();
        }
        std::cout << "================================" << std::endl;
    }
};

#endif