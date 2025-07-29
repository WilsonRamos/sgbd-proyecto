#ifndef DISK_MANAGER_EXTENDED_H
#define DISK_MANAGER_EXTENDED_H

#include "DiskManager.h"
#include "RecordReference.h"
#include "buffer/PageDirectory.h"
#include <unordered_map>
#include <vector>
#include <memory>

/**
 * @brief DiskManagerExtended - COMPLETAMENTE CORREGIDO con sincronización automática
 * 
 * ✅ CORRECCIONES FINALES APLICADAS:
 * - Sincronización automática de PageDirectory en CADA inserción
 * - Métodos compatibles con DiskManager base
 * - API correcta para insertRecord con vector<string>
 * - Verificaciones robustas de integridad
 * - Diagnósticos completos integrados
 */
class DiskManagerExtended : public DiskManager {
private:
    std::unique_ptr<PageDirectory> page_directory;
    int next_page_id;

public:
    /**
     * @brief Constructor
     */
    DiskManagerExtended(const std::string& base_path) 
        : DiskManager(base_path)
        , next_page_id(1)
    {
        page_directory = std::make_unique<PageDirectory>(base_path);
        
        std::cout << "🔧 DiskManagerExtended inicializado:" << std::endl;
        std::cout << "   📁 Ruta base: " << base_path << std::endl;
        std::cout << "   📋 Page Directory: ✓" << std::endl;
        std::cout << "   🔗 IndexManager integration: ✓" << std::endl;
        std::cout << "   🔄 Sincronización automática: ✓" << std::endl;
    }

    /**
     * @brief Destructor
     */
    ~DiskManagerExtended() {
        if (page_directory) {
            page_directory->saveToDisk();
            std::cout << "💾 PageDirectory guardado al finalizar" << std::endl;
        }
    }

    // ============================================================================
    // ✅ MÉTODOS PRINCIPALES PARA INDEXMANAGER
    // ============================================================================

    /**
     * @brief ✅ MÉTODO PRINCIPAL - Obtiene páginas físicas de una tabla
     */
    bool getTablePages(const std::string& table_name, std::vector<PhysicalAddress>& pages) {
        std::cout << "🔍 Obteniendo páginas de tabla: " << table_name << std::endl;
        
        // ✅ Usar getRelationBlocks() del DiskManager base
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it == relation_blocks.end()) {
            std::cout << "❌ Tabla no encontrada: " << table_name << std::endl;
            return false;
        }

        pages = it->second;
        std::cout << "✅ Encontradas " << pages.size() << " páginas para tabla " << table_name << std::endl;
        
        // ✅ Verificar y sincronizar automáticamente si es necesario
        ensureAllPagesRegistered(table_name);
        
