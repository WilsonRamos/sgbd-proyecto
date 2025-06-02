#ifndef FILESYSTEM_SIMULATOR_H
#define FILESYSTEM_SIMULATOR_H

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "DiskConfig.h"
#include "PhysicalAddress.h"
#include "Block.h"

namespace fs = std::filesystem;

/**
 * @brief Simula el sistema de archivos usando carpetas y archivos .txt
 * 
 * Crea una estructura jerárquica que representa la organización física
 * del disco: platos -> superficies -> pistas -> sectores
 */
class FileSystemSimulator {
private:
    std::string base_path;          // Ruta base para la simulación
    DiskConfig disk_config;         // Configuración del disco
    bool initialized;               // Si el sistema está inicializado

public:
    /**
     * @brief Constructor
     */
    FileSystemSimulator(const std::string& path = "./disk_simulation") 
        : base_path(path), initialized(false) {}

    /**
     * @brief Inicializa el sistema de archivos con la configuración dada
     */
    bool initialize(const DiskConfig& config) {
        disk_config = config;
        
        try {
            // Crear directorio base
            fs::create_directories(base_path);
            
            // Crear estructura de directorios
            createDirectoryStructure();
            
            // Guardar metadatos
            saveMetadata();
            
            initialized = true;
            std::cout << "Sistema de archivos inicializado en: " << base_path << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error inicializando sistema de archivos: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Carga una configuración existente
     */
    bool loadExisting() {
        try {
            if (!fs::exists(base_path)) {
                return false;
            }
            
            // Cargar metadatos
            if (!loadMetadata()) {
                return false;
            }
            
            initialized = true;
            std::cout << "Sistema de archivos cargado desde: " << base_path << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error cargando sistema de archivos: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Obtiene la ruta completa para una dirección física
     */
    std::string getFullPath(const PhysicalAddress& address) const {
        return base_path + "/" + address.getDirectoryPath() + "/" + address.getSectorFileName();
    }

    /**
     * @brief Verifica si una dirección física es válida
     */
    bool isValidAddress(const PhysicalAddress& address) const {
        return address.getPlatter() >= 0 && 
               address.getPlatter() < disk_config.getNumPlatters() &&
               address.getSurface() >= 0 && 
               address.getSurface() < disk_config.getSurfacesPerPlatter() &&
               address.getTrack() >= 0 && 
               address.getTrack() < disk_config.getTracksPerSurface() &&
               address.getSector() >= 0 && 
               address.getSector() < disk_config.getSectorsPerTrack();
    }

    /**
     * @brief Escribe un bloque en la dirección especificada
     */
    bool writeBlock(const PhysicalAddress& address, const Block& block) {
        if (!initialized || !isValidAddress(address)) {
            return false;
        }

        try {
            std::string file_path = getFullPath(address);
            
            // Crear directorios si no existen
            fs::create_directories(fs::path(file_path).parent_path());
            
            // Escribir el bloque
            std::ofstream file(file_path);
            if (!file.is_open()) {
                return false;
            }
            
            file << "# Sector: " << address.toString() << std::endl;
            file << "# Fecha: " << getCurrentTimestamp() << std::endl;
            file << "# Relación: " << block.getRelationName() << std::endl;
            file << "# Registros: " << block.getRecordCount() << std::endl;
            file << "# Ocupación: " << block.getOccupancyPercentage() << "%" << std::endl;
            file << "# =================================" << std::endl;
            file << block.serialize();
            
            file.close();
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error escribiendo bloque: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Lee un bloque desde la dirección especificada
     */
    bool readBlock(const PhysicalAddress& address, Block& block) {
        if (!initialized || !isValidAddress(address)) {
            return false;
        }

        try {
            std::string file_path = getFullPath(address);
            
            if (!fs::exists(file_path)) {
                return false;
            }
            
            std::ifstream file(file_path);
            if (!file.is_open()) {
                return false;
            }
            
            // Saltar comentarios
            std::string line;
            while (std::getline(file, line) && line.find("#") == 0) {
                // Saltar líneas de comentario
            }
            
            // Leer contenido del bloque
            std::ostringstream content;
            content << line << std::endl;  // Primera línea después de comentarios
            content << file.rdbuf();
            
            file.close();
            
            return block.deserialize(content.str());
            
        } catch (const std::exception& e) {
            std::cerr << "Error leyendo bloque: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Elimina un bloque (archivo de sector)
     */
    bool deleteBlock(const PhysicalAddress& address) {
        if (!initialized || !isValidAddress(address)) {
            return false;
        }

        try {
            std::string file_path = getFullPath(address);
            
            if (fs::exists(file_path)) {
                fs::remove(file_path);
                return true;
            }
            
            return false;
            
        } catch (const std::exception& e) {
            std::cerr << "Error eliminando bloque: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Lista todos los sectores ocupados
     */
    std::vector<PhysicalAddress> getOccupiedSectors() const {
        std::vector<PhysicalAddress> occupied;
        
        if (!initialized) return occupied;
        
        try {
            for (int p = 0; p < disk_config.getNumPlatters(); ++p) {
                for (int s = 0; s < disk_config.getSurfacesPerPlatter(); ++s) {
                    for (int t = 0; t < disk_config.getTracksPerSurface(); ++t) {
                        std::string track_path = base_path + "/platter_" + std::to_string(p) + 
                                               "/surface_" + std::to_string(s) + 
                                               "/track_" + std::to_string(t);
                        
                        if (fs::exists(track_path)) {
                            for (const auto& entry : fs::directory_iterator(track_path)) {
                                if (entry.is_regular_file() && 
                                    entry.path().extension() == ".txt") {
                                    
                                    std::string filename = entry.path().stem().string();
                                    if (filename.find("sector_") == 0) {
                                        int sector_num = std::stoi(filename.substr(7));
                                        occupied.emplace_back(p, s, t, sector_num);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error listando sectores: " << e.what() << std::endl;
        }
        
        return occupied;
    }

    /**
     * @brief Calcula estadísticas de uso del disco
     */
    void displayUsageStatistics() const {
        if (!initialized) {
            std::cout << "Sistema de archivos no inicializado." << std::endl;
            return;
        }
        
        auto occupied = getOccupiedSectors();
        long long total_sectors = disk_config.getTotalSectors();
        
        std::cout << "\n=== ESTADÍSTICAS DE USO DEL DISCO ===" << std::endl;
        std::cout << "Sectores totales: " << total_sectors << std::endl;
        std::cout << "Sectores ocupados: " << occupied.size() << std::endl;
        std::cout << "Sectores libres: " << (total_sectors - occupied.size()) << std::endl;
        std::cout << "Porcentaje de ocupación: " 
                  << (static_cast<double>(occupied.size()) / total_sectors * 100.0) 
                  << "%" << std::endl;
        std::cout << "Capacidad total: " << disk_config.getFormattedCapacity() << std::endl;
        std::cout << "Espacio usado: " 
                  << formatBytes(occupied.size() * disk_config.getBytesPerSector()) << std::endl;
    }

    /**
     * @brief Muestra la estructura de directorios creada
     */
    void displayDirectoryStructure() const {
        std::cout << "\n=== ESTRUCTURA DEL DISCO SIMULADO ===" << std::endl;
        std::cout << "Ruta base: " << base_path << std::endl;
        std::cout << "├── metadata/" << std::endl;
        std::cout << "│   ├── disk_config.txt" << std::endl;
        std::cout << "│   ├── usage_stats.txt" << std::endl;
        std::cout << "│   └── allocation_map.txt" << std::endl;
        
        for (int p = 0; p < disk_config.getNumPlatters(); ++p) {
            std::cout << "├── platter_" << p << "/" << std::endl;
            for (int s = 0; s < disk_config.getSurfacesPerPlatter(); ++s) {
                std::cout << "│   ├── surface_" << s << "/" << std::endl;
                std::cout << "│   │   ├── track_0/ (hasta track_" 
                          << (disk_config.getTracksPerSurface() - 1) << ")" << std::endl;
                std::cout << "│   │   │   ├── sector_0.txt" << std::endl;
                std::cout << "│   │   │   └── ... (hasta sector_" 
                          << (disk_config.getSectorsPerTrack() - 1) << ".txt)" << std::endl;
            }
        }
    }

    // Getters
    const DiskConfig& getDiskConfig() const { return disk_config; }
    const std::string& getBasePath() const { return base_path; }
    bool isInitialized() const { return initialized; }

private:
    /**
     * @brief Crea la estructura completa de directorios
     */
    void createDirectoryStructure() {
        // Crear directorio de metadatos
        fs::create_directories(base_path + "/metadata");
        
        // Crear estructura de platos/superficies/pistas
        for (int p = 0; p < disk_config.getNumPlatters(); ++p) {
            for (int s = 0; s < disk_config.getSurfacesPerPlatter(); ++s) {
                for (int t = 0; t < disk_config.getTracksPerSurface(); ++t) {
                    std::string track_path = base_path + "/platter_" + std::to_string(p) + 
                                           "/surface_" + std::to_string(s) + 
                                           "/track_" + std::to_string(t);
                    fs::create_directories(track_path);
                }
            }
        }
    }

    /**
     * @brief Guarda metadatos del disco
     */
    void saveMetadata() {
        // Guardar configuración
        disk_config.saveToFile(base_path + "/metadata/disk_config.txt");
        
        // Guardar información adicional
        std::ofstream info_file(base_path + "/metadata/disk_info.txt");
        if (info_file.is_open()) {
            info_file << "# Información del Disco SGBD" << std::endl;
            info_file << "created=" << getCurrentTimestamp() << std::endl;
            info_file << "base_path=" << base_path << std::endl;
            info_file << "total_capacity=" << disk_config.getTotalCapacity() << std::endl;
            info_file << "total_sectors=" << disk_config.getTotalSectors() << std::endl;
            info_file.close();
        }
    }

    /**
     * @brief Carga metadatos existentes
     */
    bool loadMetadata() {
        std::string config_path = base_path + "/metadata/disk_config.txt";
        
        if (!fs::exists(config_path)) {
            return false;
        }
        
        // Aquí se implementaría la carga de configuración desde archivo
        // Por simplicidad, usamos configuración por defecto
        disk_config = DiskConfig();
        
        return true;
    }

    /**
     * @brief Obtiene timestamp actual
     */
    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    /**
     * @brief Formatea bytes en unidades legibles
     */
    std::string formatBytes(long long bytes) const {
        if (bytes >= (1LL << 30)) {
            return std::to_string(bytes / (1LL << 30)) + " GB";
        } else if (bytes >= (1LL << 20)) {
            return std::to_string(bytes / (1LL << 20)) + " MB";
        } else if (bytes >= (1LL << 10)) {
            return std::to_string(bytes / (1LL << 10)) + " KB";
        }
        return std::to_string(bytes) + " bytes";
    }
};

#endif // FILESYSTEM_SIMULATOR_H