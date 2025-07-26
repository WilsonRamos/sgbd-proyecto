#ifndef DISK_MANAGER_EXTENDED_H
#define DISK_MANAGER_EXTENDED_H

#include "DiskManager.h"
#include "buffer/PageDirectory.h"
#include <memory>
#include <string>
#include <iostream>
#include <sstream>

/**
 * @brief DiskManager extendido con Page Directory integrado
 * 
 * Extiende DiskManager base con funcionalidades avanzadas:
 * - Page Directory persistente para mapeo físico
 * - Integración automática con BufferPoolManager
 * - Registro automático de bloques/páginas
 * - Compatibilidad con índices especializados
 */
class DiskManagerExtended : public DiskManager {
private:
    std::unique_ptr<PageDirectory> page_directory;
    
public:
    /**
     * @brief Constructor
     */
    DiskManagerExtended(const std::string& disk_path = "./disk_simulation") 
        : DiskManager(disk_path) {
        // Inicializar Page Directory en el constructor
        page_directory = std::make_unique<PageDirectory>(disk_path);
        std::cout << "📁 DiskManagerExtended inicializado con Page Directory" << std::endl;
    }

    /**
     * @brief Destructor - guarda Page Directory
     */
    ~DiskManagerExtended() {
        if (page_directory) {
            page_directory->saveToDisk();
            std::cout << "💾 Page Directory guardado al cerrar DiskManagerExtended" << std::endl;
        }
    }

    // ============================================================================
    // MÉTODOS SOBRECARGADOS PARA INTEGRACIÓN CON PAGE DIRECTORY
    // ============================================================================

    /**
     * @brief Inicializa disco y Page Directory
     */
    bool initialize(const DiskConfig& disk_config) {
        bool success = DiskManager::initialize(disk_config);
        if (success) {
            std::cout << "📁 Page Directory inicializado por DiskManager" << std::endl;
        }
        return success;
    }

    /**
     * @brief Carga disco existente y Page Directory
     */
    bool loadExistingDisk() {
        bool success = DiskManager::loadExistingDisk();
        if (success) {
            // Page Directory se carga automáticamente en constructor
            std::cout << "📁 Page Directory cargado por DiskManager" << std::endl;
        }
        return success;
    }

    /**
     * @brief Crea tabla y registra páginas en Page Directory
     */
    bool createTable(const std::string& table_name, 
                     const std::vector<FieldDefinition>& schema,
                     bool use_fixed_records = true) {
        
        // Llamar al método padre para crear la tabla
        bool success = DiskManager::createTable(table_name, schema, use_fixed_records);
        
        if (success) {
            // Registrar bloques de la tabla en Page Directory
            registerTablePagesInDirectory(table_name);
            std::cout << "📋 Tabla '" << table_name << "' registrada en Page Directory" << std::endl;
        }
        
        return success;
    }

    /**
     * @brief Inserta registro y actualiza Page Directory si es necesario
     */
    bool insertRecord(const std::string& table_name, 
                      const std::vector<std::string>& values) {
        
        // Obtener bloques antes de insertar
        size_t blocks_before = getTableBlockCount(table_name);
        
        // Llamar al método padre
        bool success = DiskManager::insertRecord(table_name, values);
        
        if (success) {
            // Verificar si se creó un nuevo bloque
            size_t blocks_after = getTableBlockCount(table_name);
            if (blocks_after > blocks_before) {
                // Registrar nuevos bloques en Page Directory
                registerNewTableBlocks(table_name, blocks_before);
                std::cout << "📄 Nuevos bloques registrados en Page Directory" << std::endl;
            }
        }
        
        return success;
    }

    /**
     * @brief Escribe bloque y actualiza Page Directory
     */
    bool writeBlock(const PhysicalAddress& addr, const Block& block) {
        // Usar FileSystemSimulator directamente para compatibilidad
        bool success = filesystem.writeBlock(addr, block);
        
        if (success) {
            // Verificar si necesitamos registrar en Page Directory
            ensureBlockInPageDirectory(addr, block.getBlockSize());
        }
        
        return success;
    }

    /**
     * @brief Lee bloque usando FileSystemSimulator
     */
    bool readBlock(const PhysicalAddress& addr, Block& block) {
        // Usar FileSystemSimulator directamente para compatibilidad
        return filesystem.readBlock(addr, block);
    }

    // ============================================================================
    // MÉTODOS PARA INTEGRACIÓN CON BUFFER POOL Y ÍNDICES
    // ============================================================================

    /**
     * @brief Obtiene Page Directory (para consulta por BufferPool)
     */
    PageDirectory* getPageDirectory() {
        return page_directory.get();
    }

    /**
     * @brief Busca ubicación de página en disco
     */
    bool findPageLocation(int page_id, PageLocation& location) {
        return page_directory->findPage(page_id, location);
    }