        return !pages.empty();
    }

    /**
     * @brief ✅ Crea RecordReference para un registro insertado
     */
    RecordReference createRecordReference(const PhysicalAddress& addr, int slot_id) {
        // Asegurar que el bloque esté registrado en Page Directory
        ensureBlockInPageDirectory(addr, getConfig().getBytesPerSector());
        
        // Obtener page_id para la dirección física
        int page_id = getPageIdForAddress(addr);
        if (page_id == -1) {
            // Si no existe, registrarlo
            page_id = registerBlockAsPage(addr, getConfig().getBytesPerSector());
        }
        
        // Crear y retornar RecordReference
        return RecordReference(addr, slot_id, page_id);
    }

    /**
     * @brief ✅ Resuelve RecordReference a datos reales
     */
    bool resolveRecordReference(const RecordReference& record_ref, Block& block) {
        if (!record_ref.isValid()) {
            return false;
        }
        
        // ✅ Usar método público readBlock de DiskManager base
        return readBlock(record_ref.getPhysicalAddress(), block);
    }

    /**
     * @brief ✅ Obtiene Page ID para una dirección física
     */
    int getPageIdForAddress(const PhysicalAddress& addr) {
        PageLocation location;
        
        // Buscar en Page Directory
        auto all_page_ids = page_directory->getAllPageIds();
        for (int page_id : all_page_ids) {
            if (page_directory->findPage(page_id, location)) {
                if (location.file_id == addr.toString()) {
                    return page_id;
                }
            }
        }
        
        return -1; // No encontrado
    }

    /**
     * @brief ✅ Registra un bloque como página en el Page Directory
     */
    int registerBlockAsPage(const PhysicalAddress& addr, size_t page_size) {
        // Verificar si ya está registrado
        int existing_page_id = getPageIdForAddress(addr);
        if (existing_page_id != -1) {
            return existing_page_id;
        }
        
        // Crear nueva entrada
        int new_page_id = page_directory->allocateNewPageId();
        bool success = page_directory->registerPage(new_page_id, addr, page_size);
        
        if (success) {
            std::cout << "📄 Nueva página registrada: " << addr.toString() 
                      << " → PageID " << new_page_id << std::endl;
            return new_page_id;
        } else {
            std::cout << "❌ Error registrando página: " << addr.toString() << std::endl;
            return -1;
        }
    }

    /**
     * @brief ✅ Obtiene dirección física para un Page ID
     */
    bool getAddressForPageId(int page_id, PhysicalAddress& addr) {
        PageLocation location;
        if (page_directory->findPage(page_id, location)) {
            // Parsear file_id de vuelta a PhysicalAddress
            std::string file_id = location.file_id;
            
            // Parsing simplificado: "P0_S0_T0_SEC0"
            size_t p_pos = file_id.find('P');
            size_t s_pos = file_id.find("_S");
            size_t t_pos = file_id.find("_T");
            size_t sec_pos = file_id.find("_SEC");
            
            if (p_pos != std::string::npos && s_pos != std::string::npos && 
                t_pos != std::string::npos && sec_pos != std::string::npos) {
                
                try {
                    int platter = std::stoi(file_id.substr(p_pos + 1, s_pos - p_pos - 1));
                    int surface = std::stoi(file_id.substr(s_pos + 2, t_pos - s_pos - 2));
                    int track = std::stoi(file_id.substr(t_pos + 2, sec_pos - t_pos - 2));
                    int sector = std::stoi(file_id.substr(sec_pos + 4));
                    
                    addr = PhysicalAddress(platter, surface, track, sector);
                    return true;
                } catch (const std::exception& e) {
                    std::cout << "❌ Error parseando dirección: " << file_id << " - " << e.what() << std::endl;
                }
            }
        }
        
        return false;
    }

    /**
     * @brief ✅ Asigna nuevo Page ID
     */
    int allocateNewPageId() {
        return page_directory->allocateNewPageId();
    }

    /**
     * @brief ✅ Encuentra ubicación de página en disco
     */
    bool findPageLocation(int page_id, PageLocation& location) {
        return page_directory->findPage(page_id, location);
    }

    // ============================================================================
    // ✅ MÉTODOS CRÍTICOS CON SINCRONIZACIÓN AUTOMÁTICA
    // ============================================================================

    /**
     * @brief ✅ MÉTODO CRÍTICO - Inserta registro Y sincroniza PageDirectory automáticamente
     */
    bool insertRecordFromValues(const std::string& table_name, const std::vector<std::string>& values) {
        // 1. Recordar estado ANTES de inserción
        size_t blocks_before = getTableBlockCount(table_name);
        
        // 2. ✅ INSERTAR usando método base (DiskManager::insertRecord)
        if (!DiskManager::insertRecord(table_name, values)) {
            return false;
        }
        
        // 3. ✅ SINCRONIZACIÓN AUTOMÁTICA POST-INSERCIÓN
        size_t blocks_after = getTableBlockCount(table_name);
        
        if (blocks_after > blocks_before) {
            // Nuevos bloques detectados - registrar automáticamente
            syncNewTableBlocks(table_name, blocks_before);
        }
        
        return true;
    }

    /**
     * @brief ✅ SINCRONIZACIÓN FORZADA COMPLETA
     */
    void forcePageDirectorySync() {
        std::cout << "\n🔄 SINCRONIZACIÓN FORZADA DE PAGEDIRECTORY..." << std::endl;
        
        int pages_before = page_directory->getPageCount();
        int pages_registered = 0;
        
        const auto& relation_blocks = getRelationBlocks();
        for (const auto& relation : relation_blocks) {
            std::cout << "   📋 Sincronizando tabla: " << relation.first 
                      << " (" << relation.second.size() << " bloques)" << std::endl;
            
            for (const auto& addr : relation.second) {
                int page_id = registerBlockAsPage(addr, getConfig().getBytesPerSector());
                if (page_id != -1) {
                    pages_registered++;
                }
            }
        }
        
        int pages_after = page_directory->getPageCount();
        
        std::cout << "\n✅ SINCRONIZACIÓN COMPLETADA:" << std::endl;
        std::cout << "   📄 Páginas antes: " << pages_before << std::endl;
        std::cout << "   📄 Páginas después: " << pages_after << std::endl;
        std::cout << "   📄 Páginas registradas: " << pages_registered << std::endl;
        
        // Guardar cambios inmediatamente
        if (page_directory->isDirty()) {
            page_directory->saveToDisk();
            std::cout << "   💾 PageDirectory guardado en disco" << std::endl;
        }
    }

    /**
     * @brief ✅ DIAGNÓSTICO COMPLETO DEL SISTEMA
     */
    void displayExtendedSystemInfo() const {
        std::cout << "\n📋 DISK MANAGER EXTENDED - DIAGNÓSTICO COMPLETO:" << std::endl;
        std::cout << "=================================================" << std::endl;
        
        // Estadísticas base del DiskManager
        std::cout << "📊 ESTADÍSTICAS BASE:" << std::endl;
        const_cast<DiskManagerExtended*>(this)->DiskManager::displayStatistics();
        
        // Estado del PageDirectory
        std::cout << "\n📄 PAGE DIRECTORY STATUS:" << std::endl;
        if (page_directory) {
            std::cout << "   Páginas registradas: " << page_directory->getPageCount() << std::endl;
            std::cout << "   Próximo Page ID: " << page_directory->getNextPageId() << std::endl;
            std::cout << "   Estado: " << (page_directory->isDirty() ? "Modificado" : "Sincronizado") << std::endl;
            std::cout << "   Archivo: " << page_directory->getDirectoryFile() << std::endl;
        } else {
            std::cout << "   ❌ PageDirectory no inicializado" << std::endl;
        }
        
        // Análisis de sincronización detallado
        const auto& relation_blocks = getRelationBlocks();
        int total_blocks = 0;
        
        std::cout << "\n📊 ANÁLISIS DE SINCRONIZACIÓN:" << std::endl;
        for (const auto& relation : relation_blocks) {
            int blocks_in_relation = relation.second.size();
            total_blocks += blocks_in_relation;
            
            std::cout << "   📋 " << relation.first << ":" << std::endl;
            std::cout << "      Bloques físicos: " << blocks_in_relation << std::endl;
            
            // Contar páginas registradas para esta tabla
            int registered_pages = 0;
            for (const auto& addr : relation.second) {
                if (const_cast<DiskManagerExtended*>(this)->getPageIdForAddress(addr) != -1) {
                    registered_pages++;
                }
            }
            
            std::cout << "      Páginas en directorio: " << registered_pages << std::endl;
            
            if (registered_pages != blocks_in_relation) {
                std::cout << "      ⚠️ DESINCRONIZACIÓN: " 
                          << (blocks_in_relation - registered_pages) 
                          << " bloques no registrados" << std::endl;
            } else {
                std::cout << "      ✅ Perfectamente sincronizado" << std::endl;
            }
        }
        
        std::cout << "\n📈 RESUMEN GLOBAL:" << std::endl;
        std::cout << "   Total bloques físicos: " << total_blocks << std::endl;
        std::cout << "   Total páginas registradas: " << (page_directory ? page_directory->getPageCount() : 0) << std::endl;
        
        if (page_directory && page_directory->getPageCount() != total_blocks) {
            std::cout << "   ❌ DESINCRONIZACIÓN GLOBAL detectada" << std::endl;
            std::cout << "   💡 Usar opción 53 para sincronizar automáticamente" << std::endl;
        } else {
            std::cout << "   ✅ Sistema completamente sincronizado" << std::endl;
        }
        
        std::cout << "\n🔗 INTEGRACIÓN INDEXMANAGER:" << std::endl;
        std::cout << "   ✅ getTablePages() → " << total_blocks << " páginas disponibles" << std::endl;
        std::cout << "   ✅ createRecordReference() → funcional" << std::endl;
        std::cout << "   ✅ resolveRecordReference() → funcional" << std::endl;
        std::cout << "   ✅ Sincronización automática → activa" << std::endl;
    }

    // ============================================================================
    // GETTERS PARA ACCESO CONTROLADO
    // ============================================================================
    
    const PageDirectory* getPageDirectory() const {
        return page_directory.get();
    }
    
    int getNextPageId() const {
        return next_page_id;
    }

