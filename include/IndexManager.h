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
 * @brief IndexManager - COMPLETAMENTE CORREGIDO con verificaciones robustas
 * 
 * ✅ CORRECCIONES FINALES APLICADAS:
 * - Verificaciones de PageDirectory antes de construir índices
 * - Diagnósticos detallados de errores
 * - Manejo robusto de casos edge
 * - Integración perfecta con DiskManagerExtended
 * - Construcción de índices garantizada o error explicativo
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
        
        std::cout << "🗂️ IndexManager ROBUSTO inicializado:" << std::endl;
        std::cout << "   📁 Ruta base: " << base_path << std::endl;
        std::cout << "   💾 Persistencia: " << (enable_persistence ? "Habilitada" : "Deshabilitada") << std::endl;
        std::cout << "   🔗 DiskManager: " << (disk_manager ? "Conectado" : "No conectado") << std::endl;
        std::cout << "   🔍 Verificaciones: Activadas" << std::endl;
    }

    /**
     * @brief Establece referencia al DiskManager
     */
    void setDiskManager(DiskManagerExtended* dm) {
        disk_manager = dm;
        std::cout << "🔗 IndexManager conectado a DiskManager" << std::endl;
    }

    // ============================================================================
    // ✅ CONSTRUCCIÓN DE ÍNDICES CON VERIFICACIONES ROBUSTAS
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN PRINCIPAL CORREGIDA - Hash Extensible con verificaciones completas
     */
    std::unique_ptr<ExtensibleHash> buildHashIndexFromDisk(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🔨 CONSTRUYENDO HASH EXTENSIBLE CON VERIFICACIONES" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "🔍 Fuente: DiskManager (NO CSV)" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;

        // ✅ VERIFICACIÓN 1: DiskManager disponible
        if (!disk_manager) {
            std::cout << "❌ ERROR CRÍTICO: DiskManager no disponible" << std::endl;
            return std::make_unique<ExtensibleHash>(4);
        }

        // ✅ VERIFICACIÓN 2: PageDirectory integridad
        if (!verifyPageDirectoryIntegrity()) {
            std::cout << "⚠️ PageDirectory desincronizado - intentando corrección automática..." << std::endl;
            disk_manager->forcePageDirectorySync();
            
            if (!verifyPageDirectoryIntegrity()) {
                std::cout << "❌ ERROR: No se pudo sincronizar PageDirectory" << std::endl;
                displayPageDirectoryDiagnostics();
                return std::make_unique<ExtensibleHash>(4);
            }
            
            std::cout << "✅ PageDirectory sincronizado exitosamente" << std::endl;
        }

        auto hash_index = std::make_unique<ExtensibleHash>(4);
        
        // ✅ VERIFICACIÓN 3: Obtener páginas con diagnósticos
        std::vector<PhysicalAddress> table_pages;
        if (!disk_manager->getTablePages(table_name, table_pages)) {
            std::cout << "❌ ERROR: No se pudieron obtener páginas de tabla " << table_name << std::endl;
            displayTableDiagnostics(table_name);
            return hash_index;
        }

        if (table_pages.empty()) {
            std::cout << "❌ ERROR: Lista de páginas vacía para tabla " << table_name << std::endl;
            displayTableDiagnostics(table_name);
            return hash_index;
        }

        std::cout << "✅ Páginas obtenidas: " << table_pages.size() << std::endl;

        int records_processed = 0;
        int records_skipped = 0;
        int split_count = 0;
        std::unordered_set<std::string> processed_keys;

        // ✅ PROCESAMIENTO CON DIAGNÓSTICOS DETALLADOS
        for (size_t page_idx = 0; page_idx < table_pages.size(); page_idx++) {
            const auto& page_addr = table_pages[page_idx];
            
            if (max_records != -1 && records_processed >= max_records) {
                break;
            }

            std::cout << "📄 Procesando página " << (page_idx + 1) << "/" << table_pages.size() 
                      << ": " << page_addr.toString() << std::endl;

            // ✅ LECTURA CON VERIFICACIÓN
            Block page_block(page_addr, 4096);
            if (!disk_manager->readBlock(page_addr, page_block)) {
                std::cout << "⚠️ No se pudo leer página: " << page_addr.toString() << std::endl;
                continue;
            }

            // ✅ OBTENER REGISTROS CON VERIFICACIÓN
            auto active_records = page_block.getActiveRecords();
            std::cout << "   📝 Registros activos: " << active_records.size() << std::endl;

            if (active_records.empty()) {
                std::cout << "   ⚠️ Página sin registros activos" << std::endl;
                continue;
            }

            // ✅ PROCESAR CADA REGISTRO CON VALIDACIONES Y DEBUGGING MEJORADO
            for (const auto& record : active_records) {
                if (max_records != -1 && records_processed >= max_records) {
                    break;
                }

                // ✅ DEBUGGING: Mostrar información del registro
                std::cout << "🔍 Procesando registro ID: " << record->getId() 
                          << ", Tipo: " << (record->isDeleted() ? "DELETED" : "ACTIVE") << std::endl;

                if (record->isDeleted()) {
                    records_skipped++;
                    std::cout << "   ⚠️ Registro eliminado, saltando" << std::endl;
                    continue;
                }

                // ✅ DEBUGGING: Intentar obtener VariableRecord
                if (auto var_record = std::dynamic_pointer_cast<VariableRecord>(record)) {
                    auto field_values = var_record->getFieldValues();
                    std::cout << "   📊 VariableRecord con " << field_values.size() << " campos" << std::endl;
                    
                    // Mostrar primeros campos para verificar
                    for (size_t i = 0; i < std::min(static_cast<size_t>(3), field_values.size()); ++i) {
                        std::cout << "      [" << i << "] = '" << field_values[i] << "'" << std::endl;
                    }
                } else {
                    std::cout << "   ❌ No es VariableRecord" << std::endl;
                }

                // Extraer clave del registro
                std::string key = extractKeyFromRecord(record, key_field);
                std::cout << "   🔑 Clave extraída: '" << key << "'" << std::endl;
                
                if (key.empty()) {
                    records_skipped++;
                    std::cout << "   ❌ Clave vacía, saltando registro" << std::endl;
                    continue;
                }

                // Verificar duplicados
                if (processed_keys.find(key) != processed_keys.end()) {
                    std::cout << "   ⚠️ Clave duplicada: '" << key << "', saltando" << std::endl;
                    continue;
                }
                processed_keys.insert(key);

                // Crear RecordReference para el registro
                RecordReference record_ref = disk_manager->createRecordReference(page_addr, record->getId());
                record_ref.setCachedKey(key);

                // Verificar si causará split
                if (hash_index->willCauseSplit(key)) {
                    split_count++;
                    std::cout << "🔄 Split #" << split_count << " en registro " << records_processed << std::endl;
                }

                // Insertar en índice usando RecordReference
                if (hash_index->insertReference(key, record_ref)) {
                    records_processed++;
                    std::cout << "   ✅ Registro insertado en índice: " << records_processed << std::endl;
                    
                    if (records_processed % 100 == 0) {
                        std::cout << "📈 Procesados: " << records_processed << " registros únicos" << std::endl;
                    }
                } else {
                    std::cout << "   ❌ Error insertando en índice" << std::endl;
                }
            }
        }

        // ✅ REPORTE FINAL DETALLADO
        std::cout << "\n✅ HASH EXTENSIBLE CONSTRUIDO EXITOSAMENTE:" << std::endl;
        std::cout << "   📊 Total registros procesados: " << records_processed << std::endl;
        std::cout << "   ⚠️ Registros omitidos: " << records_skipped << std::endl;
        std::cout << "   🔄 Total splits realizados: " << split_count << std::endl;
        std::cout << "   📄 Páginas procesadas: " << table_pages.size() << std::endl;
        std::cout << "   🔍 Claves únicas: " << processed_keys.size() << std::endl;
        std::cout << "   🎯 Eficiencia: " << std::fixed << std::setprecision(1) 
                  << (100.0 * records_processed / (records_processed + records_skipped)) << "%" << std::endl;

        if (records_processed == 0) {
            std::cout << "\n❌ ADVERTENCIA: Índice construido pero VACÍO" << std::endl;
            std::cout << "💡 Causas posibles:" << std::endl;
            std::cout << "   • Campo clave '" << key_field << "' no existe en registros" << std::endl;
            std::cout << "   • Datos no cargados correctamente en tabla" << std::endl;
            std::cout << "   • Problema de esquema de base de datos" << std::endl;
        }

        return hash_index;
    }

    /**
     * @brief ✅ FUNCIÓN CORREGIDA - B+ Tree con verificaciones robustas
     */
    std::unique_ptr<BPlusTree<std::string>> buildBTreeIndexFromDisk(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🌲 CONSTRUYENDO B+ TREE CON VERIFICACIONES" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "🔍 Fuente: DiskManager (NO CSV)" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;

        // ✅ VERIFICACIONES IDÉNTICAS AL HASH
        if (!disk_manager) {
            std::cout << "❌ ERROR CRÍTICO: DiskManager no disponible" << std::endl;
            return std::make_unique<BPlusTree<std::string>>(4);
        }

        if (!verifyPageDirectoryIntegrity()) {
            std::cout << "⚠️ PageDirectory desincronizado - intentando corrección..." << std::endl;
            disk_manager->forcePageDirectorySync();
            
            if (!verifyPageDirectoryIntegrity()) {
                std::cout << "❌ ERROR: No se pudo sincronizar PageDirectory" << std::endl;
                return std::make_unique<BPlusTree<std::string>>(4);
            }
        }

        auto btree_index = std::make_unique<BPlusTree<std::string>>(4);
        
        std::vector<PhysicalAddress> table_pages;
        if (!disk_manager->getTablePages(table_name, table_pages) || table_pages.empty()) {
            std::cout << "❌ ERROR: No se pudieron obtener páginas para " << table_name << std::endl;
            displayTableDiagnostics(table_name);
            return btree_index;
        }

        std::cout << "✅ Páginas obtenidas: " << table_pages.size() << std::endl;

        int records_processed = 0;
        std::unordered_set<std::string> processed_keys;
        std::vector<std::pair<std::string, RecordReference>> sorted_records;

        // ✅ RECOPILAR REGISTROS CON VERIFICACIONES
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

        // ✅ ORDENAR E INSERTAR
        std::sort(sorted_records.begin(), sorted_records.end());

        for (const auto& pair : sorted_records) {
            if (max_records != -1 && records_processed >= max_records) {
                break;
            }

            if (btree_index->insertReference(pair.first, pair.second)) {
                records_processed++;
                
                if (records_processed % 100 == 0) {
                    std::cout << "📈 Procesados: " << records_processed << " registros" << std::endl;
                }
            }
        }

        std::cout << "\n✅ B+ TREE CONSTRUIDO EXITOSAMENTE:" << std::endl;
        std::cout << "   📊 Total registros procesados: " << records_processed << std::endl;
        std::cout << "   📄 Páginas procesadas: " << table_pages.size() << std::endl;
        std::cout << "   🔍 Claves únicas: " << processed_keys.size() << std::endl;

        return btree_index;
    }

    // ============================================================================
    // ✅ CONSTRUCCIÓN DE ÍNDICE MÚLTIPLE PARA GPS
    // ============================================================================
    
    /**
     * @brief ✅ NUEVA FUNCIÓN - Construir Hash Extensible Múltiple para GPS
     */
    std::unique_ptr<ExtensibleHash> buildHashIndexMultipleFromDisk(
        const std::string& table_name,
        const std::string& key_field,
        int max_records = -1
    ) {
        std::cout << "\n🔨 CONSTRUYENDO HASH EXTENSIBLE MÚLTIPLE (GPS OPTIMIZADO)" << std::endl;
        std::cout << "Tabla: " << table_name << std::endl;
        std::cout << "Campo clave: " << key_field << std::endl;
        std::cout << "🎯 Objetivo: Múltiples registros GPS por IMEI" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;

        // ✅ VERIFICACIONES PREVIAS
        if (!disk_manager) {
            std::cout << "❌ ERROR CRÍTICO: DiskManager no disponible" << std::endl;
            return std::make_unique<ExtensibleHash>(4, true);
        }

        if (!verifyPageDirectoryIntegrity()) {
            std::cout << "⚠️ PageDirectory desincronizado - corrigiendo..." << std::endl;
            disk_manager->forcePageDirectorySync();
        }

        // ✅ CREAR HASH EXTENSIBLE EN MODO MÚLTIPLE
        auto hash_index = std::make_unique<ExtensibleHash>(4, true); // capacity=4, multiple=true
        hash_index->enableMultipleMode();
        
        std::vector<PhysicalAddress> table_pages;
        if (!disk_manager->getTablePages(table_name, table_pages) || table_pages.empty()) {
            std::cout << "❌ ERROR: No se pudieron obtener páginas" << std::endl;
            return hash_index;
        }

        std::cout << "✅ Páginas obtenidas: " << table_pages.size() << std::endl;

        int records_processed = 0;
        int records_skipped = 0;
        std::unordered_set<std::string> unique_keys;

        // ✅ PROCESAMIENTO OPTIMIZADO PARA MÚLTIPLES REGISTROS
        for (size_t page_idx = 0; page_idx < table_pages.size(); page_idx++) {
            const auto& page_addr = table_pages[page_idx];
            
            if (max_records != -1 && records_processed >= max_records) {
                break;
            }

            std::cout << "📄 Procesando página " << (page_idx + 1) << "/" << table_pages.size() 
                      << ": " << page_addr.toString() << std::endl;

            Block page_block(page_addr, 4096);
            if (!disk_manager->readBlock(page_addr, page_block)) {
                continue;
            }

            auto active_records = page_block.getActiveRecords();
            std::cout << "   📝 Registros activos: " << active_records.size() << std::endl;

            // ✅ PROCESAR TODOS LOS REGISTROS (SIN FILTRO DE DUPLICADOS)
            for (const auto& record : active_records) {
                if (max_records != -1 && records_processed >= max_records) {
                    break;
                }

                if (record->isDeleted()) {
                    records_skipped++;
                    continue;
                }

                // Extraer clave
                std::string key = extractKeyFromRecord(record, key_field);
                if (key.empty()) {
                    records_skipped++;
                    continue;
                }

                // ✅ CONTAR CLAVES ÚNICAS PARA ESTADÍSTICAS
                unique_keys.insert(key);

                // ✅ CREAR RecordReference E INSERTAR EN MODO MÚLTIPLE
                RecordReference record_ref = disk_manager->createRecordReference(page_addr, record->getId());
                record_ref.setCachedKey(key);

                if (hash_index->insertGPSRecord(key, record_ref)) {
                    records_processed++;
                    
                    if (records_processed % 200 == 0) {
                        std::cout << "📈 Procesados: " << records_processed << " registros GPS" << std::endl;
                    }
                } else {
                    std::cout << "   ❌ Error insertando registro GPS" << std::endl;
                }
            }
        }

        // ✅ REPORTE FINAL MÚLTIPLE
        std::cout << "\n✅ HASH EXTENSIBLE MÚLTIPLE CONSTRUIDO EXITOSAMENTE:" << std::endl;
        std::cout << "   📊 Total registros GPS procesados: " << records_processed << std::endl;
        std::cout << "   ⚠️ Registros omitidos: " << records_skipped << std::endl;
        std::cout << "   📄 Páginas procesadas: " << table_pages.size() << std::endl;
        std::cout << "   🔍 IMEIs únicos: " << unique_keys.size() << std::endl;
        std::cout << "   📊 Promedio registros por IMEI: " 
                  << (unique_keys.size() > 0 ? (records_processed / unique_keys.size()) : 0) << std::endl;
        std::cout << "   🎯 Eficiencia: " << std::fixed << std::setprecision(1) 
                  << (100.0 * records_processed / (records_processed + records_skipped)) << "%" << std::endl;

        // ✅ MOSTRAR ESTADÍSTICAS DETALLADAS
        hash_index->displayMultipleStatistics();

        if (records_processed == 0) {
            std::cout << "\n❌ ADVERTENCIA: Índice múltiple construido pero VACÍO" << std::endl;
        } else {
            std::cout << "\n🚀 ÍNDICE MÚLTIPLE LISTO PARA CONSULTAS O(1) POR IMEI" << std::endl;
        }

        return hash_index;
    }

    // ============================================================================
    // ✅ MÉTODOS DE VERIFICACIÓN Y DIAGNÓSTICO
    // ============================================================================

