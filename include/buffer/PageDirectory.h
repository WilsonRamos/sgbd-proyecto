#ifndef PAGE_DIRECTORY_H
#define PAGE_DIRECTORY_H

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "../PhysicalAddress.h"

/**
 * @brief Metadatos de ubicación de página en disco
 */
struct PageLocation {
    std::string file_id;      // Identificador del archivo
    size_t offset;           // Offset dentro del archivo
    size_t size;             // Tamaño de la página
    
    PageLocation() : offset(0), size(0) {}
    PageLocation(const std::string& fid, size_t off, size_t sz) 
        : file_id(fid), offset(off), size(sz) {}
    
    std::string toString() const {
        return file_id + "|" + std::to_string(offset) + "|" + std::to_string(size);
    }
    
    bool fromString(const std::string& str) {
        std::istringstream iss(str);
        std::string offset_str, size_str;
        return std::getline(iss, file_id, '|') &&
               std::getline(iss, offset_str, '|') &&
               std::getline(iss, size_str) &&
               (offset = std::stoull(offset_str), true) &&
               (size = std::stoull(size_str), true);
    }
};

/**
 * @brief Page Directory - Mapeo persistente de PageID a ubicación en disco
 * 
 * Implementa el concepto de Page Directory de la conferencia CMU:
 * - Gestionado por DiskManager (NO por BufferPoolManager)
 * - Persistente en disco (metadata/page_directory.txt)
 * - Mapea PageID → (FileID, Offset, Size)
 * - Diferente de Page Table (que está en memoria)
 * - Creado automáticamente cuando DiskManager crea tablas/bloques
 */
class PageDirectory {
private:
    std::unordered_map<int, PageLocation> directory;  // PageID → PageLocation
    std::string directory_file;                       // Archivo persistente
    bool is_dirty;                                   // Si necesita ser guardado
    int next_page_id;                                // Próximo Page ID disponible

public:
    /**
     * @brief Constructor - Solo debe ser usado por DiskManager
     */
    PageDirectory(const std::string& base_path = "./disk_simulation") 
        : directory_file(base_path + "/metadata/page_directory.txt")
        , is_dirty(false)
        , next_page_id(1)
    {
        loadFromDisk();
    }

    /**
     * @brief Asigna nuevo Page ID automáticamente (usado por DiskManager)
     */
    int allocateNewPageId() {
        return next_page_id++;
    }

    /**
     * @brief Registra una nueva página en el directorio (llamado por DiskManager)
     */
    bool registerPage(int page_id, const PhysicalAddress& physical_addr, size_t page_size) {
        // Convertir PhysicalAddress a identificador de archivo
        std::string file_id = physical_addr.toString();
        size_t offset = 0;  // En nuestro sistema, cada sector es un archivo
        
        directory[page_id] = PageLocation(file_id, offset, page_size);
        is_dirty = true;
        
        // Actualizar next_page_id si es necesario
        if (page_id >= next_page_id) {
            next_page_id = page_id + 1;
        }
        
        std::cout << "📁 Page Directory: Registrada página " << page_id 
                  << " → " << file_id << std::endl;
        return true;
    }

    /**
     * @brief Busca la ubicación de una página
     */
    bool findPage(int page_id, PageLocation& location) const {
        auto it = directory.find(page_id);
        if (it != directory.end()) {
            location = it->second;
            return true;
        }
        return false;
    }

    /**
     * @brief Elimina una página del directorio
     */
    bool removePage(int page_id) {
        auto it = directory.find(page_id);
        if (it != directory.end()) {
            directory.erase(it);
            is_dirty = true;
            std::cout << "📁 Page Directory: Eliminada página " << page_id << std::endl;
            return true;
        }
        return false;
    }

    /**
     * @brief Verifica si una página existe
     */
    bool pageExists(int page_id) const {
        return directory.find(page_id) != directory.end();
    }

    /**
     * @brief Obtiene todas las páginas registradas
     */
    std::vector<int> getAllPageIds() const {
        std::vector<int> page_ids;
        for (const auto& entry : directory) {
            page_ids.push_back(entry.first);
        }
        return page_ids;
    }

    /**
     * @brief Guarda el directorio en disco
     */
    bool saveToDisk() {
        if (!is_dirty) return true;
        
        std::ofstream file(directory_file);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo abrir " << directory_file << std::endl;
            return false;
        }
        
        file << "# Page Directory - Mapeo PageID → (FileID, Offset, Size)" << std::endl;
        file << "# Generado automáticamente por DiskManager" << std::endl;
        file << "total_pages=" << directory.size() << std::endl;
        file << "next_page_id=" << next_page_id << std::endl;
        
        for (const auto& entry : directory) {
            file << entry.first << "=" << entry.second.toString() << std::endl;
        }
        
        file.close();
        is_dirty = false;
        
        std::cout << "💾 Page Directory guardado: " << directory.size() 
                  << " páginas (next_id: " << next_page_id << ")" << std::endl;
        return true;
    }

    /**
     * @brief Carga el directorio desde disco
     */
    bool loadFromDisk() {
        std::ifstream file(directory_file);
        if (!file.is_open()) {
            std::cout << "📁 Page Directory: Creando nuevo directorio" << std::endl;
            next_page_id = 1;
            return true;  // Archivo nuevo, no es error
        }
        
        directory.clear();
        std::string line;
        int max_page_id = 0;
        
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("total_pages=") == 0) continue;
            if (line.find("next_page_id=") == 0) {
                next_page_id = std::stoi(line.substr(13));
                continue;
            }
            
            // Parsear línea: page_id=file_id|offset|size
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                int page_id = std::stoi(line.substr(0, eq_pos));
                std::string location_str = line.substr(eq_pos + 1);
                
                PageLocation location;
                if (location.fromString(location_str)) {
                    directory[page_id] = location;
                    max_page_id = std::max(max_page_id, page_id);
                }
            }
        }
        
        file.close();
        is_dirty = false;
        
        // Asegurar que next_page_id sea correcto
        if (next_page_id <= max_page_id) {
            next_page_id = max_page_id + 1;
        }
        
        std::cout << "📁 Page Directory cargado: " << directory.size() 
                  << " páginas (next_id: " << next_page_id << ")" << std::endl;
        return true;
    }

    /**
     * @brief Muestra información del directorio
     */
    void displayInfo() const {
        std::cout << "\n=== PAGE DIRECTORY (DISCO) ===" << std::endl;
        std::cout << "Archivo: " << directory_file << std::endl;
        std::cout << "Páginas registradas: " << directory.size() << std::endl;
        std::cout << "Estado: " << (is_dirty ? "Modificado" : "Sincronizado") << std::endl;
        
        if (!directory.empty()) {
            std::cout << "\nPáginas:" << std::endl;
            for (const auto& entry : directory) {
                std::cout << "  Página " << entry.first << " → " 
                          << entry.second.file_id 
                          << " (offset: " << entry.second.offset 
                          << ", size: " << entry.second.size << ")" << std::endl;
            }
        }
    }

    /**
     * @brief Destructor - asegura que se guarden los cambios
     */
    ~PageDirectory() {
        if (is_dirty) {
            saveToDisk();
        }
    }

    // Getters
    size_t getPageCount() const { return directory.size(); }
    bool isDirty() const { return is_dirty; }
    const std::string& getDirectoryFile() const { return directory_file; }
    int getNextPageId() const { return next_page_id; }
};

#endif // PAGE_DIRECTORY_H