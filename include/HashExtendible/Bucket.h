#ifndef BUCKET_H
#define BUCKET_H

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include "../Record.h"
#include "HashFunction.h"

struct HashEntry {
    std::string key;
    std::unique_ptr<Record> record;
    
    HashEntry(const std::string& k, std::unique_ptr<Record> r) 
        : key(k), record(std::move(r)) {}
    
    // Constructor de copia para hacer copias profundas
    HashEntry(const HashEntry& other) 
        : key(other.key), record(other.record->clone()) {}
    
    // Operador de asignación para copias profundas
    HashEntry& operator=(const HashEntry& other) {
        if (this != &other) {
            key = other.key;
            record = other.record->clone();
        }
        return *this;
    }
    
    // Constructor de movimiento (por defecto está bien)
    HashEntry(HashEntry&& other) noexcept = default;
    HashEntry& operator=(HashEntry&& other) noexcept = default;
};

class Bucket {
private:
    int local_depth;
    int max_capacity;
    std::vector<HashEntry> entries;

public:
    Bucket(int capacity = 4) : local_depth(0), max_capacity(capacity) {}
    
    // Getters
    int getLocalDepth() const { return local_depth; }
    int getSize() const { return entries.size(); }
    int getCapacity() const { return max_capacity; }
    bool isFull() const { return entries.size() >= static_cast<size_t>(max_capacity); }
    bool isEmpty() const { return entries.empty(); }
    
    // Setters
    void setLocalDepth(int depth) { local_depth = depth; }
    void incrementDepth() { local_depth++; }
    
    // Operaciones principales
    bool insert(const std::string& key, std::unique_ptr<Record> record) {
        if (isFull()) return false;
        
        // Verificar si la clave ya existe
        for (auto& entry : entries) {
            if (entry.key == key) {
                entry.record = std::move(record); // Actualizar
                return true;
            }
        }
        
        entries.emplace_back(key, std::move(record));
        return true;
    }
    
    bool search(const std::string& key, Record& record) const {
        for (const auto& entry : entries) {
            if (entry.key == key) {
                record = *entry.record;
                return true;
            }
        }
        return false;
    }
    
    bool remove(const std::string& key) {
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->key == key) {
                entries.erase(it);
                return true;
            }
        }
        return false;
    }
    
    // Para división de bucket
    std::shared_ptr<Bucket> split(int split_bit_position) {
        auto new_bucket = std::make_shared<Bucket>(max_capacity);
        new_bucket->setLocalDepth(local_depth);
        
        auto it = entries.begin();
        while (it != entries.end()) {
            uint32_t hash_val = HashFunction::hashString(it->key);
            bool move_to_new = (hash_val >> split_bit_position) & 1;
            
            if (move_to_new) {
                new_bucket->entries.push_back(std::move(*it));
                it = entries.erase(it);
            } else {
                ++it;
            }
        }
        
        return new_bucket;
    }
    
    // Debug
    void display() const {
        std::cout << "Bucket (depth=" << local_depth << ", size=" << entries.size() << "): ";
        for (const auto& entry : entries) {
            std::cout << "[" << entry.key << "] ";
        }
        std::cout << std::endl;
    }
    
    std::vector<HashEntry>& getEntries() { return entries; }
    const std::vector<HashEntry>& getEntries() const { return entries; }
};

#endif