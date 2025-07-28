#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>

#include "HashExtendible/ExtensibleHash.h"
#include "BPlusTree/BPlusTree.h"
#include "RecordReference.h"
#include "DiskManagerExtended.h"
#include "Block.h"

/**
 * @brief IndexManager - Gestor de persistencia de índices CORREGIDO
 * 
 * ✅ CORRIGE EL PROBLEMA PRINCIPAL:
 * - Ya NO lee desde CSV directamente
 * - USA DiskManagerExtended::getTablePages() como debe ser
 * - Integra RecordReference como puente
 * - Previene claves duplicadas
 * - Mantiene arquitectura Buffer Pool + Page Directory
 * 
 * Funcionalidades:
 * - Construye Hash Extensible desde páginas físicas del DiskManager
 * - Construye B+ Tree desde páginas físicas del DiskManager
 * - Guarda/carga índices de forma persistente
 * - Maneja metadatos de índices
 * - Integra con RecordReference para acceso lazy
 */
class IndexManager {
private:
    std::string base_path;              // Ruta base del sistema
    std::string index_data_path;        // Ruta de datos de índices
    std::string metadata_path;          // Ruta de metadatos
    bool enable_persistence;            // Si la persistencia está habilitada
    
    // Referencias al DiskManager (NO CSV)
    DiskManagerExtended* disk_manager;  // Referencia al DiskManager
    
    // Cache de metadatos
    std::unordered_map<std::string, size_t> index_metadata;

public:
    /**
     * @brief Constructor
     */
    IndexManager(const std::string& path, bool enable_persist = true, DiskManagerExtended* dm = nullptr)
        : base_path(path)
        , index_data_path(path + "/metadata/indexes")
        , metadata_path(path + "/metadata")
        , enable_persistence(enable_persist)
        , disk_manager(dm)
    {
        if (enable_persistence) {
            std::filesystem::create_directories(index_data_path);
            std::filesystem::create_directories(metadata_path);
        }
        
        std::cout << "🗂️ IndexManager inicializado:" << std::endl;
        std::cout << "   📁 Ruta base: " << base_path << std::endl;
        std::cout << "   💾 Persistencia: " << (enable_persistence ? "Habilitada" : "Deshabilitada") << std::endl;
        std::cout << "   🔗 DiskManager: " << (disk_manager ? "Conectado" : "No conectado") << std::endl;
    }

    /**
     * @brief Establece referencia al DiskManager
     */
    void setDiskManager(DiskManagerExtended* dm) {
        disk_manager = dm;
        std::cout << "🔗 IndexManager conectado a DiskManager" << std::endl;
    }