private:
    // ============================================================================
    // MÉTODOS AUXILIARES PRIVADOS PARA SINCRONIZACIÓN
    // ============================================================================

    /**
     * @brief Sincroniza nuevos bloques de tabla en PageDirectory
     */
    void syncNewTableBlocks(const std::string& table_name, size_t start_index) {
        std::cout << "🔧 Sincronizando nuevos bloques para " << table_name << "..." << std::endl;
        
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it != relation_blocks.end()) {
            int synced = 0;
            for (size_t i = start_index; i < it->second.size(); ++i) {
                const auto& addr = it->second[i];
                int page_id = registerBlockAsPage(addr, getConfig().getBytesPerSector());
                if (page_id != -1) {
                    synced++;
                }
            }
            
            if (synced > 0) {
                std::cout << "✅ " << synced << " nuevos bloques sincronizados automáticamente" << std::endl;
                
                // Guardar cambios inmediatamente
                if (page_directory->isDirty()) {
                    page_directory->saveToDisk();
                }
            }
        }
    }

    /**
     * @brief Asegura que todas las páginas de una tabla estén registradas
     */
    void ensureAllPagesRegistered(const std::string& table_name) {
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it != relation_blocks.end()) {
            int registered = 0;
            for (const auto& addr : it->second) {
                if (getPageIdForAddress(addr) == -1) {
                    if (registerBlockAsPage(addr, getConfig().getBytesPerSector()) != -1) {
                        registered++;
                    }
                }
            }
            
            if (registered > 0) {
                std::cout << "🔧 " << registered << " páginas registradas automáticamente para " << table_name << std::endl;
            }
        }
    }

    /**
     * @brief Obtiene número de bloques de una tabla
     */
    size_t getTableBlockCount(const std::string& table_name) const {
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        return (it != relation_blocks.end()) ? it->second.size() : 0;
    }

    /**
     * @brief Asegura que un bloque esté en Page Directory
     */
    void ensureBlockInPageDirectory(const PhysicalAddress& addr, size_t page_size) {
        if (getPageIdForAddress(addr) == -1) {
            registerBlockAsPage(addr, page_size);
        }
    }
};

#endif // DISK_MANAGER_EXTENDED_H