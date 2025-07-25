#ifndef DISK_MANAGER_EXTENDED_H
#define DISK_MANAGER_EXTENDED_H

#include "DiskManager.h"
#include "buffer/PageDirectory.h"
#include <memory>

/**
 * @brief DiskManager Extendido con Page Directory
 * 
 * Extiende el DiskManager existente para incluir gestión del Page Directory:
 * - Page Directory es responsabilidad del DiskManager (no del BufferPool)
 * - Mapea automáticamente bloques a páginas cuando se crean tablas
 * - Mantiene persistencia del directorio en disco
 * - Proporciona interfaz para que BufferPool consulte ubicaciones
 */
class DiskManagerExtended : public DiskManager {
private:
    std::unique_ptr<PageDirectory> page_directory;    // Page Directory gestionado aquí
    
    
public:
    /**
     * @brief Constructor
     */
    DiskManagerExtended(const std::string& disk_path = "./disk_simulation") 
        : DiskManager(disk_path) {
        // Inicializar Page Directory en el constructor
        page_directory = std::make_unique<PageDirectory>(disk_path);
    }

    /**
     * @brief Inicializa disco y Page Directory
     */
    bool initialize(const DiskConfig& disk_config) {
        bool success = DiskManager::initialize(disk_config);
        if (success) {
            // Page Directory ya se inicializó en constructor
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
            }
        }
        
        return success;
    }

    /**
     * @brief Escribe bloque y actualiza Page Directory
     */
    bool writeBlock(const PhysicalAddress& addr, const Block& block) {
        // Implementación inline - usar filesystem directamente
        bool success = getFileSystem().writeBlock(addr, block);
        
        if (success) {
            // Verificar si necesitamos registrar en Page Directory
            ensureBlockInPageDirectory(addr, block.getBlockSize());
        }
        
        return success;
    }

    /**
     * @brief Lee bloque 
     */
    bool readBlock(const PhysicalAddress& addr, Block& block) {
        // Implementación inline - usar filesystem directamente
        return getFileSystem().readBlock(addr, block);
    }

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
        return page_directory->registerPage(page_id, addr, block_size);
    }

    /**
     * @brief Muestra información del Page Directory
     */
    void displayPageDirectory() {
        page_directory->displayInfo();
    }

    /**
     * @brief Estadísticas incluyendo Page Directory
     */
    void displayStatistics() {
        DiskManager::displayStatistics();
        page_directory->displayInfo();
    }

    /**
     * @brief Destructor - guarda Page Directory
     */
    ~DiskManagerExtended() {
        if (page_directory) {
            page_directory->saveToDisk();
        }
    }

protected:
    /**
     * @brief Obtiene acceso a relation_blocks del padre
     */
    const std::map<std::string, std::vector<PhysicalAddress>>& getRelationBlocks() const {
        return relation_blocks;
    }

    /**
     * @brief Obtiene acceso a config del padre  
     */
    const DiskConfig& getDiskConfig() const {
        return config;
    }

    /**
     * @brief Obtiene acceso al filesystem del padre
     */
    FileSystemSimulator& getFileSystem() {
        return filesystem;
    }

private:
    /**
     * @brief Registra páginas de una tabla en Page Directory
     */
    void registerTablePagesInDirectory(const std::string& table_name) {
        const auto& relation_blocks_ref = getRelationBlocks();
        auto it = relation_blocks_ref.find(table_name);
        if (it != relation_blocks_ref.end()) {
            for (const auto& addr : it->second) {
                // Verificar si ya está registrado
                int existing_page_id = getPageIdForAddress(addr);
                if (existing_page_id == -1) {
                    // No está registrado, registrar
                    registerBlockAsPage(addr, getDiskConfig().getBytesPerSector());
                }
            }
        }
    }

    /**
     * @brief Registra nuevos bloques de tabla en Page Directory
     */
    void registerNewTableBlocks(const std::string& table_name, size_t start_index) {
        const auto& relation_blocks_ref = getRelationBlocks();
        auto it = relation_blocks_ref.find(table_name);
        if (it != relation_blocks_ref.end()) {
            for (size_t i = start_index; i < it->second.size(); ++i) {
                const auto& addr = it->second[i];
                registerBlockAsPage(addr, getDiskConfig().getBytesPerSector());
            }
        }
    }

    /**
     * @brief Obtiene número de bloques de una tabla
     */
    size_t getTableBlockCount(const std::string& table_name) {
        const auto& relation_blocks_ref = getRelationBlocks();
        auto it = relation_blocks_ref.find(table_name);
        return (it != relation_blocks_ref.end()) ? it->second.size() : 0;
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
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setPlatter(std::stoi(part.substr(1)));
            }
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setSurface(std::stoi(part.substr(1)));
            }
            if (std::getline(iss, part, '_') && part.size() > 1) {
                addr.setTrack(std::stoi(part.substr(1)));
            }
            if (std::getline(iss, part) && part.size() > 3) {
                addr.setSector(std::stoi(part.substr(3)));
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
};

#endif // DISK_MANAGER_EXTENDED_H