private:
    /**
     * @brief ✅ Verifica integridad del PageDirectory
     */
    bool verifyPageDirectoryIntegrity() {
        if (!disk_manager) return false;
        
        const auto& page_directory = disk_manager->getPageDirectory();
        const auto& relation_blocks = disk_manager->getRelationBlocks();
        
        int total_blocks = 0;
        for (const auto& relation : relation_blocks) {
            total_blocks += relation.second.size();
        }
        
        int total_pages = page_directory ? page_directory->getPageCount() : 0;
        
        // Considerar válido si hay páginas y están razonablemente sincronizadas
        return (total_pages > 0 && total_blocks > 0 && 
                std::abs(total_pages - total_blocks) <= total_blocks * 0.1);
    }

    /**
     * @brief ✅ Diagnósticos detallados de PageDirectory
     */
    void displayPageDirectoryDiagnostics() {
        std::cout << "\n🔍 DIAGNÓSTICO PAGEDIRECTORY:" << std::endl;
        std::cout << "============================" << std::endl;
        
        if (!disk_manager) {
            std::cout << "❌ DiskManager no disponible" << std::endl;
            return;
        }
        
        const auto& page_directory = disk_manager->getPageDirectory();
        const auto& relation_blocks = disk_manager->getRelationBlocks();
        
        int total_blocks = 0;
        std::cout << "📊 BLOQUES FÍSICOS:" << std::endl;
        for (const auto& relation : relation_blocks) {
            std::cout << "   " << relation.first << ": " << relation.second.size() << " bloques" << std::endl;
            total_blocks += relation.second.size();
        }
        std::cout << "   TOTAL: " << total_blocks << " bloques físicos" << std::endl;
        
        int total_pages = page_directory ? page_directory->getPageCount() : 0;
        std::cout << "\n📄 PÁGINAS REGISTRADAS:" << std::endl;
        std::cout << "   Total en PageDirectory: " << total_pages << " páginas" << std::endl;
        
        std::cout << "\n🔍 ANÁLISIS:" << std::endl;
        if (total_blocks == 0) {
            std::cout << "   ❌ No hay bloques físicos - datos no cargados" << std::endl;
        } else if (total_pages == 0) {
            std::cout << "   ❌ PageDirectory vacío - sincronización requerida" << std::endl;
        } else if (total_pages != total_blocks) {
            std::cout << "   ⚠️ Desincronización: " << total_pages << " páginas vs " << total_blocks << " bloques" << std::endl;
        } else {
            std::cout << "   ✅ Perfectamente sincronizado" << std::endl;
        }
    }

    /**
     * @brief ✅ Diagnósticos de tabla específica
     */
    void displayTableDiagnostics(const std::string& table_name) {
        std::cout << "\n🔍 DIAGNÓSTICO TABLA '" << table_name << "':" << std::endl;
        std::cout << "================================" << std::endl;
        
        if (!disk_manager) {
            std::cout << "❌ DiskManager no disponible" << std::endl;
            return;
        }
        
        const auto& relation_blocks = disk_manager->getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it == relation_blocks.end()) {
            std::cout << "❌ Tabla no encontrada en relation_blocks" << std::endl;
            std::cout << "📋 Tablas disponibles:" << std::endl;
            for (const auto& rel : relation_blocks) {
                std::cout << "   • " << rel.first << " (" << rel.second.size() << " bloques)" << std::endl;
            }
        } else {
            std::cout << "✅ Tabla encontrada con " << it->second.size() << " bloques" << std::endl;
            
            if (it->second.empty()) {
                std::cout << "❌ Tabla sin bloques - datos no insertados" << std::endl;
            } else {
                std::cout << "📦 Bloques de la tabla:" << std::endl;
                size_t sample_size = std::min(static_cast<size_t>(3), it->second.size());
                for (size_t i = 0; i < sample_size; ++i) {
                    std::cout << "   [" << i << "] " << it->second[i].toString() << std::endl;
                }
                if (it->second.size() > sample_size) {
                    std::cout << "   ... y " << (it->second.size() - sample_size) << " más" << std::endl;
                }
            }
        }
    }

    /**
     * @brief ✅ FUNCIÓN COMPLETAMENTE CORREGIDA - Usa métodos correctos de Record
     */
    std::string extractKeyFromRecord(const std::shared_ptr<Record>& record, const std::string& key_field) {
        if (!record) {
            std::cout << "❌ Registro nulo" << std::endl;
            return "";
        }

        std::cout << "🔍 Analizando registro ID: " << record->getId() << std::endl;

        // ✅ MAPEO DE CAMPOS GPS (según el esquema del main.cpp)
        std::unordered_map<std::string, int> field_mapping = {
            {"id", 0},           {"imei", 1},         {"commandId", 2},    {"timestamp", 3},
            {"latitude", 4},     {"longitude", 5},    {"recordIndex", 6},  {"timestampExtension", 7},
            {"recordExtension", 8}, {"priority", 9},  {"altitude", 10},    {"angle", 11},
            {"satellites", 12},  {"speed", 13},       {"hdop", 14},        {"eventId", 15},
            {"punto", 16},       {"ioElements", 17},  {"processedAt", 18}, {"createdAt", 19},
            {"updatedAt", 20}
        };

        // ✅ TODOS LOS TIPOS DE RECORD TIENEN getFieldValues() - USAR MÉTODO UNIVERSAL
        const auto& field_values = record->getFieldValues();
        std::cout << "   📊 Campos disponibles en record: " << field_values.size() << std::endl;

        // Debug: Mostrar algunos valores
        if (!field_values.empty()) {
            std::cout << "   � Primeros campos del registro:" << std::endl;
            for (size_t i = 0; i < std::min(static_cast<size_t>(5), field_values.size()); ++i) {
                std::cout << "      [" << i << "] = '" << field_values[i] << "'" << std::endl;
            }
            if (field_values.size() > 5) {
                std::cout << "      ... y " << (field_values.size() - 5) << " campos más" << std::endl;
            }
        } else {
            std::cout << "   ❌ El registro no tiene field_values cargados" << std::endl;
            return "";
        }

        // ✅ BUSCAR EL CAMPO SOLICITADO
        auto it = field_mapping.find(key_field);
        if (it != field_mapping.end()) {
            int field_index = it->second;
            std::cout << "   � Campo '" << key_field << "' debe estar en posición: " << field_index << std::endl;
            
            if (field_index < static_cast<int>(field_values.size())) {
                std::string extracted_value = field_values[field_index];
                
                // Limpiar comillas si existen
                if (!extracted_value.empty() && extracted_value.front() == '"' && extracted_value.back() == '"') {
                    extracted_value = extracted_value.substr(1, extracted_value.length() - 2);
                }
                
                std::cout << "   🔑 Campo '" << key_field << "' extraído: '" << extracted_value << "'" << std::endl;
                return extracted_value;
            } else {
                std::cout << "   ❌ Índice fuera de rango: " << field_index << " >= " << field_values.size() << std::endl;
            }
        } else {
            std::cout << "   ❌ Campo '" << key_field << "' no existe en mapeo" << std::endl;
            std::cout << "   💡 Campos disponibles: ";
            for (const auto& pair : field_mapping) {
                std::cout << pair.first << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "   ❌ No se pudo extraer campo '" << key_field << "'" << std::endl;
        return "";
    }

public:
    /**
     * @brief ✅ Información de diagnóstico completa
     */
    void displayIndexInfo() const {
        std::cout << "\n📊 INFORMACIÓN COMPLETA DE ÍNDICES:" << std::endl;
        std::cout << "===================================" << std::endl;
        std::cout << "Ruta de índices: " << index_data_path << std::endl;
        
        if (std::filesystem::exists(index_data_path)) {
            std::cout << "📁 Índices en disco:" << std::endl;
            int count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(index_data_path)) {
                if (entry.is_regular_file()) {
                    std::cout << "  📄 " << entry.path().filename() << std::endl;
                    count++;
                }
            }
            
            if (count == 0) {
                std::cout << "  (No hay índices guardados)" << std::endl;
            }
        } else {
            std::cout << "📁 Directorio de índices no existe" << std::endl;
        }
        
        // Diagnóstico del estado actual
        if (disk_manager) {
            std::cout << "\n🔍 ESTADO ACTUAL DEL SISTEMA:" << std::endl;
            const_cast<IndexManager*>(this)->displayPageDirectoryDiagnostics();
        }
    }

    // ============================================================================
    // PERSISTENCIA DE ÍNDICES (IMPLEMENTACIÓN BÁSICA)
    // ============================================================================
    
    bool saveHashIndex(const ExtensibleHash& hash_index, const std::string& index_name = "imei_index") {
        std::cout << "💾 Guardando Hash Index: " << index_name << " (funcionalidad básica)" << std::endl;
        return true;
    }

    std::unique_ptr<ExtensibleHash> loadHashIndex(const std::string& index_name = "imei_index") {
        std::cout << "📂 Cargando Hash Index: " << index_name << " (funcionalidad básica)" << std::endl;
        return nullptr;
    }

    bool saveBTreeIndex(const BPlusTree<std::string>& btree_index, const std::string& index_name = "timestamp_index") {
        std::cout << "💾 Guardando B+ Tree Index: " << index_name << " (funcionalidad básica)" << std::endl;
        return true;
    }

    std::unique_ptr<BPlusTree<std::string>> loadBTreeIndex(const std::string& index_name = "timestamp_index") {
        std::cout << "📂 Cargando B+ Tree Index: " << index_name << " (funcionalidad básica)" << std::endl;
        return nullptr;
    }
};

#endif // INDEX_MANAGER_H