    // ============================================================================
    // CONSTRUCCIÓN DE ÍNDICES DESDE DISKMANAGER (CORREGIDO)
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN PRINCIPAL CORREGIDA - Construye Hash Extensible desde DiskManager
     * 
     * ANTES: Leía CSV directamente ❌
     * AHORA: USA getTablePages() del DiskManager ✅
     */
    std::unique_ptr<ExtensibleHash> buildHashIndexFromDisk(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🔨 CONSTRUYENDO HASH EXTENSIBLE DESDE DISKMANAGER" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "🔍 Fuente: DiskManager (NO CSV)" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;

        if (!disk_manager) {
            std::cout << "❌ Error: DiskManager no disponible" << std::endl;
            return nullptr;
        }

        auto hash_index = std::make_unique<ExtensibleHash>(4);
        
        // ✅ CORRECCIÓN PRINCIPAL: Usar getTablePages() en lugar de CSV
        std::vector<PhysicalAddress> table_pages;
        if (!disk_manager->getTablePages(table_name, table_pages)) {
            std::cout << "❌ Error: No se pudieron obtener páginas de tabla " << table_name << std::endl;
            return hash_index;
        }

        std::cout << "📊 Páginas encontradas: " << table_pages.size() << std::endl;

        int records_processed = 0;
        int split_count = 0;
        std::unordered_set<std::string> processed_keys; // ✅ Prevenir duplicados

        // Iterar sobre cada página física
        for (const auto& page_addr : table_pages) {
            if (max_records != -1 && records_processed >= max_records) {
                break;
            }

            std::cout << "📄 Procesando página: " << page_addr.toString() << std::endl;

            // Leer bloque desde DiskManager
            Block page_block(page_addr, 4096);
            if (!disk_manager->readBlock(page_addr, page_block)) {
                std::cout << "⚠️ No se pudo leer página: " << page_addr.toString() << std::endl;
                continue;
            }

            // Obtener registros activos del bloque
            auto active_records = page_block.getActiveRecords();
            std::cout << "   📝 Registros activos: " << active_records.size() << std::endl;

            // Procesar cada registro en la página
            for (const auto& record : active_records) {
                if (max_records != -1 && records_processed >= max_records) {
                    break;
                }

                // Extraer clave del registro
                std::string key = extractKeyFromRecord(record, key_field);
                if (key.empty()) {
                    continue;
                }

                // ✅ Verificar duplicados
                if (processed_keys.find(key) != processed_keys.end()) {
                    std::cout << "⚠️ Clave duplicada ignorada: " << key << std::endl;
                    continue;
                }
                processed_keys.insert(key);

                // Crear RecordReference para el registro
                RecordReference record_ref = disk_manager->createRecordReference(page_addr, record->getId());
                record_ref.setCachedKey(key); // Optimización

                // Verificar si causará split
                if (hash_index->willCauseSplit(key)) {
                    split_count++;
                    std::cout << "🔄 Split #" << split_count << " en registro " << records_processed << std::endl;
                }

                // Insertar en índice usando RecordReference
                if (hash_index->insertReference(key, record_ref)) {
                    records_processed++;
                    
                    if (records_processed % 500 == 0) {
                        std::cout << "📈 Procesados: " << records_processed << " registros únicos" << std::endl;
                    }
                }
            }
        }

        std::cout << "\n✅ HASH EXTENSIBLE CONSTRUIDO:" << std::endl;
        std::cout << "   📊 Total registros: " << records_processed << std::endl;
        std::cout << "   🔄 Total splits: " << split_count << std::endl;
        std::cout << "   📄 Páginas procesadas: " << table_pages.size() << std::endl;
        std::cout << "   🔍 Claves únicas: " << processed_keys.size() << std::endl;

        return hash_index;
    }

    /**
     * @brief ✅ FUNCIÓN CORREGIDA - Construye B+ Tree desde DiskManager
     */
    std::unique_ptr<BPlusTree<std::string>> buildBTreeIndexFromDisk(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🌲 CONSTRUYENDO B+ TREE DESDE DISKMANAGER" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "🔍 Fuente: DiskManager (NO CSV)" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;

        if (!disk_manager) {
            std::cout << "❌ Error: DiskManager no disponible" << std::endl;
            return nullptr;
        }

        auto btree_index = std::make_unique<BPlusTree<std::string>>(4);
        
        // ✅ Usar getTablePages() en lugar de CSV
        std::vector<PhysicalAddress> table_pages;
        if (!disk_manager->getTablePages(table_name, table_pages)) {
            std::cout << "❌ Error: No se pudieron obtener páginas de tabla " << table_name << std::endl;
            return btree_index;
        }

        std::cout << "📊 Páginas encontradas: " << table_pages.size() << std::endl;

        int records_processed = 0;
        std::unordered_set<std::string> processed_keys; // Prevenir duplicados

        // Recopilar todos los registros ordenados
        std::vector<std::pair<std::string, RecordReference>> sorted_records;

        // Iterar sobre cada página física
        for (const auto& page_addr : table_pages) {
            Block page_block(page_addr, 4096);
            if (!disk_manager->readBlock(page_addr, page_block)) {
                continue;
            }

            auto active_records = page_block.getActiveRecords();
            
            for (const auto& record : active_records) {
                std::string key = extractKeyFromRecord(record, key_field);
                if (key.empty() || processed_keys.find(key) != processed_keys.end()) {
                    continue;
                }
                processed_keys.insert(key);

                RecordReference record_ref = disk_manager->createRecordReference(page_addr, record->getId());
                record_ref.setCachedKey(key);
                
                sorted_records.emplace_back(key, record_ref);
            }
        }

        // Ordenar por clave para inserción eficiente en B+ Tree
        std::sort(sorted_records.begin(), sorted_records.end());

        // Insertar en B+ Tree
        for (const auto& pair : sorted_records) {
            if (max_records != -1 && records_processed >= max_records) {
                break;
            }

            if (btree_index->insertReference(pair.first, pair.second)) {
                records_processed++;
                
                if (records_processed % 500 == 0) {
                    std::cout << "📈 Procesados: " << records_processed << " registros" << std::endl;
                }
            }
        }

        std::cout << "\n✅ B+ TREE CONSTRUIDO:" << std::endl;
        std::cout << "   📊 Total registros: " << records_processed << std::endl;
        std::cout << "   📄 Páginas procesadas: " << table_pages.size() << std::endl;
        std::cout << "   🔍 Claves únicas: " << processed_keys.size() << std::endl;

        return btree_index;
    }

