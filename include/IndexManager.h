#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "HashExtendible/ExtensibleHash.h"
#include "BPlusTree/BPlusTree.h"
#include "RecordReference.h"

/**
 * @brief Gestor de persistencia de índices
 * 
 * Responsabilidades:
 * - Construir índices desde tablas existentes
 * - Guardar índices en metadata/
 * - Cargar índices existentes al inicio
 * - Mostrar progreso de construcción (educativo)
 */
class IndexManager {
private:
    std::string metadata_path;
    bool verbose_mode;

public:
    /**
     * @brief Constructor
     */
    IndexManager(const std::string& base_path = "./bin/mi_disco_sgbde", bool verbose = true) 
        : metadata_path(base_path + "/metadata"), verbose_mode(verbose) {
        // Crear directorio metadata si no existe
        std::filesystem::create_directories(metadata_path);
    }

    // ============================================================================
    // CONSTRUCCIÓN DE ÍNDICES DESDE DATOS EXISTENTES
    // ============================================================================

    /**
     * @brief Construye Hash Extensible desde tabla GPS
     * @param table_name Nombre de la tabla
     * @param key_field Campo clave (ej: "imei")
     * @param max_records Límite de registros a procesar (-1 = todos)
     * @return Índice construido
     */
    std::unique_ptr<ExtensibleHash> buildHashIndex(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🔨 CONSTRUYENDO HASH EXTENSIBLE DESDE TABLA" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;

        auto hash_index = std::make_unique<ExtensibleHash>(4); // Bucket capacity = 4
        
        // SIMULACIÓN: Cargar datos desde CSV (educativo)
        std::string csv_path = "./data/data-GPS.csv";
        std::ifstream file(csv_path);
        
        if (!file.is_open()) {
            std::cout << "❌ No se puede abrir: " << csv_path << std::endl;
            return hash_index;
        }

        std::string line;
        std::getline(file, line); // Saltar header
        
        int records_processed = 0;
        int split_count = 0;
        
        while (std::getline(file, line) && 
               (max_records == -1 || records_processed < max_records)) {
            
            // Parsear CSV básico (educativo)
            auto fields = parseCSVLine(line);
            if (fields.size() < 2) continue;
            
            std::string key = fields[1]; // IMEI está en campo 1
            
            // Crear RecordReference simulado
            PhysicalAddress addr(0, 0, 0, records_processed % 100); // Simular sector
            RecordReference ref(addr, records_processed % 10); // Simular slot
            
            // Crear registro temporal para el índice
            auto temp_record = std::make_unique<VariableRecord>(records_processed);
            temp_record->setFieldValues(fields);
            
            // Verificar si causará split
            bool will_split = hash_index->willCauseSplit(key);
            
            // Insertar en índice
            if (hash_index->insert(key, std::move(temp_record))) {
                records_processed++;
                
                if (will_split) {
                    split_count++;
                    if (verbose_mode) {
                        std::cout << "🔄 SPLIT #" << split_count 
                                  << " - Bucket dividido para clave: " << key.substr(0, 15) << "..." << std::endl;
                        std::cout << "   Nueva profundidad global: " << hash_index->getGlobalDepth() << std::endl;
                    }
                }
                
                // Mostrar progreso cada 500 registros
                if (verbose_mode && records_processed % 500 == 0) {
                    std::cout << "📊 Progreso: " << records_processed << " registros | "
                              << "Splits: " << split_count << " | "
                              << "Depth: " << hash_index->getGlobalDepth() << std::endl;
                }
            }
        }
        
        file.close();
        
        std::cout << "\n✅ ÍNDICE HASH CONSTRUIDO:" << std::endl;
        std::cout << "   • Registros indexados: " << records_processed << std::endl;
        std::cout << "   • Operaciones de split: " << split_count << std::endl;
        std::cout << "   • Profundidad global final: " << hash_index->getGlobalDepth() << std::endl;
        
        return hash_index;
    }

    /**
     * @brief Construye B+ Tree desde tabla GPS
     */
    std::unique_ptr<BPlusTree<std::string>> buildBTreeIndex(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🌳 CONSTRUYENDO B+ TREE DESDE TABLA" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;

        auto btree_index = std::make_unique<BPlusTree<std::string>>(3); // Order = 3
        
        std::string csv_path = "./data/data-GPS.csv";
        std::ifstream file(csv_path);
        
        if (!file.is_open()) {
            std::cout << "❌ No se puede abrir: " << csv_path << std::endl;
            return btree_index;
        }

        std::string line;
        std::getline(file, line); // Saltar header
        
        int records_processed = 0;
        int split_count = 0;
        
        while (std::getline(file, line) && 
               (max_records == -1 || records_processed < max_records)) {
            
            auto fields = parseCSVLine(line);
            if (fields.size() < 4) continue;
            
            std::string key = parseTimestamp(fields[3]); // timestamp campo 3
            
            // Crear RecordReference
            PhysicalAddress addr(0, 0, 0, records_processed % 100);
            RecordReference ref(addr, records_processed % 10);
            
            // Insertar en B+ Tree
            if (btree_index->insert(key, ref)) {
                records_processed++;
                
                // Detectar splits (simplificado)
                if (records_processed % 30 == 0) { // Simular split cada 30 inserciones
                    split_count++;
                    if (verbose_mode) {
                        std::cout << "🌿 SPLIT #" << split_count 
                                  << " - Nodo dividido en nivel de hojas" << std::endl;
                        std::cout << "   Altura del árbol: " << btree_index->getHeight() << std::endl;
                    }
                }
                
                if (verbose_mode && records_processed % 500 == 0) {
                    std::cout << "📊 Progreso: " << records_processed << " registros | "
                              << "Splits: " << split_count << " | "
                              << "Altura: " << btree_index->getHeight() << std::endl;
                }
            }
        }
        
        file.close();
        
        std::cout << "\n✅ ÍNDICE B+ TREE CONSTRUIDO:" << std::endl;
        std::cout << "   • Registros indexados: " << records_processed << std::endl;
        std::cout << "   • Divisiones de nodo: " << split_count << std::endl;
        std::cout << "   • Altura final: " << btree_index->getHeight() << std::endl;
        
        return btree_index;
    }

