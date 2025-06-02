#ifndef DISK_CONFIG_H
#define DISK_CONFIG_H

#include <iostream>
#include <fstream>
#include <string>

/**
 * @brief Configuración física del disco simulado
 * 
 * Define los parámetros físicos del disco basados en el modelo
 * de discos magnéticos del capítulo 13 (Megatron 747 como referencia)
 */
class DiskConfig {
private:
    int num_platters;           // Número de platos
    int surfaces_per_platter;   // Superficies por plato (generalmente 2)
    int tracks_per_surface;     // Pistas por superficie
    int sectors_per_track;      // Sectores por pista
    int bytes_per_sector;       // Bytes por sector
    
    // Parámetros de rendimiento (en milisegundos)
    double seek_time_ms;        // Tiempo de búsqueda promedio
    double rotational_latency_ms;  // Latencia rotacional promedio
    double transfer_time_ms;    // Tiempo de transferencia por sector

public:
    /**
     * @brief Constructor por defecto - Configuración tipo Megatron 747
     */
    DiskConfig() 
        : num_platters(4)
        , surfaces_per_platter(2)
        , tracks_per_surface(65536)
        , sectors_per_track(256)
        , bytes_per_sector(4096)
        , seek_time_ms(6.46)
        , rotational_latency_ms(4.17)
        , transfer_time_ms(0.13)
    {
    }

    /**
     * @brief Constructor personalizado
     */
    DiskConfig(int platters, int surfaces, int tracks, int sectors, int bytes_sector)
        : num_platters(platters)
        , surfaces_per_platter(surfaces)
        , tracks_per_surface(tracks)
        , sectors_per_track(sectors)
        , bytes_per_sector(bytes_sector)
        , seek_time_ms(6.46)
        , rotational_latency_ms(4.17)
        , transfer_time_ms(0.13)
    {
    }

    // Getters
    int getNumPlatters() const { return num_platters; }
    int getSurfacesPerPlatter() const { return surfaces_per_platter; }
    int getTracksPerSurface() const { return tracks_per_surface; }
    int getSectorsPerTrack() const { return sectors_per_track; }
    int getBytesPerSector() const { return bytes_per_sector; }
    double getSeekTime() const { return seek_time_ms; }
    double getRotationalLatency() const { return rotational_latency_ms; }
    double getTransferTime() const { return transfer_time_ms; }

    /**
     * @brief Calcula la capacidad total del disco en bytes
     */
    long long getTotalCapacity() const {
        return static_cast<long long>(num_platters) * 
               surfaces_per_platter * 
               tracks_per_surface * 
               sectors_per_track * 
               bytes_per_sector;
    }

    /**
     * @brief Calcula el número total de sectores
     */
    long long getTotalSectors() const {
        return static_cast<long long>(num_platters) * 
               surfaces_per_platter * 
               tracks_per_surface * 
               sectors_per_track;
    }

    /**
     * @brief Calcula el número total de superficies
     */
    int getTotalSurfaces() const {
        return num_platters * surfaces_per_platter;
    }

    /**
     * @brief Formatea la capacidad en unidades legibles
     */
    std::string getFormattedCapacity() const {
        long long bytes = getTotalCapacity();
        
        if (bytes >= (1LL << 40)) {
            return std::to_string(bytes / (1LL << 40)) + " TB";
        } else if (bytes >= (1LL << 30)) {
            return std::to_string(bytes / (1LL << 30)) + " GB";
        } else if (bytes >= (1LL << 20)) {
            return std::to_string(bytes / (1LL << 20)) + " MB";
        } else if (bytes >= (1LL << 10)) {
            return std::to_string(bytes / (1LL << 10)) + " KB";
        }
        return std::to_string(bytes) + " bytes";
    }

    /**
     * @brief Muestra la configuración del disco
     */
    void displayConfig() const {
        std::cout << "\n=== CONFIGURACIÓN DEL DISCO ===" << std::endl;
        std::cout << "Platos: " << num_platters << std::endl;
        std::cout << "Superficies por plato: " << surfaces_per_platter << std::endl;
        std::cout << "Pistas por superficie: " << tracks_per_surface << std::endl;
        std::cout << "Sectores por pista: " << sectors_per_track << std::endl;
        std::cout << "Bytes por sector: " << bytes_per_sector << std::endl;
        std::cout << "Capacidad total: " << getFormattedCapacity() << std::endl;
        std::cout << "Total de sectores: " << getTotalSectors() << std::endl;
        std::cout << "\n=== PARÁMETROS DE RENDIMIENTO ===" << std::endl;
        std::cout << "Tiempo de búsqueda promedio: " << seek_time_ms << " ms" << std::endl;
        std::cout << "Latencia rotacional promedio: " << rotational_latency_ms << " ms" << std::endl;
        std::cout << "Tiempo de transferencia: " << transfer_time_ms << " ms/sector" << std::endl;
    }

    /**
     * @brief Guarda la configuración en un archivo
     */
    bool saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << "# Configuración del Disco SGBD" << std::endl;
        file << "num_platters=" << num_platters << std::endl;
        file << "surfaces_per_platter=" << surfaces_per_platter << std::endl;
        file << "tracks_per_surface=" << tracks_per_surface << std::endl;
        file << "sectors_per_track=" << sectors_per_track << std::endl;
        file << "bytes_per_sector=" << bytes_per_sector << std::endl;
        file << "seek_time_ms=" << seek_time_ms << std::endl;
        file << "rotational_latency_ms=" << rotational_latency_ms << std::endl;
        file << "transfer_time_ms=" << transfer_time_ms << std::endl;
        
        file.close();
        return true;
    }

    /**
     * @brief Valida que la configuración sea consistente
     */
    bool isValid() const {
        return num_platters > 0 && 
               surfaces_per_platter > 0 && 
               tracks_per_surface > 0 && 
               sectors_per_track > 0 && 
               bytes_per_sector > 0;
    }
};

#endif // DISK_CONFIG_H