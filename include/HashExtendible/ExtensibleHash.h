#ifndef EXTENSIBLE_HASH_H
#define EXTENSIBLE_HASH_H

#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include "Directory.h"
#include "Bucket.h"
#include "HashFunction.h"
#include "../Record.h"
#include "../RecordReference.h"

/**
 * @brief Hash Extensible ACTUALIZADO con RecordReference
 * 
 * ✅ CORRIGIDO PARA INTEGRACIÓN CON DISKMANAGER:
 * - Almacena RecordReference en lugar de Record completo
 * - Optimiza memoria de índices 
 * - Integra con DiskManager para resolución lazy
 * - Soporte completo para persistencia
 * - Previene claves duplicadas
 * - Estadísticas educativas detalladas
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
    size_t directory_expansions;

public:
    /**
     * @brief Constructor
     */
    ExtensibleHash(int capacity = 4) 
        : bucket_capacity(capacity)
        , total_records(0)
        , insert_operations(0)
        , search_operations(0)
        , split_operations(0)
        , directory_expansions(0) {
        
        directory = std::make_unique<Directory>(capacity);
        std::cout << "🔗 Hash Extensible inicializado (capacidad: " << capacity << ")" << std::endl;
    }
    
    // ============================================================================
    // OPERACIONES BÁSICAS CON RECORDREFERENCE
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN PRINCIPAL - Insertar usando RecordReference
     */
    bool insertReference(const std::string& key, const RecordReference& record_ref) {
        if (key.empty() || !record_ref.isValid()) {
            return false;
        }

        insert_operations++;

        // Obtener bucket apropiado
        auto bucket = directory->getBucket(key);
        if (!bucket) {
            std::cout << "❌ Error: No se pudo obtener bucket para clave: " << key << std::endl;
            return false;
        }

        // Verificar si necesita split
        if (bucket->needsSplit(key)) {
            std::cout << "🔄 Bucket lleno, iniciando split para clave: " << key << std::endl;
            
            if (!handleBucketSplit(key)) {
                std::cout << "❌ Error en split de bucket" << std::endl;
                return false;
            }

            // Obtener bucket después del split
            bucket = directory->getBucket(key);
            if (!bucket) {
                std::cout << "❌ Error: Bucket no disponible después del split" << std::endl;
                return false;
            }
        }

        // Insertar en bucket
        if (bucket->insert(key, record_ref)) {
            total_records++;
            std::cout << "✅ Insertado: " << key << " -> " << record_ref.toString() << std::endl;
            return true;
        }

        std::cout << "❌ Error insertando en bucket" << std::endl;
        return false;
    }

    /**
     * @brief Buscar clave y obtener RecordReference
     */
    bool search(const std::string& key, RecordReference& record_ref) {
        if (key.empty()) {
            return false;
        }

        search_operations++;

        auto bucket = directory->getBucket(key);
        if (!bucket) {
            return false;
        }

        return bucket->search(key, record_ref);
    }

    /**
     * @brief ✅ FUNCIÓN DE COMPATIBILIDAD - Insertar Record tradicional
     */
    bool insert(const std::string& key, std::unique_ptr<Record> record) {
        // Para compatibilidad con código existente
        // Crear RecordReference temporal (esto requeriría DiskManager)
        std::cout << "⚠️ insert() con Record: Usar insertReference() preferiblemente" << std::endl;
        
        // Crear referencia temporal - EN PRODUCCIÓN ESTO VENDRÍA DEL DISKMANAGER
        PhysicalAddress temp_addr(0, 0, 0, record->getId());
        RecordReference temp_ref(temp_addr, record->getId());
        
        return insertReference(key, temp_ref);
    }

    /**
     * @brief Eliminar entrada por clave
     */
    bool remove(const std::string& key) {
        if (key.empty()) {
            return false;
        }

        auto bucket = directory->getBucket(key);
        if (!bucket) {
            return false;
        }

        if (bucket->remove(key)) {
            total_records--;
            return true;
        }

        return false;
    }

    // ============================================================================
    // GESTIÓN DE SPLITS Y DIRECTORIO
    // ============================================================================

