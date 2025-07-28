#include "../include/IndexManager.h"
#include "../include/DiskManagerExtended.h"
#include "../include/Block.h"
#include "../include/Record.h"
#include "../include/VariableRecord.h"
#include <iostream>
#include <set>

// ============================================================================
// IMPLEMENTACIÓN CORREGIDA - CONSTRUYE HASH EXTENSIBLE DESDE DISCO
// ============================================================================

std::unique_ptr<ExtensibleHash> IndexManager::buildHashIndex(
    const std::string& table_name,
    const std::string& key_field,
    int max_records
) {
    std::cout << "\n🔨 CONSTRUYENDO HASH EXTENSIBLE DESDE DATOS EN DISCO" << std::endl;
    std::cout << "Tabla: " << table_name << std::endl;
    std::cout << "Campo clave: " << key_field << std::endl;
    std::cout << "Fuente: DiskManager (datos almacenados)" << std::endl;
    std::cout << "=" << std::string(50, '=') << std::endl;

    if (!disk_manager_ref) {
        std::cout << "❌ ERROR: DiskManager no disponible" << std::endl;
        std::cout << "💡 Asegúrese de llamar setDiskManagerReference() primero" << std::endl;
        return nullptr;
    }

    auto hash_index = std::make_unique<ExtensibleHash>(4); // Bucket capacity = 4
    
    try {
        // PASO 1: Obtener páginas de la tabla desde DiskManager
        std::cout << "📋 Obteniendo páginas de la tabla '" << table_name << "'..." << std::endl;
        auto table_pages = disk_manager_ref->getTablePages(table_name);
        
        if (table_pages.empty()) {
            std::cout << "⚠️ No se encontraron páginas para la tabla '" << table_name << "'" << std::endl;
            return hash_index;
        }

        std::cout << "📄 Páginas encontradas: " << table_pages.size() << std::endl;

        // PASO 2: Configurar campo de clave (asumiendo que IMEI está en posición 1)
        int key_field_index = 1; // IMEI en el campo 1 para GPS
        if (key_field == "id") {
            key_field_index = 0;
        } else if (key_field == "timestamp") {
            key_field_index = 3;
        }

        std::cout << "🔑 Campo clave: " << key_field << " (índice: " << key_field_index << ")" << std::endl;

        // PASO 3: Iterar sobre todas las páginas y procesar registros
        int records_processed = 0;
        int duplicate_count = 0;
        int split_count = 0;
        std::set<std::string> processed_keys; // Para detectar duplicados

        for (const auto& page_addr : table_pages) {
            if (max_records != -1 && records_processed >= max_records) {
                std::cout << "🛑 Límite de registros alcanzado: " << max_records << std::endl;
                break;
            }

            // Leer bloque desde disco
            Block block(page_addr, disk_manager_ref->getDiskConfig().getBytesPerSector());
            if (!disk_manager_ref->readBlock(page_addr, block)) {
                std::cout << "❌ Error leyendo bloque: " << page_addr << std::endl;
                continue;
            }

            // Verificar que el bloque pertenece a la tabla correcta
            if (block.getRelationName() != table_name) {
                continue; // Saltar bloques de otras tablas
            }

            std::cout << "📖 Procesando bloque: " << page_addr 
                      << " (registros: " << block.getRecordCount() << ")" << std::endl;

            // PASO 4: Procesar cada registro del bloque
            auto records = block.getActiveRecords();
            for (const auto& record : records) {
                if (max_records != -1 && records_processed >= max_records) {
                    break;
                }

                // Obtener campos del registro
                auto field_values = record->getFieldValues();
                
                if (field_values.size() <= static_cast<size_t>(key_field_index)) {
                    std::cout << "⚠️ Registro sin campo clave suficiente" << std::endl;
                    continue;
                }

                std::string key = field_values[key_field_index];
                
                // VERIFICACIÓN DE DUPLICADOS
                if (processed_keys.find(key) != processed_keys.end()) {
                    duplicate_count++;
                    if (duplicate_count <= 5) { // Mostrar solo los primeros 5
                        std::cout << "   ⚠️ Clave duplicada: " << key.substr(0, 15) << "..." << std::endl;
                    }
                    continue; // Saltar duplicados
                }
                
                processed_keys.insert(key);

                // Verificar si causará split
                if (hash_index->willCauseSplit(key)) {
                    split_count++;
                    std::cout << "🔄 Split #" << split_count << " en registro " << records_processed << std::endl;
                }

                // Crear RecordReference para este registro
                RecordReference record_ref(page_addr, record->getId());
                
                // Insertar en índice usando RecordReference
                if (hash_index->insertReference(key, record_ref)) {
                    records_processed++;
                    
                    if (records_processed % 100 == 0) {
                        std::cout << "📈 Procesados: " << records_processed << " registros únicos" << std::endl;
                    }
                } else {
                    std::cout << "❌ Error insertando en hash: " << key << std::endl;
                }
            }
        }

        // PASO 5: Mostrar estadísticas finales
        std::cout << "\n✅ HASH EXTENSIBLE CONSTRUIDO:" << std::endl;
        std::cout << "   • Registros procesados: " << records_processed << std::endl;
        std::cout << "   • Duplicados omitidos: " << duplicate_count << std::endl;
        std::cout << "   • Splits realizados: " << split_count << std::endl;
        std::cout << "   • Profundidad global final: " << hash_index->getGlobalDepth() << std::endl;

        return hash_index;

    } catch (const std::exception& e) {
        std::cout << "❌ ERROR en buildHashIndex: " << e.what() << std::endl;
        return nullptr;
    }
}

