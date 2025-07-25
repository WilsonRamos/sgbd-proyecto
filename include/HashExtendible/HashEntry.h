#ifndef HASH_ENTRY_H
#define HASH_ENTRY_H

#include <string>
#include <memory>
#include "../Record.h"

/**
 * @brief Entrada en bucket de Hash Extensible
 */
struct HashEntry {
    std::string key;
    std::unique_ptr<Record> record;
    
    HashEntry(const std::string& k, std::unique_ptr<Record> r) 
        : key(k), record(std::move(r)) {}
    
    // Constructor de copia para manipulación de buckets
    HashEntry(const HashEntry& other) 
        : key(other.key), record(other.record->clone()) {}
    
    // Operador de asignación
    HashEntry& operator=(const HashEntry& other) {
        if (this != &other) {
            key = other.key;
            record = other.record->clone();
        }
        return *this;
    }
    
    // Constructor de movimiento
    HashEntry(HashEntry&& other) noexcept 
        : key(std::move(other.key)), record(std::move(other.record)) {}
    
    // Operador de asignación de movimiento
    HashEntry& operator=(HashEntry&& other) noexcept {
        if (this != &other) {
            key = std::move(other.key);
            record = std::move(other.record);
        }
        return *this;
    }
};

#endif