private:
    /**
     * @brief Maneja el split de bucket cuando está lleno
     */
    bool handleBucketSplit(const std::string& key) {
        auto bucket = directory->getBucket(key);
        if (!bucket) {
            return false;
        }

        split_operations++;

        // Si la profundidad local == profundidad global, expandir directorio
        if (bucket->getLocalDepth() == directory->getGlobalDepth()) {
            std::cout << "📈 Expandiendo directorio: depth " << directory->getGlobalDepth() 
                      << " -> " << (directory->getGlobalDepth() + 1) << std::endl;
            
            if (!directory->expand()) {
                std::cout << "❌ Error expandiendo directorio" << std::endl;
                return false;
            }
            directory_expansions++;
        }

        // Dividir bucket
        auto new_bucket = bucket->split();
        if (!new_bucket) {
            std::cout << "❌ Error dividiendo bucket" << std::endl;
            return false;
        }

        // Actualizar directorio con nuevo bucket
        return directory->updateAfterSplit(bucket, new_bucket);
    }

public:
    // ============================================================================
    // FUNCIONES DE ANÁLISIS Y PREDICCIÓN
    // ============================================================================

    /**
     * @brief ✅ Verifica si una inserción causará split
     */
    bool willCauseSplit(const std::string& key) const {
        auto bucket = directory->getBucket(key);
        return bucket ? bucket->needsSplit(key) : false;
    }

    /**
     * @brief Obtiene distribución de buckets
     */
    std::vector<std::pair<int, int>> getBucketDistribution() const {
        std::vector<std::pair<int, int>> distribution; // (local_depth, entry_count)
        
        auto unique_buckets = directory->getUniqueBuckets();
        for (const auto& bucket : unique_buckets) {
            distribution.emplace_back(bucket->getLocalDepth(), bucket->size());
        }
        
        return distribution;
    }

    /**
     * @brief Factor de carga promedio
     */
    double getLoadFactor() const {
        if (total_records == 0) return 0.0;
        
        auto unique_buckets = directory->getUniqueBuckets();
        size_t total_capacity = unique_buckets.size() * bucket_capacity;
        
        return static_cast<double>(total_records) / total_capacity;
    }

    // ============================================================================
    // ACCESO A DATOS
    // ============================================================================

    /**
     * @brief ✅ Obtiene todas las claves
     */
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        auto unique_buckets = directory->getUniqueBuckets();
        
        for (const auto& bucket : unique_buckets) {
            auto bucket_keys = bucket->getAllKeys();
            keys.insert(keys.end(), bucket_keys.begin(), bucket_keys.end());
        }
        
        return keys;
    }

    /**
     * @brief ✅ Obtiene todas las referencias
     */
    std::vector<RecordReference> getAllReferences() const {
        std::vector<RecordReference> refs;
        auto unique_buckets = directory->getUniqueBuckets();
        
        for (const auto& bucket : unique_buckets) {
            auto bucket_refs = bucket->getAllReferences();
            refs.insert(refs.end(), bucket_refs.begin(), bucket_refs.end());
        }
        
        return refs;
    }

    /**
     * @brief Obtiene pares clave-referencia
     */
    std::vector<std::pair<std::string, RecordReference>> getAllEntries() const {
        std::vector<std::pair<std::string, RecordReference>> entries;
        auto unique_buckets = directory->getUniqueBuckets();
        
        for (const auto& bucket : unique_buckets) {
            auto bucket_entries = bucket->getAllEntries();
            entries.insert(entries.end(), bucket_entries.begin(), bucket_entries.end());
        }
        
        return entries;
    }

    // ============================================================================
    // ESTADÍSTICAS Y INFORMACIÓN
    // ============================================================================

    size_t getTotalRecords() const { return total_records; }
    size_t getInsertOperations() const { return insert_operations; }
    size_t getSearchOperations() const { return search_operations; }
    size_t getSplitOperations() const { return split_operations; }
    size_t getDirectoryExpansions() const { return directory_expansions; }
    int getBucketCapacity() const { return bucket_capacity; }

    /**
     * @brief Estadísticas completas
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        ss << "🔗 ESTADÍSTICAS HASH EXTENSIBLE:" << std::endl;
        ss << "================================" << std::endl;
        ss << "Total registros: " << total_records << std::endl;
        ss << "Operaciones inserción: " << insert_operations << std::endl;
        ss << "Operaciones búsqueda: " << search_operations << std::endl;
        ss << "Splits realizados: " << split_operations << std::endl;
        ss << "Expansiones directorio: " << directory_expansions << std::endl;
        
        ss << "\n📊 DIRECTORIO:" << std::endl;
        ss << "Profundidad global: " << directory->getGlobalDepth() << std::endl;
        ss << "Tamaño directorio: " << directory->getSize() << std::endl;
        ss << "Buckets únicos: " << directory->getUniqueBuckets().size() << std::endl;
        
        ss << "\n📦 BUCKETS:" << std::endl;
        ss << "Capacidad por bucket: " << bucket_capacity << std::endl;
        ss << "Factor de carga: " << std::fixed << std::setprecision(2) 
           << (getLoadFactor() * 100) << "%" << std::endl;

        // Distribución de ocupación
        auto distribution = getBucketDistribution();
        std::map<int, int> depth_count;
        int min_entries = bucket_capacity, max_entries = 0;
        
        for (const auto& pair : distribution) {
            depth_count[pair.first]++;
            min_entries = std::min(min_entries, pair.second);
            max_entries = std::max(max_entries, pair.second);
        }
        
        ss << "Entradas por bucket: [" << min_entries << " - " << max_entries << "]" << std::endl;
        
        ss << "\n🔍 PROFUNDIDADES LOCALES:" << std::endl;
        for (const auto& pair : depth_count) {
            ss << "Depth " << pair.first << ": " << pair.second << " buckets" << std::endl;
        }

        return ss.str();
    }

    /**
     * @brief Muestra estadísticas detalladas
     */
    void displayStatistics() const {
        std::cout << getStatistics() << std::endl;
    }
    
    /**
     * @brief Muestra estructura del directorio (educativo)
     */
    void displayStructure() const {
        std::cout << "\n🏗️ ESTRUCTURA DEL HASH EXTENSIBLE:" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        
        directory->display();
        
        std::cout << "\n📋 RESUMEN DE BUCKETS:" << std::endl;
        auto unique_buckets = directory->getUniqueBuckets();
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            std::cout << "Bucket " << i << " -> " 
                      << unique_buckets[i]->getRecordCount() << "/" << bucket_capacity << " registros" << std::endl;
        }
    }

    // ============================================================================
    // PERSISTENCIA
    // ============================================================================
    
    /**
     * @brief Serializa el hash completo para persistencia
     */
    std::string serialize() const {
        std::ostringstream oss;
        
        oss << "EXTENSIBLE_HASH_V2" << std::endl;
        oss << "bucket_capacity=" << bucket_capacity << std::endl;
        oss << "total_records=" << total_records << std::endl;
        oss << "insert_operations=" << insert_operations << std::endl;
        oss << "search_operations=" << search_operations << std::endl;
        oss << "split_operations=" << split_operations << std::endl;
        oss << "directory_expansions=" << directory_expansions << std::endl;
        oss << "END_METADATA" << std::endl;
        
        // Serializar directorio
        oss << directory->serialize() << std::endl;
        
        return oss.str();
    }

    /**
     * @brief Deserializa desde string
     */
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string line;
        
        // Verificar formato
        std::getline(iss, line);
        if (line != "EXTENSIBLE_HASH_V2") {
            std::cout << "❌ Formato de hash inválido" << std::endl;
            return false;
        }

        // Leer metadatos
        while (std::getline(iss, line) && line != "END_METADATA") {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                if (key == "bucket_capacity") {
                    bucket_capacity = std::stoi(value);
                } else if (key == "total_records") {
                    total_records = std::stoull(value);
                } else if (key == "insert_operations") {
                    insert_operations = std::stoull(value);
                } else if (key == "search_operations") {
                    search_operations = std::stoull(value);
                } else if (key == "split_operations") {
                    split_operations = std::stoull(value);
                } else if (key == "directory_expansions") {
                    directory_expansions = std::stoull(value);
                }
            }
        }

        // Leer contenido del directorio
        std::stringstream remaining_content;
        while (std::getline(iss, line)) {
            remaining_content << line << std::endl;
        }

        // Recrear directorio y deserializar
        directory = std::make_unique<Directory>(bucket_capacity);
        return directory->deserialize(remaining_content.str());
    }

    // ============================================================================
    // ANÁLISIS EDUCATIVO
    // ============================================================================

    /**
     * @brief Análisis de distribución de claves
     */
    void analyzeKeyDistribution() const {
        auto keys = getAllKeys();
        if (keys.empty()) {
            std::cout << "⚠️ No hay claves para analizar" << std::endl;
            return;
        }

        std::cout << "\n🔍 ANÁLISIS DE DISTRIBUCIÓN DE CLAVES:" << std::endl;
        std::cout << "=====================================" << std::endl;

        // Usar función de hash para análisis
        HashFunction::analyzeHashDistribution(keys, directory->getGlobalDepth());
        
        // Análisis de colisiones
        HashFunction::analyzeCollisions(keys);
    }

    /**
     * @brief Simulación de inserción (educativo)
     */
    void simulateInsertion(const std::string& key) const {
        std::cout << "\n🎯 SIMULACIÓN DE INSERCIÓN: '" << key << "'" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;

        // Mostrar información de hash
        HashFunction::showHashInfo(key, directory->getGlobalDepth() + 2);

        // Mostrar bucket destino
        auto bucket = directory->getBucket(key);
        if (bucket) {
            std::cout << "\n📦 BUCKET DESTINO:" << std::endl;
            bucket->displayDetailed();
            
            std::cout << "\n🔄 PREDICCIÓN:" << std::endl;
            if (bucket->needsSplit(key)) {
                std::cout << "⚠️ Esta inserción causará SPLIT" << std::endl;
                if (bucket->getLocalDepth() == directory->getGlobalDepth()) {
                    std::cout << "📈 También causará EXPANSIÓN DE DIRECTORIO" << std::endl;
                }
            } else {
                std::cout << "✅ Inserción normal (sin split)" << std::endl;
            }
        }
    }

    /**
     * @brief Información de rendimiento
     */
    void displayPerformanceInfo() const {
        std::cout << "\n⚡ INFORMACIÓN DE RENDIMIENTO:" << std::endl;
        std::cout << "============================" << std::endl;

        if (search_operations > 0) {
            std::cout << "Promedio accesos por búsqueda: 1.0 (acceso directo)" << std::endl;
        }

        if (insert_operations > 0) {
            double split_rate = static_cast<double>(split_operations) / insert_operations;
            std::cout << "Tasa de splits: " << std::fixed << std::setprecision(2) 
                      << (split_rate * 100) << "%" << std::endl;
        }

        std::cout << "Eficiencia de espacio: " << std::fixed << std::setprecision(1) 
                  << (getLoadFactor() * 100) << "%" << std::endl;

        // Complejidad teórica
        std::cout << "\n📊 COMPLEJIDAD TEÓRICA:" << std::endl;
        std::cout << "Búsqueda: O(1) - acceso directo via hash" << std::endl;
        std::cout << "Inserción: O(1) amortizado - splits ocasionales" << std::endl;
        std::cout << "Eliminación: O(1) - acceso directo" << std::endl;
    }

    /**
     * @brief Validación de consistencia completa
     */
    bool validateConsistency() const {
        std::cout << "\n🔍 VALIDANDO CONSISTENCIA DEL HASH EXTENSIBLE..." << std::endl;

        bool is_consistent = true;

        // Validar directorio
        if (!directory->validateConsistency()) {
            std::cout << "❌ Directorio inconsistente" << std::endl;
            is_consistent = false;
        }

        // Validar buckets
        auto unique_buckets = directory->getUniqueBuckets();
        for (size_t i = 0; i < unique_buckets.size(); i++) {
            if (!unique_buckets[i]->validateConsistency()) {
                std::cout << "❌ Bucket " << i << " inconsistente" << std::endl;
                is_consistent = false;
            }
        }

        // Validar conteo de registros
        size_t counted_records = 0;
        for (const auto& bucket : unique_buckets) {
            counted_records += bucket->getRecordCount();
        }

        if (counted_records != total_records) {
            std::cout << "❌ Conteo de registros inconsistente: " 
                      << counted_records << " != " << total_records << std::endl;
            is_consistent = false;
        }

        if (is_consistent) {
            std::cout << "✅ Hash Extensible consistente" << std::endl;
        }

        return is_consistent;
    }
};

#endif // EXTENSIBLE_HASH_H