    /**
     * @brief Convierte PhysicalAddress a Page ID
     */
    int getPageIdForAddress(const PhysicalAddress& addr) {
        // Buscar en el Page Directory el page_id que corresponde a esta dirección
        auto all_pages = page_directory->getAllPageIds();
        for (int page_id : all_pages) {
            PageLocation location;
            if (page_directory->findPage(page_id, location)) {
                if (location.file_id == addr.toString()) {
                    return page_id;
                }
            }
        }
        return -1; // No encontrado
    }

    /**
     * @brief Obtiene PhysicalAddress para un Page ID
     */
    bool getAddressForPageId(int page_id, PhysicalAddress& addr) {
        PageLocation location;
        if (page_directory->findPage(page_id, location)) {
            return parsePhysicalAddressFromFileId(location.file_id, addr);
        }
        return false;
    }

    /**
     * @brief Asigna nuevo Page ID
     */
    int allocateNewPageId() {
        return page_directory->allocateNewPageId();
    }

    /**
     * @brief Registra bloque en Page Directory
     */
    bool registerBlockAsPage(const PhysicalAddress& addr, size_t block_size) {
        int page_id = page_directory->allocateNewPageId();
        bool success = page_directory->registerPage(page_id, addr, block_size);
        if (success) {
            std::cout << "📄 Bloque " << addr.toString() << " registrado como página " << page_id << std::endl;
        }
        return success;
    }

    // ============================================================================
    // MÉTODOS DE ACCESO COMPATIBLES CON DISEÑO EXISTENTE
    // ============================================================================

    /**
     * @brief Obtiene referencia al FileSystemSimulator (para compatibilidad)
     */
    FileSystemSimulator& getFileSystem() {
        return filesystem;
    }

    /**
     * @brief Obtiene configuración del disco
     */
    const DiskConfig& getDiskConfig() const {
        return config;
    }

    /**
     * @brief Obtiene bloques de relaciones (para compatibilidad)
     */
    const std::map<std::string, std::vector<PhysicalAddress>>& getRelationBlocks() const {
        return relation_blocks;
    }

    /**
     * @brief Obtiene páginas de una tabla
     */
    std::vector<PhysicalAddress> getTablePages(const std::string& table_name) {
        auto it = relation_blocks.find(table_name);
        if (it != relation_blocks.end()) {
            return it->second;
        }
        return {};
    }

    // ============================================================================
    // INFORMACIÓN Y ESTADÍSTICAS EXTENDIDAS
    // ============================================================================

    /**
     * @brief Muestra información del Page Directory
     */
    void displayPageDirectory() {
        if (page_directory) {
            page_directory->displayInfo();
        } else {
            std::cout << "❌ Page Directory no inicializado" << std::endl;
        }
    }

    /**
     * @brief Estadísticas incluyendo Page Directory
     */
    void displayStatistics() {
        std::cout << "\n=== ESTADÍSTICAS DISKMANAGER EXTENDIDO ===" << std::endl;
        
        // Estadísticas del DiskManager base
        DiskManager::displayStatistics();
        
        // Estadísticas del Page Directory
        if (page_directory) {
            std::cout << "\n=== ESTADÍSTICAS PAGE DIRECTORY ===" << std::endl;
            page_directory->displayInfo();
        }
        
        // Estadísticas de mapeo
        std::cout << "\n=== ESTADÍSTICAS DE MAPEO ===" << std::endl;
        auto all_pages = page_directory->getAllPageIds();
        std::cout << "Total de páginas registradas: " << all_pages.size() << std::endl;
        
        // Contar páginas por tabla
        std::map<std::string, int> pages_per_table;
        for (const auto& table_entry : relation_blocks) {
            int page_count = 0;
            for (const auto& addr : table_entry.second) {
                if (getPageIdForAddress(addr) != -1) {
                    page_count++;
                }
            }
            pages_per_table[table_entry.first] = page_count;
        }
        
        std::cout << "Páginas por tabla:" << std::endl;
        for (const auto& entry : pages_per_table) {
            std::cout << "  " << entry.first << ": " << entry.second << " páginas" << std::endl;
        }
    }

protected:
    // ============================================================================
    // MÉTODOS AUXILIARES PRIVADOS
    // ============================================================================

    /**
     * @brief Registra páginas de una tabla en Page Directory
     */
    void registerTablePagesInDirectory(const std::string& table_name) {
        auto it = relation_blocks.find(table_name);
        if (it != relation_blocks.end()) {
            for (const auto& addr : it->second) {
                // Verificar si ya está registrado
                int existing_page_id = getPageIdForAddress(addr);
                if (existing_page_id == -1) {
                    // No está registrado, registrar
                    registerBlockAsPage(addr, config.getBytesPerSector());
                }
            }
        }
    }

    /**
     * @brief Registra nuevos bloques de tabla en Page Directory
     */
    void registerNewTableBlocks(const std::string& table_name, size_t start_index) {
        auto it = relation_blocks.find(table_name);
        if (it != relation_blocks.end()) {
            for (size_t i = start_index; i < it->second.size(); ++i) {
                const auto& addr = it->second[i];
                registerBlockAsPage(addr, config.getBytesPerSector());
            }
        }
    }

