#ifndef EXTENSIBLE_HASH_H
#define EXTENSIBLE_HASH_H

#include <string>
#include <memory>
#include <iostream>
#include "Directory.h"
#include "Bucket.h"
#include "HashFunction.h"
#include "../Record.h"

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
    ExtensibleHash(int capacity = 4) 
        : bucket_capacity(capacity)
        , total_records(0)
        , insert_operations(0)
        , search_operations(0)
        , split_operations(0) {
        
        directory = std::make_unique<Directory>(capacity);
        std::cout << "🔗 Hash Extendible inicializado (capacidad: " << capacity << ")" << std::endl;
    }
    
    // Insertar registro
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
            std::cout << "🔄 Dividiendo bucket para clave: " << key << std::endl;
            
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
    
    // Buscar registro
    bool search(const std::string& key, Record& record) {
        search_operations++;
        
        auto bucket = directory->getBucket(key);
        return bucket->search(key, record);
    }
    
    // Eliminar registro
    bool remove(const std::string& key) {
        auto bucket = directory->getBucket(key);
        if (bucket->remove(key)) {
            total_records--;
            return true;
        }
        return false;
    }
    
    // Búsqueda por rango (para demonstration - no es eficiente en hash)
    std::vector<std::unique_ptr<Record>> rangeSearch(const std::string& start_key, const std::string& end_key) {
        std::cout << "⚠️ Advertencia: Búsqueda por rango no es eficiente en Hash Extendible" << std::endl;
        std::vector<std::unique_ptr<Record>> results;
        
        // Iterar todos los buckets (ineficiente, pero funcional)
        for (int i = 0; i < directory->getSize(); i++) {
            auto bucket = directory->getBucket(std::to_string(i));
            for (const auto& entry : bucket->getEntries()) {
                if (entry.key >= start_key && entry.key <= end_key) {
                    results.push_back(entry.record->clone());
                }
            }
        }
        
        return results;
    }
    
    // Estadísticas
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
    }
    
    // Debug - mostrar estructura completa
    void displayStructure() const {
        directory->display();
    }
    
    // Información del índice
    size_t getTotalRecords() const { return total_records; }
    size_t getSplitOperations() const { return split_operations; }
    int getGlobalDepth() const { return directory->getGlobalDepth(); }
};

#endif