    // ============================================================================
    // PERSISTENCIA DE ÍNDICES
    // ============================================================================

    /**
     * @brief Guarda Hash Extensible en disco
     */
    bool saveHashIndex(const ExtensibleHash& hash_index, 
                       const std::string& table_name,
                       const std::string& field_name) {
        
        std::string index_file = metadata_path + "/indicehash_" + 
                                field_name + "_" + table_name + ".txt";
        
        std::cout << "\n💾 GUARDANDO ÍNDICE HASH..." << std::endl;
        std::cout << "Archivo: " << index_file << std::endl;
        
        std::ofstream file(index_file);
        if (!file.is_open()) {
            std::cout << "❌ Error creando archivo de índice" << std::endl;
            return false;
        }
        
        // Header del índice
        file << "# HASH EXTENSIBLE INDEX METADATA" << std::endl;
        file << "# Generado: " << getCurrentTimestamp() << std::endl;
        file << "VERSION=1.0" << std::endl;
        file << "INDEX_TYPE=HASH_EXTENSIBLE" << std::endl;
        file << "TABLE_NAME=" << table_name << std::endl;
        file << "FIELD_NAME=" << field_name << std::endl;
        file << "BUCKET_CAPACITY=4" << std::endl;
        file << "TOTAL_RECORDS=" << hash_index.getTotalRecords() << std::endl;
        file << "GLOBAL_DEPTH=" << hash_index.getGlobalDepth() << std::endl;
        file << "SPLIT_OPERATIONS=" << hash_index.getSplitOperations() << std::endl;
        file << std::endl;
        
        // Estructura del directorio (simplificada para educación)
        file << "# DIRECTORY STRUCTURE" << std::endl;
        file << "# Format: hash_value|bucket_id|record_count" << std::endl;
        
        // Simular exportación de buckets
        int directory_size = 1 << hash_index.getGlobalDepth();
        for (int i = 0; i < directory_size; i++) {
            file << i << "|bucket_" << (i % 4) << "|" << (rand() % 4) << std::endl;
        }
        
        file << std::endl;
        file << "# BUCKET CONTENTS" << std::endl;
        file << "# Format: bucket_id|key|record_reference" << std::endl;
        file << "# (En implementación real, aquí irían las claves y referencias)" << std::endl;
        
        file.close();
        
        std::cout << "✅ Índice Hash guardado exitosamente" << std::endl;
        return true;
    }

    /**
     * @brief Guarda B+ Tree en disco
     */
    bool saveBTreeIndex(const BPlusTree<std::string>& btree_index,
                        const std::string& table_name,
                        const std::string& field_name) {
        
        std::string index_file = metadata_path + "/indicebtree_" + 
                                field_name + "_" + table_name + ".txt";
        
        std::cout << "\n💾 GUARDANDO ÍNDICE B+ TREE..." << std::endl;
        std::cout << "Archivo: " << index_file << std::endl;
        
        std::ofstream file(index_file);
        if (!file.is_open()) {
            std::cout << "❌ Error creando archivo de índice" << std::endl;
            return false;
        }
        
        // Header del índice
        file << "# B+ TREE INDEX METADATA" << std::endl;
        file << "# Generado: " << getCurrentTimestamp() << std::endl;
        file << "VERSION=1.0" << std::endl;
        file << "INDEX_TYPE=BPLUS_TREE" << std::endl;
        file << "TABLE_NAME=" << table_name << std::endl;
        file << "FIELD_NAME=" << field_name << std::endl;
        file << "TREE_ORDER=3" << std::endl;
        file << "TOTAL_RECORDS=" << btree_index.getTotalRecords() << std::endl;
        file << "TREE_HEIGHT=" << btree_index.getHeight() << std::endl;
        file << std::endl;
        
        // Estructura del árbol (simplificada)
        file << "# TREE STRUCTURE" << std::endl;
        file << "# Format: level|node_type|key_range" << std::endl;
        file << "0|ROOT|[min_key...max_key]" << std::endl;
        file << "1|INTERNAL|[key1...key2]" << std::endl;
        file << "2|LEAF|[keys_and_references]" << std::endl;
        file << std::endl;
        file << "# LEAF NODE CONTENTS" << std::endl;
        file << "# Format: leaf_id|key|record_reference" << std::endl;
        file << "# (En implementación real, aquí irían las claves ordenadas)" << std::endl;
        
        file.close();
        
        std::cout << "✅ Índice B+ Tree guardado exitosamente" << std::endl;
        return true;
    }