// ============================================================================
// IMPLEMENTACIÓN CORREGIDA - CONSTRUYE B+ TREE DESDE DISCO
// ============================================================================

std::unique_ptr<BPlusTree<std::string>> IndexManager::buildBTreeIndex(
    const std::string& table_name,
    const std::string& key_field,
    int max_records
) {
    std::cout << "\n🌳 CONSTRUYENDO B+ TREE DESDE DATOS EN DISCO" << std::endl;
    std::cout << "Tabla: " << table_name << std::endl;
    std::cout << "Campo clave: " << key_field << std::endl;
    std::cout << "Fuente: DiskManager (datos almacenados)" << std::endl;
    std::cout << "=" << std::string(50, '=') << std::endl;

    if (!disk_manager_ref) {
        std::cout << "❌ ERROR: DiskManager no disponible" << std::endl;
        std::cout << "💡 Asegúrese de llamar setDiskManagerReference() primero" << std::endl;
        return nullptr;
    }

    auto btree_index = std::make_unique<BPlusTree<std::string>>(4); // Order = 4
    
    try {
        // PASO 1: Obtener páginas de la tabla desde DiskManager
        std::cout << "📋 Obteniendo páginas de la tabla '" << table_name << "'..." << std::endl;
        auto table_pages = disk_manager_ref->getTablePages(table_name);
        
        if (table_pages.empty()) {
            std::cout << "⚠️ No se encontraron páginas para la tabla '" << table_name << "'" << std::endl;
            return btree_index;
        }

        std::cout << "📄 Páginas encontradas: " << table_pages.size() << std::endl;

        // PASO 2: Configurar campo de clave
        int key_field_index = 3; // Timestamp en el campo 3 para GPS
        if (key_field == "id") {
            key_field_index = 0;
        } else if (key_field == "imei") {
            key_field_index = 1;
        }

        std::cout << "🔑 Campo clave: " << key_field << " (índice: " << key_field_index << ")" << std::endl;

        // PASO 3: Iterar sobre todas las páginas y procesar registros
        int records_processed = 0;
        int duplicate_count = 0;
        std::set<std::string> processed_keys; // Para detectar duplicados

        for (const auto& page_addr : table_pages) {
            if (max_records != -1 && records_processed >= max_records) {
                std::cout << "🛑 Límite de registros alcanzado: " << max_records << std::endl;
                break;
            }

            // Leer bloque desde disco
            Block block(page_addr, disk_manager_ref->getDiskConfig().getBytesPerSector());
            if (!disk_manager_ref->readBlock(page_addr, block)) {
                std::cout << "❌ Error leyendo bloque: " << page_addr << std::endl;
                continue;
            }

            // Verificar que el bloque pertenece a la tabla correcta
            if (block.getRelationName() != table_name) {
                continue; // Saltar bloques de otras tablas
            }

            std::cout << "📖 Procesando bloque: " << page_addr 
                      << " (registros: " << block.getRecordCount() << ")" << std::endl;

            // PASO 4: Procesar cada registro del bloque
            auto records = block.getActiveRecords();
            for (const auto& record : records) {
                if (max_records != -1 && records_processed >= max_records) {
                    break;
                }

                // Obtener campos del registro
                auto field_values = record->getFieldValues();
                
                if (field_values.size() <= static_cast<size_t>(key_field_index)) {
                    std::cout << "⚠️ Registro sin campo clave suficiente" << std::endl;
                    continue;
                }

                std::string key = field_values[key_field_index];
                
                // VERIFICACIÓN DE DUPLICADOS
                if (processed_keys.find(key) != processed_keys.end()) {
                    duplicate_count++;
                    if (duplicate_count <= 5) { // Mostrar solo los primeros 5
                        std::cout << "   ⚠️ Clave duplicada: " << key.substr(0, 20) << "..." << std::endl;
                    }
                    continue; // Saltar duplicados
                }
                
                processed_keys.insert(key);

                // Crear RecordReference para este registro
                RecordReference record_ref(page_addr, record->getId());
                
                // Insertar en B+ Tree usando RecordReference
                if (btree_index->insertReference(key, record_ref)) {
                    records_processed++;
                    
                    if (records_processed % 100 == 0) {
                        std::cout << "📈 Procesados: " << records_processed << " registros únicos" << std::endl;
                    }
                } else {
                    std::cout << "❌ Error insertando en B+ Tree: " << key << std::endl;
                }
            }
        }

        // PASO 5: Mostrar estadísticas finales
        std::cout << "\n✅ B+ TREE CONSTRUIDO:" << std::endl;
        std::cout << "   • Registros procesados: " << records_processed << std::endl;
        std::cout << "   • Duplicados omitidos: " << duplicate_count << std::endl;
        std::cout << "   • Altura del árbol: " << btree_index->getHeight() << std::endl;
        std::cout << "   • Orden del árbol: " << btree_index->getOrder() << std::endl;

        return btree_index;

    } catch (const std::exception& e) {
        std::cout << "❌ ERROR en buildBTreeIndex: " << e.what() << std::endl;
        return nullptr;
    }
}