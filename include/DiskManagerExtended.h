#ifndef DISK_MANAGER_EXTENDED_H
#define DISK_MANAGER_EXTENDED_H

#include "DiskManager.h"
#include "RecordReference.h"
#include "buffer/PageDirectory.h"
#include <unordered_map>
#include <vector>
#include <memory>

/**
 * @brief DiskManagerExtended - COMPLETAMENTE CORREGIDO para integración con IndexManager
 * 
 * ✅ CORRECCIONES FINALES APLICADAS:
 * - Eliminados override incorrectos
 * - Métodos compatibles con DiskManager base
 * - API correcta para insertRecord con vector<string>
 * - Sin métodos que no existen en clase base
 * - Sincronización correcta con relation_blocks del DiskManager base
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
    }

    /**
     * @brief Destructor
     */
    ~DiskManagerExtended() {
        if (page_directory) {
            page_directory->saveToDisk();
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
        
        // ✅ CORRECCIÓN: Usar getRelationBlocks() del DiskManager base
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it == relation_blocks.end()) {
            std::cout << "❌ Tabla no encontrada: " << table_name << std::endl;
            return false;
        }

        pages = it->second;
        std::cout << "✅ Encontradas " << pages.size() << " páginas para tabla " << table_name << std::endl;
        
        // Asegurar que todas las páginas estén en Page Directory
        registerTablePagesInDirectory(table_name);
        
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
        
        // ✅ CORRECCIÓN: Usar método público readBlock de DiskManager base
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
    bool registerBlockAsPage(const PhysicalAddress& addr, size_t page_size) {
        int page_id = page_directory->allocateNewPageId();
        return page_directory->registerPage(page_id, addr, page_size);
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
    // ✅ MÉTODOS COMPATIBLES CON DISKMANAGER BASE
    // ============================================================================

    /**
     * @brief ✅ Inserta registro usando vector<string> (compatible con DiskManager base)
     */
    bool insertRecordFromValues(const std::string& table_name, const std::vector<std::string>& values) {
        // Recordar cuántos bloques tenía la tabla antes
        size_t blocks_before = getTableBlockCount(table_name);
        
        // ✅ CORRECCIÓN: Usar método insertRecord del DiskManager base
        if (!insertRecord(table_name, values)) {
            return false;
        }
        
        // Registrar nuevos bloques en Page Directory si los hubo
        size_t blocks_after = getTableBlockCount(table_name);
        if (blocks_after > blocks_before) {
            registerNewTableBlocks(table_name, blocks_before);
        }
        
        return true;
    }

    /**
     * @brief ✅ Información del sistema SIN override
     */
    void displayExtendedSystemInfo() const {
        std::cout << "\n📋 DISK MANAGER EXTENDED INFO:" << std::endl;
        std::cout << "===============================" << std::endl;
        
        // Mostrar estadísticas base
        std::cout << "📊 ESTADÍSTICAS BASE:" << std::endl;
        const_cast<DiskManagerExtended*>(this)->displayStatistics();
        
        std::cout << "\n📋 PAGE DIRECTORY:" << std::endl;
        page_directory->displayInfo();
        
        std::cout << "\n📊 RELACIONES Y BLOQUES:" << std::endl;
        const auto& relation_blocks = getRelationBlocks();
        for (const auto& relation : relation_blocks) {
            std::cout << "   📋 " << relation.first << ": " << relation.second.size() << " bloques" << std::endl;
            
            // Mostrar algunas direcciones como muestra
            size_t sample_size = std::min(static_cast<size_t>(3), relation.second.size());
            for (size_t i = 0; i < sample_size; i++) {
                std::cout << "      • " << relation.second[i].toString() << std::endl;
            }
            if (relation.second.size() > sample_size) {
                std::cout << "      • ... y " << (relation.second.size() - sample_size) << " más" << std::endl;
            }
        }
        
        std::cout << "\n🔗 INTEGRACIÓN INDEXMANAGER:" << std::endl;
        std::cout << "   ✅ getTablePages() disponible" << std::endl;
        std::cout << "   ✅ createRecordReference() disponible" << std::endl;
        std::cout << "   ✅ resolveRecordReference() disponible" << std::endl;
        std::cout << "   📈 Page Directory: " << page_directory->getPageCount() << " páginas" << std::endl;
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
    // MÉTODOS AUXILIARES PRIVADOS
    // ============================================================================

    /**
     * @brief Registra páginas de una tabla en Page Directory
     */
    void registerTablePagesInDirectory(const std::string& table_name) {
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it != relation_blocks.end()) {
            for (const auto& addr : it->second) {
                int existing_page_id = getPageIdForAddress(addr);
                if (existing_page_id == -1) {
                    registerBlockAsPage(addr, getConfig().getBytesPerSector());
                }
            }
        }
    }

    /**
     * @brief Registra nuevos bloques de tabla en Page Directory
     */
    void registerNewTableBlocks(const std::string& table_name, size_t start_index) {
        const auto& relation_blocks = getRelationBlocks();
        auto it = relation_blocks.find(table_name);
        
        if (it != relation_blocks.end()) {
            for (size_t i = start_index; i < it->second.size(); ++i) {
                const auto& addr = it->second[i];
                registerBlockAsPage(addr, getConfig().getBytesPerSector());
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