    /**
     * @brief Obtiene número de bloques de una tabla
     */
    size_t getTableBlockCount(const std::string& table_name) {
        auto it = relation_blocks.find(table_name);
        return (it != relation_blocks.end()) ? it->second.size() : 0;
    }

    /**
     * @brief Asegura que un bloque esté en Page Directory
     */
    void ensureBlockInPageDirectory(const PhysicalAddress& addr, size_t block_size) {
        int existing_page_id = getPageIdForAddress(addr);
        if (existing_page_id == -1) {
            registerBlockAsPage(addr, block_size);
        }
    }

    /**
     * @brief Parsea file_id para obtener PhysicalAddress
     */
    bool parsePhysicalAddressFromFileId(const std::string& file_id, PhysicalAddress& addr) {
        // Formato esperado: "P0_S0_T0_SEC1"
        std::istringstream iss(file_id);
        std::string part;
        
        try {
            // Parsear P0
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setPlatter(std::stoi(part.substr(1)));
            } else {
                return false;
            }
            
            // Parsear S0
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setSurface(std::stoi(part.substr(1)));
            } else {
                return false;
            }
            
            // Parsear T0
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setTrack(std::stoi(part.substr(1)));
            } else {
                return false;
            }
            
            // Parsear SEC1
            if (std::getline(iss, part) && part.size() > 3) {
                addr.setSector(std::stoi(part.substr(3)));
            } else {
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Error parseando file_id '" << file_id << "': " << e.what() << std::endl;
            return false;
        }
    }

public:
    // ============================================================================
    // MÉTODOS ESPECÍFICOS PARA INTEGRACIÓN CON ÍNDICES
    // ============================================================================

    /**
     * @brief Crea RecordReference para un registro insertado
     */
    RecordReference createRecordReference(const PhysicalAddress& addr, int slot_id) {
        // Asegurar que el bloque esté registrado en Page Directory
        ensureBlockInPageDirectory(addr, config.getBytesPerSector());
        
        // Crear y retornar RecordReference
        return RecordReference(addr, slot_id);
    }

    /**
     * @brief Resuelve RecordReference a datos reales
     */
    bool resolveRecordReference(const RecordReference& record_ref, Block& block) {
        if (!record_ref.isValid()) {
            return false;
        }
        
        // Leer bloque desde la dirección física
        return readBlock(record_ref.getPhysicalAddress(), block);
    }

    /**
     * @brief Obtiene información de página para BufferManager
     */
    struct PageInfo {
        int page_id;
        PhysicalAddress physical_address;
        size_t page_size;
        bool is_registered;
    };
    
    PageInfo getPageInfo(const PhysicalAddress& addr) {
        PageInfo info;
        info.physical_address = addr;
        info.page_size = config.getBytesPerSector();
        info.page_id = getPageIdForAddress(addr);
        info.is_registered = (info.page_id != -1);
        
        return info;
    }

    /**
     * @brief Sincroniza Page Directory con estado actual
     */
    void synchronizePageDirectory() {
        std::cout << "🔄 Sincronizando Page Directory..." << std::endl;
        
        int registered_count = 0;
        for (const auto& table_entry : relation_blocks) {
            for (const auto& addr : table_entry.second) {
                if (getPageIdForAddress(addr) == -1) {
                    registerBlockAsPage(addr, config.getBytesPerSector());
                    registered_count++;
                }
            }
        }
        
        std::cout << "✅ Sincronización completada: " << registered_count 
                  << " bloques registrados" << std::endl;
    }

    /**
     * @brief Valida integridad del Page Directory
     */
    bool validatePageDirectory() {
        std::cout << "🔍 Validando integridad del Page Directory..." << std::endl;
        
        bool is_valid = true;
        auto all_pages = page_directory->getAllPageIds();
        
        for (int page_id : all_pages) {
            PageLocation location;
            if (page_directory->findPage(page_id, location)) {
                // Verificar que el archivo físico exista
                PhysicalAddress addr;
                if (parsePhysicalAddressFromFileId(location.file_id, addr)) {
                    std::string file_path = filesystem.getFullPath(addr);
                    if (!std::filesystem::exists(file_path)) {
                        std::cout << "❌ Archivo físico no encontrado para página " 
                                  << page_id << ": " << file_path << std::endl;
                        is_valid = false;
                    }
                } else {
                    std::cout << "❌ Error parseando file_id para página " 
                              << page_id << ": " << location.file_id << std::endl;
                    is_valid = false;
                }
            }
        }
        
        if (is_valid) {
            std::cout << "✅ Page Directory válido (" << all_pages.size() << " páginas)" << std::endl;
        } else {
            std::cout << "❌ Page Directory contiene inconsistencias" << std::endl;
        }
        
        return is_valid;
    }
};

#endif // DISK_MANAGER_EXTENDED_H