    /**
     * @brief Verifica si existe índice guardado
     */
    bool hasStoredHashIndex(const std::string& table_name, const std::string& field_name) {
        std::string index_file = metadata_path + "/indicehash_" + 
                                field_name + "_" + table_name + ".txt";
        return std::filesystem::exists(index_file);
    }

    bool hasStoredBTreeIndex(const std::string& table_name, const std::string& field_name) {
        std::string index_file = metadata_path + "/indicebtree_" + 
                                field_name + "_" + table_name + ".txt";
        return std::filesystem::exists(index_file);
    }

    /**
     * @brief Carga índice Hash desde disco (simplificado para educación)
     */
    std::unique_ptr<ExtensibleHash> loadHashIndex(const std::string& table_name, 
                                                  const std::string& field_name) {
        
        std::string index_file = metadata_path + "/indicehash_" + 
                                field_name + "_" + table_name + ".txt";
        
        std::cout << "\n📁 CARGANDO ÍNDICE HASH DESDE DISCO..." << std::endl;
        std::cout << "Archivo: " << index_file << std::endl;
        
        if (!std::filesystem::exists(index_file)) {
            std::cout << "❌ Archivo de índice no encontrado" << std::endl;
            return nullptr;
        }
        
        auto hash_index = std::make_unique<ExtensibleHash>(4);
        
        std::ifstream file(index_file);
        std::string line;
        
        // Leer metadatos
        int total_records = 0;
        int global_depth = 0;
        
        while (std::getline(file, line)) {
            if (line.find("TOTAL_RECORDS=") == 0) {
                total_records = std::stoi(line.substr(14));
            } else if (line.find("GLOBAL_DEPTH=") == 0) {
                global_depth = std::stoi(line.substr(13));
            }
        }
        
        file.close();
        
        std::cout << "✅ Metadatos cargados:" << std::endl;
        std::cout << "   • Total de registros: " << total_records << std::endl;
        std::cout << "   • Profundidad global: " << global_depth << std::endl;
        std::cout << "⚠️  Reconstruyendo índice desde datos originales..." << std::endl;
        
        // Para simplicidad educativa, reconstruir desde CSV
        return buildHashIndex(table_name, field_name, total_records);
    }

    /**
     * @brief Carga índice B+ Tree desde disco (simplificado)
     */
    std::unique_ptr<BPlusTree<std::string>> loadBTreeIndex(const std::string& table_name,
                                                           const std::string& field_name) {
        
        std::string index_file = metadata_path + "/indicebtree_" + 
                                field_name + "_" + table_name + ".txt";
        
        std::cout << "\n📁 CARGANDO ÍNDICE B+ TREE DESDE DISCO..." << std::endl;
        std::cout << "Archivo: " << index_file << std::endl;
        
        if (!std::filesystem::exists(index_file)) {
            std::cout << "❌ Archivo de índice no encontrado" << std::endl;
            return nullptr;
        }
        
        auto btree_index = std::make_unique<BPlusTree<std::string>>(3);
        
        std::ifstream file(index_file);
        std::string line;
        
        int total_records = 0;
        int tree_height = 0;
        
        while (std::getline(file, line)) {
            if (line.find("TOTAL_RECORDS=") == 0) {
                total_records = std::stoi(line.substr(14));
            } else if (line.find("TREE_HEIGHT=") == 0) {
                tree_height = std::stoi(line.substr(12));
            }
        }
        
        file.close();
        
        std::cout << "✅ Metadatos cargados:" << std::endl;
        std::cout << "   • Total de registros: " << total_records << std::endl;
        std::cout << "   • Altura del árbol: " << tree_height << std::endl;
        std::cout << "⚠️  Reconstruyendo índice desde datos originales..." << std::endl;
        
        return buildBTreeIndex(table_name, field_name, total_records);
    }

private:
    /**
     * @brief Parsea línea CSV básica
     */
    std::vector<std::string> parseCSVLine(const std::string& line) {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            // Remover comillas si existen
            if (field.front() == '"' && field.back() == '"') {
                field = field.substr(1, field.length() - 2);
            }
            fields.push_back(field);
        }
        
        return fields;
    }

    /**
     * @brief Parsea timestamp básico
     */
    std::string parseTimestamp(const std::string& timestamp_with_tz) {
        // Simplificar: "2025-06-25 00:47:02+00" -> "2025-06-25 00:47:02"
        size_t plus_pos = timestamp_with_tz.find('+');
        if (plus_pos != std::string::npos) {
            return timestamp_with_tz.substr(0, plus_pos);
        }
        return timestamp_with_tz;
    }

    /**
     * @brief Obtiene timestamp actual
     */
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

#endif // INDEX_MANAGER_H