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
     * @brief ✅ FUNCIÓN AGREGADA - Obtiene bucket por índice
     */
    std::shared_ptr<Bucket> getBucketByIndex(int index) {
        if (index < 0 || index >= static_cast<int>(directory.size())) {
            return nullptr;
        }
        return directory[index];
    }
    
    /**
     * @brief Divide un bucket cuando está lleno
     */
    bool splitBucket(const std::string& key) {
        auto bucket = getBucket(key);
        
        if (!bucket->isFull()) {
            return false; // No necesita división
        }
        
        std::cout << "🔄 Iniciando división de bucket..." << std::endl;
        
        // Verificar si necesitamos expandir el directorio
        if (bucket->getLocalDepth() == global_depth) {
            expandDirectory();
        }
        
        // Crear nuevo bucket
        int new_local_depth = bucket->getLocalDepth() + 1;
        auto new_bucket = std::make_shared<Bucket>(bucket_capacity, new_local_depth);
        
        // Incrementar profundidad local del bucket original
        bucket->incrementLocalDepth();
        
        // Redistribuir registros
        redistributeRecords(bucket, new_bucket);
        
        // Actualizar punteros del directorio
        updateDirectoryPointers(bucket, new_bucket);
        
        std::cout << "✅ División completada exitosamente" << std::endl;
        return true;
    }
    
    /**
     * @brief Expande el directorio duplicando su tamaño
     */
    void expandDirectory() {
        std::cout << "📈 Expandiendo directorio: " << global_depth << " → " << (global_depth + 1) << std::endl;
        
        global_depth++;
        size_t new_size = 1 << global_depth;
        
        // Duplicar entradas existentes
        directory.resize(new_size);
        for (size_t i = directory_size; i < new_size; i++) {
            directory[i] = directory[i - directory_size];
        }
        
        directory_size = new_size;
        
        std::cout << "✅ Directorio expandido a tamaño: " << directory_size << std::endl;
    }
    
    // ============================================================================
    // ESTADÍSTICAS Y INFORMACIÓN
    // ============================================================================
    
    /**
     * @brief Muestra estructura del directorio
     */
    void display() const {
        std::cout << "\n📁 ESTRUCTURA DEL DIRECTORIO" << std::endl;
        std::cout << "=========================================" << std::endl;
        std::cout << "Profundidad Global: " << global_depth << std::endl;
        std::cout << "Tamaño Directorio: " << directory_size << std::endl;
        std::cout << "Capacidad por Bucket: " << bucket_capacity << std::endl;
        
        auto unique_buckets = getUniqueBuckets();
        std::cout << "Buckets Únicos: " << unique_buckets.size() << std::endl;
        
        std::cout << "\n📋 Mapeo Directorio → Buckets:" << std::endl;
        for (size_t i = 0; i < std::min(directory.size(), size_t(16)); i++) { // Mostrar máximo 16 entradas
            std::cout << "  [" << i << "] → Bucket@" << directory[i].get() 
                      << " (LD:" << directory[i]->getLocalDepth() 
                      << ", Records:" << directory[i]->getRecordCount() << ")" << std::endl;
        }
        
        if (directory.size() > 16) {
            std::cout << "  ... (" << (directory.size() - 16) << " entradas más)" << std::endl;
        }
        
        std::cout << "\n📊 Detalles de Buckets Únicos:" << std::endl;
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            std::cout << "\n--- Bucket " << i << " ---" << std::endl;
            unique_buckets[i]->display();
        }
    }
    
    /**
     * @brief Obtiene estadísticas detalladas
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        auto unique_buckets = getUniqueBuckets();
        
        ss << "=== ESTADÍSTICAS DEL DIRECTORIO ===\n";
        ss << "Profundidad Global: " << global_depth << "\n";
        ss << "Tamaño Directorio: " << directory_size << "\n";
        ss << "Buckets Únicos: " << unique_buckets.size() << "\n";
        ss << "Capacidad por Bucket: " << bucket_capacity << "\n";
        
        // Calcular total de registros
        size_t total_records = 0;
        for (const auto& bucket : unique_buckets) {
            total_records += bucket->getRecordCount();
        }
        
        ss << "Total Registros: " << total_records << "\n";
        
        if (!unique_buckets.empty()) {
            double load_factor = (double)total_records / (unique_buckets.size() * bucket_capacity);
            ss << "Load Factor: " << std::fixed << std::setprecision(2) << load_factor << "\n";
        }
        
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
    
    // ============================================================================
    // ✅ FUNCIÓN AGREGADA - BÚSQUEDA Y ANÁLISIS
    // ============================================================================
    
    /**
     * @brief Analiza la distribución de claves
     */
    std::map<std::string, int> analyzeKeyDistribution() const {
        std::map<std::string, int> distribution;
        auto unique_buckets = getUniqueBuckets();
        
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            std::string bucket_id = "bucket_" + std::to_string(i);
            distribution[bucket_id] = unique_buckets[i]->getRecordCount();
        }
        
        return distribution;
    }
    
    /**
     * @brief Verifica integridad del directorio
     */
    bool validateIntegrity() const {
        // Verificar que todos los punteros son válidos
        for (const auto& bucket : directory) {
            if (!bucket) {
                std::cout << "❌ Bucket nulo encontrado en directorio" << std::endl;
                return false;
            }
            
            if (!bucket->validateIntegrity()) {
                std::cout << "❌ Bucket con integridad comprometida" << std::endl;
                return false;
            }
        }
        
        std::cout << "✅ Integridad del directorio verificada" << std::endl;
        return true;
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
        
        // Obtener todas las entradas del bucket
        auto all_entries = old_bucket->extractAllEntries();
        
        // Limpiar bucket original
        old_bucket->clear();
        
        int records_in_old = 0;
        int records_in_new = 0;
        
        // Redistribuir basándose en hash con nueva profundidad local
        for (auto& entry : all_entries) {
            size_t hash_value = std::hash<std::string>{}(entry.key);
            int new_local_depth = old_bucket->getLocalDepth();
            size_t mask = (1 << new_local_depth) - 1;
            size_t bucket_index = hash_value & mask;
            
            // El bit más significativo determina a cuál bucket va
            if (bucket_index & (1 << (new_local_depth - 1))) {
                new_bucket->insertRecord(entry.key, std::move(entry.record));
                records_in_new++;
            } else {
                old_bucket->insertRecord(entry.key, std::move(entry.record));
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
        
        for (size_t i = 0; i < directory.size(); i++) {
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