    // ============================================================================
    // PERSISTENCIA DE ÍNDICES
    // ============================================================================
    
    /**
     * @brief Guarda Hash Extensible en disco
     */
    bool saveHashIndex(const ExtensibleHash& hash_index, const std::string& index_name = "imei_index") {
        if (!enable_persistence) return false;

        std::cout << "💾 Guardando Hash Index: " << index_name << std::endl;

        std::string hash_file = index_data_path + "/hash_" + index_name + ".idx";

        try {
            std::ofstream file(hash_file);
            file << "HASH_INDEX_V1" << std::endl;
            file << "total_records=" << hash_index.getTotalRecords() << std::endl;
            file << "bucket_capacity=4" << std::endl;
            file << "END_METADATA" << std::endl;

            // Serializar estructura (implementación específica del ExtensibleHash)
            file << hash_index.serialize() << std::endl;
            file.close();

            // Guardar metadatos
            saveHashMetadata(index_name, hash_index.getTotalRecords());

            std::cout << "✅ Hash Index guardado exitosamente" << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cout << "❌ Error guardando Hash Index: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Carga Hash Extensible desde disco
     */
    std::unique_ptr<ExtensibleHash> loadHashIndex(const std::string& index_name = "imei_index") {
        if (!enable_persistence) return nullptr;

        std::cout << "📂 Cargando Hash Index: " << index_name << std::endl;

        std::string hash_file = index_data_path + "/hash_" + index_name + ".idx";

        if (!std::filesystem::exists(hash_file)) {
            std::cout << "⚠️ Archivo de índice no encontrado: " << hash_file << std::endl;
            return nullptr;
        }

        try {
            std::ifstream file(hash_file);
            std::string line;

            // Verificar formato
            std::getline(file, line);
            if (line != "HASH_INDEX_V1") {
                std::cout << "❌ Formato de archivo inválido" << std::endl;
                return nullptr;
            }

            // Leer metadatos
            int total_records = 0;
            int bucket_capacity = 4;

            while (std::getline(file, line) && line != "END_METADATA") {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);

                    if (key == "total_records") {
                        total_records = std::stoi(value);
                    } else if (key == "bucket_capacity") {
                        bucket_capacity = std::stoi(value);
                    }
                }
            }

            // Crear nuevo hash y deserializar
            auto hash_index = std::make_unique<ExtensibleHash>(bucket_capacity);

            std::stringstream remaining_content;
            while (std::getline(file, line)) {
                remaining_content << line << std::endl;
            }

            if (hash_index->deserialize(remaining_content.str())) {
                std::cout << "✅ Hash Index cargado: " << total_records << " registros" << std::endl;
                return hash_index;
            } else {
                std::cout << "❌ Error deserializando Hash Index" << std::endl;
                return nullptr;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Error cargando Hash Index: " << e.what() << std::endl;
            return nullptr;
        }
    }

    /**
     * @brief Guarda B+ Tree en disco
     */
    bool saveBTreeIndex(const BPlusTree<std::string>& btree_index, const std::string& index_name = "timestamp_index") {
        if (!enable_persistence) return false;

        std::cout << "💾 Guardando B+ Tree Index: " << index_name << std::endl;

        std::string btree_file = index_data_path + "/btree_" + index_name + ".idx";

        try {
            std::ofstream file(btree_file);
            file << "BTREE_INDEX_V1" << std::endl;
            file << "order=4" << std::endl;
            file << "total_records=" << btree_index.size() << std::endl;
            file << "END_METADATA" << std::endl;

            // Serializar B+ Tree
            file << btree_index.serialize() << std::endl;
            file.close();

            std::cout << "✅ B+ Tree Index guardado exitosamente" << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cout << "❌ Error guardando B+ Tree Index: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Carga B+ Tree desde disco
     */
    std::unique_ptr<BPlusTree<std::string>> loadBTreeIndex(const std::string& index_name = "timestamp_index") {
        if (!enable_persistence) return nullptr;

        std::cout << "📂 Cargando B+ Tree Index: " << index_name << std::endl;

        std::string btree_file = index_data_path + "/btree_" + index_name + ".idx";

        if (!std::filesystem::exists(btree_file)) {
            std::cout << "⚠️ Archivo de índice no encontrado: " << btree_file << std::endl;
            return nullptr;
        }

        try {
            auto btree_index = std::make_unique<BPlusTree<std::string>>(4);

            std::ifstream file(btree_file);
            std::stringstream content;
            std::string line;

            // Saltar metadatos
            while (std::getline(file, line) && line != "END_METADATA") {}

            // Leer contenido serializado
            while (std::getline(file, line)) {
                content << line << std::endl;
            }

            if (btree_index->deserialize(content.str())) {
                std::cout << "✅ B+ Tree Index cargado exitosamente" << std::endl;
                return btree_index;
            } else {
                std::cout << "❌ Error deserializando B+ Tree Index" << std::endl;
                return nullptr;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Error cargando B+ Tree Index: " << e.what() << std::endl;
            return nullptr;
        }
    }

    // ============================================================================
    // UTILIDADES PRIVADAS
    // ============================================================================

private:
    /**
     * @brief Extrae clave de un registro según el campo especificado
     */
    std::string extractKeyFromRecord(const std::shared_ptr<Record>& record, const std::string& key_field) {
        if (!record) return "";

        // Para VariableRecord (GPS data)
        if (auto var_record = std::dynamic_pointer_cast<VariableRecord>(record)) {
            auto values = var_record->getFieldValues();
            
            // Mapeo de campos GPS (basado en estructura conocida)
            std::unordered_map<std::string, int> field_mapping = {
                {"imei", 1},        // IMEI en posición 1
                {"timestamp", 2},   // Timestamp en posición 2
                {"latitude", 3},    // Latitude en posición 3
                {"longitude", 4}    // Longitude en posición 4
            };

            auto it = field_mapping.find(key_field);
            if (it != field_mapping.end() && it->second < static_cast<int>(values.size())) {
                return values[it->second];
            }
        }

        return "";
    }

    /**
     * @brief Guarda metadatos de índice
     */
    void saveHashMetadata(const std::string& index_name, size_t record_count) {
        std::string metadata_file = metadata_path + "/index_metadata.txt";
        std::ofstream file(metadata_file, std::ios::app);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        file << index_name << "|HASH|" << record_count << "|" << time_t << std::endl;
        file.close();
    }

public:
    /**
     * @brief Información de índices disponibles
     */
    void displayIndexInfo() const {
        std::cout << "\n📊 INFORMACIÓN DE ÍNDICES:" << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << "Ruta de índices: " << index_data_path << std::endl;
        
        if (std::filesystem::exists(index_data_path)) {
            std::cout << "Índices disponibles:" << std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(index_data_path)) {
                if (entry.is_regular_file()) {
                    std::cout << "  📄 " << entry.path().filename() << std::endl;
                }
            }
        } else {
            std::cout << "📁 Directorio de índices no existe" << std::endl;
        }
    }
};

#endif // INDEX_MANAGER_H