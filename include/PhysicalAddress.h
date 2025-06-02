#ifndef PHYSICAL_ADDRESS_H
#define PHYSICAL_ADDRESS_H

#include <iostream>
#include <string>
#include <sstream>

/**
 * @brief Representa una dirección física en el disco simulado
 * 
 * Estructura jerárquica: Plato -> Superficie -> Pista -> Sector
 * Basado en la arquitectura de discos magnéticos del capítulo 13
 */
class PhysicalAddress {
private:
    int platter;    // Número de plato (0-n)
    int surface;    // Número de superficie (0-1, arriba/abajo del plato)
    int track;      // Número de pista/cilindro (0-n)
    int sector;     // Número de sector (0-n)

public:
    /**
     * @brief Constructor por defecto
     */
    PhysicalAddress() : platter(0), surface(0), track(0), sector(0) {}

    /**
     * @brief Constructor con parámetros
     */
    PhysicalAddress(int p, int s, int t, int sec) 
        : platter(p), surface(s), track(t), sector(sec) {}

    // Getters
    int getPlatter() const { return platter; }
    int getSurface() const { return surface; }
    int getTrack() const { return track; }
    int getSector() const { return sector; }

    // Setters
    void setPlatter(int p) { platter = p; }
    void setSurface(int s) { surface = s; }
    void setTrack(int t) { track = t; }
    void setSector(int sec) { sector = sec; }

    /**
     * @brief Convierte la dirección a string para identificación
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "P" << platter << "_S" << surface << "_T" << track << "_SEC" << sector;
        return oss.str();
    }

    /**
     * @brief Obtiene la ruta del directorio para esta dirección
     */
    std::string getDirectoryPath() const {
        std::ostringstream oss;
        oss << "platter_" << platter << "/surface_" << surface << "/track_" << track;
        return oss.str();
    }

    /**
     * @brief Obtiene el nombre del archivo del sector
     */
    std::string getSectorFileName() const {
        return "sector_" + std::to_string(sector) + ".txt";
    }

    /**
     * @brief Operador de comparación para ordenamiento
     */
    bool operator<(const PhysicalAddress& other) const {
        if (platter != other.platter) return platter < other.platter;
        if (surface != other.surface) return surface < other.surface;
        if (track != other.track) return track < other.track;
        return sector < other.sector;
    }

    /**
     * @brief Operador de igualdad
     */
    bool operator==(const PhysicalAddress& other) const {
        return platter == other.platter && 
               surface == other.surface && 
               track == other.track && 
               sector == other.sector;
    }

    /**
     * @brief Imprime la dirección en formato legible
     */
    friend std::ostream& operator<<(std::ostream& os, const PhysicalAddress& addr) {
        os << "(" << addr.platter << "," << addr.surface << "," 
           << addr.track << "," << addr.sector << ")";
        return os;
    }
};

#endif // PHYSICAL_ADDRESS_H