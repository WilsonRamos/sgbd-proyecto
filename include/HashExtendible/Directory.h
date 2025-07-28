#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include <set>          
#include <sstream>      
#include <iomanip>      
#include <map>
#include <functional>
#include <algorithm>
#include "Bucket.h"
#include "HashFunction.h"

/**
 * @brief Directorio para Hash Extensible ACTUALIZADO
 * 
 * ✅ MEJORAS IMPLEMENTADAS:
 * - Gestión robusta de splits y expansiones
 * - Validación de consistencia completa
 * - Serialización/deserialización mejorada
 * - Análisis educativo detallado
 * - Optimizaciones de memoria
 * - Soporte para debugging avanzado
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
        size_t hash_value = HashFunction::hash(key);
        size_t index = hash_value & ((1 << global_depth) - 1); // Máscara con global_depth bits
        
        if (index >= directory.size()) {
            std::cout << "❌ Índice fuera de rango: " << index << " >= " << directory.size() << std::endl;
            return nullptr;
        }
        
        return directory[index];
    }
    
    /**
     * @brief ✅ Obtiene bucket por índice
     */
    std::shared_ptr<Bucket> getBucketByIndex(size_t index) {
        if (index >= directory.size()) {
            return nullptr;
        }
        return directory[index];
    }

    /**
     * @brief ✅ Expande el directorio (duplica el tamaño)
     */
    bool expand() {
        if (global_depth >= 20) { // Límite de seguridad
            std::cout << "❌ Límite de profundidad alcanzado: " << global_depth << std::endl;
            return false;
        }

        std::cout << "📈 Expandiendo directorio: depth " << global_depth 
                  << " -> " << (global_depth + 1) << std::endl;

        size_t old_size = directory_size;
        global_depth++;
        directory_size = 1 << global_depth;

        // Duplicar directorio
        directory.resize(directory_size);
        for (size_t i = 0; i < old_size; i++) {
            directory[i + old_size] = directory[i];
        }

        std::cout << "✅ Directorio expandido: " << old_size << " -> " << directory_size << " entradas" << std::endl;
        return true;
    }

    /**
     * @brief ✅ Actualiza directorio después de split de bucket
     */
    bool updateAfterSplit(std::shared_ptr<Bucket> old_bucket, std::shared_ptr<Bucket> new_bucket) {
        if (!old_bucket || !new_bucket) {
            std::cout << "❌ Buckets inválidos para update" << std::endl;
            return false;
        }

        int local_depth = old_bucket->getLocalDepth();
        if (local_depth != new_bucket->getLocalDepth()) {
            std::cout << "❌ Profundidades locales inconsistentes" << std::endl;
            return false;
        }

        // Calcular máscara para redistribución
        size_t bit_mask = 1ULL << (local_depth - 1);
        
        std::cout << "🔄 Actualizando directorio para depth " << local_depth 
                  << " (mask: 0x" << std::hex << bit_mask << std::dec << ")" << std::endl;

        // Redistribuir entradas del directorio
        for (size_t i = 0; i < directory_size; i++) {
            if (directory[i] == old_bucket) {
                // Determinar si esta entrada debe apuntar al nuevo bucket
                if (i & bit_mask) {
                    directory[i] = new_bucket;
                    std::cout << "   Entrada " << i << " -> nuevo bucket" << std::endl;
                } else {
                    std::cout << "   Entrada " << i << " -> bucket original" << std::endl;
                }
            }
        }

        return true;
    }

    // ============================================================================
    // ACCESO A INFORMACIÓN
    // ============================================================================
    
    int getGlobalDepth() const { return global_depth; }
    size_t getSize() const { return directory_size; }
    int getBucketCapacity() const { return bucket_capacity; }

    /**
     * @brief ✅ Obtiene buckets únicos (sin duplicados)
     */
    std::vector<std::shared_ptr<Bucket>> getUniqueBuckets() const {
        std::set<std::shared_ptr<Bucket>> unique_set;
        
        for (const auto& bucket : directory) {
            unique_set.insert(bucket);
        }
        
        return std::vector<std::shared_ptr<Bucket>>(unique_set.begin(), unique_set.end());
    }

    /**
     * @brief Cuenta el número de buckets únicos
     */
    size_t getUniqueBucketCount() const {
        return getUniqueBuckets().size();
    }

    /**
     * @brief Obtiene estadísticas de distribución
     */
    std::map<std::shared_ptr<Bucket>, std::vector<size_t>> getBucketIndices() const {
        std::map<std::shared_ptr<Bucket>, std::vector<size_t>> bucket_indices;
        
        for (size_t i = 0; i < directory.size(); i++) {
            bucket_indices[directory[i]].push_back(i);
        }
        
        return bucket_indices;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra el directorio completo (educativo)
     */
    void display() const {
        std::cout << "\n📁 DIRECTORIO HASH EXTENSIBLE:" << std::endl;
        std::cout << "===============================" << std::endl;
        std::cout << "Profundidad global: " << global_depth << std::endl;
        std::cout << "Tamaño directorio: " << directory_size << std::endl;
        std::cout << "Buckets únicos: " << getUniqueBucketCount() << std::endl;
        
        // Mostrar mapeo entrada -> bucket
        auto bucket_indices = getBucketIndices();
        int bucket_id = 0;
        
        std::cout << "\n🗂️ MAPEO DIRECTORIO -> BUCKETS:" << std::endl;
        std::cout << "Entrada | Binario    | Bucket | Local Depth | Entradas" << std::endl;
        std::cout << "--------|------------|--------|-------------|----------" << std::endl;
        
        for (size_t i = 0; i < directory_size; i++) {
            auto bucket = directory[i];
            std::string binary = HashFunction::toBinaryString(i, global_depth);
            
            // Encontrar ID del bucket
            int current_bucket_id = -1;
            int id = 0;
            for (const auto& pair : bucket_indices) {
                if (pair.first == bucket) {
                    current_bucket_id = id;
                    break;
                }
                id++;
            }
            
            std::cout << std::setw(7) << i << " | ";
            std::cout << std::setw(10) << binary << " | ";
            std::cout << std::setw(6) << current_bucket_id << " | ";
            std::cout << std::setw(11) << bucket->getLocalDepth() << " | ";
            std::cout << std::setw(8) << bucket->size() << std::endl;
        }
        
        // Mostrar buckets detallados
        std::cout << "\n📦 BUCKETS DETALLADOS:" << std::endl;
        bucket_id = 0;
        for (const auto& pair : bucket_indices) {
            std::cout << "\n🔹 Bucket " << bucket_id << " (Local Depth: " 
                      << pair.first->getLocalDepth() << "):" << std::endl;
            std::cout << "   Entradas directorio: ";
            for (size_t idx : pair.second) {
                std::cout << idx << " ";
            }
            std::cout << std::endl;
            
            pair.first->display();
            bucket_id++;
        }
    }

    /**
     * @brief Muestra solo la estructura del directorio (compacto)
     */
    void displayCompact() const {
        std::cout << "📁 Directory[depth=" << global_depth << ", size=" << directory_size 
                  << ", unique_buckets=" << getUniqueBucketCount() << "]" << std::endl;
        
        // Mostrar pattern de buckets
        std::cout << "Pattern: ";
        auto bucket_indices = getBucketIndices();
        std::map<std::shared_ptr<Bucket>, char> bucket_chars;
        char current_char = 'A';
        
        for (const auto& pair : bucket_indices) {
            bucket_chars[pair.first] = current_char++;
        }
        
        for (size_t i = 0; i < std::min(directory_size, size_t(32)); i++) {
            std::cout << bucket_chars[directory[i]];
        }
        if (directory_size > 32) {
            std::cout << "...";
        }
        std::cout << std::endl;
    }

    // ============================================================================
    // SERIALIZACIÓN PARA PERSISTENCIA
    // ============================================================================
    
    /**
     * @brief Serializa el directorio completo
     */
    std::string serialize() const {
        std::ostringstream oss;
        
        oss << "DIRECTORY_V1" << std::endl;
        oss << "global_depth=" << global_depth << std::endl;
        oss << "bucket_capacity=" << bucket_capacity << std::endl;
        oss << "directory_size=" << directory_size << std::endl;
        
        // Serializar buckets únicos primero
        auto unique_buckets = getUniqueBuckets();
        oss << "unique_buckets=" << unique_buckets.size() << std::endl;
        
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            oss << "BUCKET_" << i << "_START" << std::endl;
            oss << unique_buckets[i]->serialize();
            oss << "BUCKET_" << i << "_END" << std::endl;
        }
        
        // Mapear directorio a buckets únicos
        oss << "DIRECTORY_MAPPING" << std::endl;
        for (size_t i = 0; i < directory_size; i++) {
            // Encontrar índice del bucket único
            auto it = std::find(unique_buckets.begin(), unique_buckets.end(), directory[i]);
            size_t bucket_index = std::distance(unique_buckets.begin(), it);
            oss << i << "=" << bucket_index << std::endl;
        }
        oss << "END_DIRECTORY_MAPPING" << std::endl;
        
        return oss.str();
    }

    /**
     * @brief Deserializa directorio desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string line;
        
        // Verificar formato
        std::getline(iss, line);
        if (line != "DIRECTORY_V1") {
            std::cout << "❌ Formato de directorio inválido" << std::endl;
            return false;
        }
        
        // Leer metadatos
        std::map<std::string, std::string> metadata;
        while (std::getline(iss, line) && line.find("unique_buckets=") != 0) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                metadata[line.substr(0, pos)] = line.substr(pos + 1);
            }
        }
        
        // Aplicar metadatos
        global_depth = std::stoi(metadata["global_depth"]);
        bucket_capacity = std::stoi(metadata["bucket_capacity"]);
        directory_size = std::stoull(metadata["directory_size"]);
        
        // Leer número de buckets únicos
        size_t unique_count = std::stoull(line.substr(15)); // "unique_buckets=".length()
        
        std::vector<std::shared_ptr<Bucket>> unique_buckets(unique_count);
        
        // Deserializar buckets únicos
        for (size_t i = 0; i < unique_count; i++) {
            std::string start_marker = "BUCKET_" + std::to_string(i) + "_START";
            std::string end_marker = "BUCKET_" + std::to_string(i) + "_END";
            
            // Buscar inicio del bucket
            while (std::getline(iss, line) && line != start_marker) {}
            
            // Leer contenido del bucket
            std::stringstream bucket_content;
            while (std::getline(iss, line) && line != end_marker) {
                bucket_content << line << std::endl;
            }
            
            // Crear y deserializar bucket
            unique_buckets[i] = std::make_shared<Bucket>(bucket_capacity, 0);
            if (!unique_buckets[i]->deserialize(bucket_content.str())) {
                std::cout << "❌ Error deserializando bucket " << i << std::endl;
                return false;
            }
        }
        
        // Leer mapeo del directorio
        while (std::getline(iss, line) && line != "DIRECTORY_MAPPING") {}
        
        directory.resize(directory_size);
        while (std::getline(iss, line) && line != "END_DIRECTORY_MAPPING") {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                size_t dir_index = std::stoull(line.substr(0, pos));
                size_t bucket_index = std::stoull(line.substr(pos + 1));
                
                if (dir_index < directory_size && bucket_index < unique_count) {
                    directory[dir_index] = unique_buckets[bucket_index];
                }
            }
        }
        
        std::cout << "✅ Directorio deserializado: " << unique_count << " buckets únicos" << std::endl;
        return true;
    }

    // ============================================================================
    // VALIDACIÓN Y ANÁLISIS
    // ============================================================================
    
    /**
     * @brief Valida consistencia del directorio
     */
    bool validateConsistency() const {
        std::cout << "🔍 Validando consistencia del directorio..." << std::endl;
        
        bool is_consistent = true;
        
        // Verificar tamaño
        if (directory.size() != directory_size) {
            std::cout << "❌ Tamaño inconsistente: " << directory.size() 
                      << " != " << directory_size << std::endl;
            is_consistent = false;
        }
        
        if (directory_size != (1ULL << global_depth)) {
            std::cout << "❌ Tamaño no coincide con profundidad: " 
                      << directory_size << " != 2^" << global_depth << std::endl;
            is_consistent = false;
        }
        
        // Verificar buckets válidos
        for (size_t i = 0; i < directory.size(); i++) {
            if (!directory[i]) {
                std::cout << "❌ Bucket nulo en posición " << i << std::endl;
                is_consistent = false;
            }
        }
        
        // Verificar profundidades locales
        auto unique_buckets = getUniqueBuckets();
        for (const auto& bucket : unique_buckets) {
            if (bucket->getLocalDepth() > global_depth) {
                std::cout << "❌ Profundidad local > global: " 
                          << bucket->getLocalDepth() << " > " << global_depth << std::endl;
                is_consistent = false;
            }
        }
        
        if (is_consistent) {
            std::cout << "✅ Directorio consistente" << std::endl;
        }
        
        return is_consistent;
    }

    /**
     * @brief Estadísticas del directorio
     */
    struct DirectoryStats {
        int global_depth;
        size_t directory_size;
        size_t unique_bucket_count;
        double space_utilization;
        std::map<int, int> depth_distribution;
        size_t total_entries;
    };

    DirectoryStats getStats() const {
        DirectoryStats stats;
        stats.global_depth = global_depth;
        stats.directory_size = directory_size;
        stats.unique_bucket_count = getUniqueBucketCount();
        
        auto unique_buckets = getUniqueBuckets();
        stats.total_entries = 0;
        
        for (const auto& bucket : unique_buckets) {
            stats.total_entries += bucket->size();
            stats.depth_distribution[bucket->getLocalDepth()]++;
        }
        
        size_t total_capacity = unique_buckets.size() * bucket_capacity;
        stats.space_utilization = (total_capacity > 0) ? 
            static_cast<double>(stats.total_entries) / total_capacity : 0.0;
        
        return stats;
    }

    /**
     * @brief Análisis de fragmentación
     */
    void analyzeFragmentation() const {
        std::cout << "\n🧩 ANÁLISIS DE FRAGMENTACIÓN:" << std::endl;
        std::cout << "=============================" << std::endl;
        
        auto bucket_indices = getBucketIndices();
        
        std::cout << "Buckets únicos: " << bucket_indices.size() << std::endl;
        std::cout << "Entradas directorio: " << directory_size << std::endl;
        std::cout << "Factor de sharing: " << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(directory_size) / bucket_indices.size()) << std::endl;
        
        // Distribución de referencias por bucket
        std::map<size_t, int> ref_count_distribution;
        for (const auto& pair : bucket_indices) {
            ref_count_distribution[pair.second.size()]++;
        }
        
        std::cout << "\nDistribución de referencias:" << std::endl;
        for (const auto& pair : ref_count_distribution) {
            std::cout << "  " << pair.first << " refs: " << pair.second << " buckets" << std::endl;
        }
    }
};

#endif // DIRECTORY_H