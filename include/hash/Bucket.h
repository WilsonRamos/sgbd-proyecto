#ifndef BUCKET_H
#define BUCKET_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <type_traits>

/**
 * @brief Bucket para almacenar pares clave-valor en Hash Extensible
 */
template<typename K, typename V>
class Bucket {
private:
    std::vector<std::pair<K, V>> data;
    int local_depth;
    int max_capacity;
    
public:
    Bucket(int capacity = 4, int depth = 0) 
        : local_depth(depth), max_capacity(capacity) {}
    
    // Getters y setters
    int getLocalDepth() const { return local_depth; }
    void setLocalDepth(int depth) { local_depth = depth; }
    size_t size() const { return data.size(); }
    bool isFull() const { return data.size() >= static_cast<size_t>(max_capacity); }
    bool isEmpty() const { return data.empty(); }
    int getCapacity() const { return max_capacity; }
    
    // Operaciones básicas
    bool insert(const K& key, const V& value);
    bool remove(const K& key);
    bool find(const K& key, V& value) const;
    std::vector<std::pair<K, V>> getAllPairs() const { return data; }
    
    // Para debugging
    void display() const;
    
    template<typename Key, typename Value>
    friend class ExtendibleHash;
};

// =================================================================
// IMPLEMENTACIÓN DE BUCKET
// =================================================================

template<typename K, typename V>
bool Bucket<K, V>::insert(const K& key, const V& value) {
    // Verificar si la clave ya existe
    for (auto& pair : data) {
        if (pair.first == key) {
            pair.second = value; // Actualizar valor existente
            return true;
        }
    }
    
    // Verificar si hay espacio
    if (isFull()) {
        return false;
    }
    
    // Insertar nuevo elemento
    data.emplace_back(key, value);
    return true;
}

template<typename K, typename V>
bool Bucket<K, V>::remove(const K& key) {
    auto it = std::find_if(data.begin(), data.end(),
        [&key](const std::pair<K, V>& pair) { return pair.first == key; });
    
    if (it != data.end()) {
        data.erase(it);
        return true;
    }
    return false;
}

template<typename K, typename V>
bool Bucket<K, V>::find(const K& key, V& value) const {
    for (const auto& pair : data) {
        if (pair.first == key) {
            value = pair.second;
            return true;
        }
    }
    return false;
}

template<typename K, typename V>
void Bucket<K, V>::display() const {
    std::cout << "[LD:" << local_depth << ", " << data.size() 
              << "/" << max_capacity << "] ";
    for (const auto& pair : data) {
        std::cout << "(" << pair.first << ":";
        
        // Manejar diferentes tipos de V para impresión
        if constexpr (std::is_same_v<V, std::vector<int>>) {
            std::cout << "[";
            for (size_t i = 0; i < pair.second.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << pair.second[i];
                if (i >= 2) { // Mostrar solo los primeros 3 elementos
                    std::cout << "...(" << pair.second.size() << " total)";
                    break;
                }
            }
            std::cout << "]";
        } else {
            std::cout << pair.second;
        }
        std::cout << ") ";
    }
}

#endif // BUCKET_H
