#ifndef DISK_STRUCTURE_H
#define DISK_STRUCTURE_H

#include <vector>
#include <iostream>
#include <cstring>
#include <chrono>

// Constantes de configuración del disco
const int BYTES_POR_SECTOR = 512;
const int SECTORES_POR_BLOQUE = 8;  // 4KB por bloque
const int BYTES_POR_BLOQUE = BYTES_POR_SECTOR * SECTORES_POR_BLOQUE;

// Estructura para representar una dirección física en el disco
struct DireccionFisica {
    int plato;
    int superficie;  // 0 = superior, 1 = inferior
    int pista;
    int sector;
    
    DireccionFisica() : plato(0), superficie(0), pista(0), sector(0) {}
    
    DireccionFisica(int p, int sup, int pis, int sec) 
        : plato(p), superficie(sup), pista(pis), sector(sec) {}
    
    void imprimir() const {
        std::cout << "Plato: " << plato 
                  << ", Superficie: " << superficie 
                  << ", Pista: " << pista 
                  << ", Sector: " << sector << std::endl;
    }
};

// Clase para representar un Sector físico
class Sector {
private:
    char datos[BYTES_POR_SECTOR];
    bool ocupado;
    
public:
    Sector() : ocupado(false) {
        memset(datos, 0, BYTES_POR_SECTOR);
    }
    
    bool escribir(const char* data, int size) {
        if (size > BYTES_POR_SECTOR) return false;
        memcpy(datos, data, size);
        ocupado = true;
        return true;
    }
    
    bool leer(char* buffer, int size) {
        if (size > BYTES_POR_SECTOR) return false;
        memcpy(buffer, datos, size);
        return true;
    }
    
    bool estaOcupado() const { return ocupado; }
    void limpiar() { 
        memset(datos, 0, BYTES_POR_SECTOR); 
        ocupado = false;
    }
    
    int espacioLibre() const {
        return ocupado ? 0 : BYTES_POR_SECTOR;
    }
};

// Clase para representar una Pista
class Pista {
private:
    std::vector<Sector> sectores;
    int numeroSectores;
    
public:
    Pista(int numSectores) : numeroSectores(numSectores) {
        sectores.resize(numSectores);
    }
    
    Sector* obtenerSector(int numSector) {
        if (numSector >= 0 && numSector < numeroSectores) {
            return &sectores[numSector];
        }
        return nullptr;
    }
    
    int sectoresLibres() const {
        int libres = 0;
        for (const auto& sector : sectores) {
            if (!sector.estaOcupado()) libres++;
        }
        return libres;
    }
    
    int getNumeroSectores() const { return numeroSectores; }
};

// Clase para representar una Superficie
class Superficie {
private:
    std::vector<Pista> pistas;
    int numeroPistas;
    int sectoresPorPista;
    
public:
    Superficie(int numPistas, int secPorPista) 
        : numeroPistas(numPistas), sectoresPorPista(secPorPista) {
        for (int i = 0; i < numPistas; i++) {
            pistas.emplace_back(secPorPista);
        }
    }
    
    Pista* obtenerPista(int numPista) {
        if (numPista >= 0 && numPista < numeroPistas) {
            return &pistas[numPista];
        }
        return nullptr;
    }
    
    int pistasLibres() const {
        int libres = 0;
        for (const auto& pista : pistas) {
            if (pista.sectoresLibres() > 0) libres++;
        }
        return libres;
    }
    
    int getNumeroPistas() const { return numeroPistas; }
    int getSectoresPorPista() const { return sectoresPorPista; }
};

// Clase para representar un Plato
class Plato {
private:
    Superficie superficieSuperior;
    Superficie superficieInferior;
    
public:
    Plato(int numPistas, int sectoresPorPista) 
        : superficieSuperior(numPistas, sectoresPorPista),
          superficieInferior(numPistas, sectoresPorPista) {}
    
    Superficie* obtenerSuperficie(int numSuperficie) {
        if (numSuperficie == 0) return &superficieSuperior;
        if (numSuperficie == 1) return &superficieInferior;
        return nullptr;
    }
    
    int superficiesConEspacio() const {
        int count = 0;
        if (superficieSuperior.pistasLibres() > 0) count++;
        if (superficieInferior.pistasLibres() > 0) count++;
        return count;
    }
};

// Clase principal del Disco
class Disco {
private:
    std::vector<Plato> platos;
    int numeroPlatos;
    int pistasPorSuperficie;
    int sectoresPorPista;
    
    // Estadísticas
    int totalSectores;
    int sectoresOcupados;
    
public:
    Disco(int numPlatos, int numPistas, int numSectores) 
        : numeroPlatos(numPlatos), 
          pistasPorSuperficie(numPistas), 
          sectoresPorPista(numSectores),
          sectoresOcupados(0) {
        
        // Crear los platos
        for (int i = 0; i < numPlatos; i++) {
            platos.emplace_back(numPistas, numSectores);
        }
        
        // Calcular total de sectores
        totalSectores = numPlatos * 2 * numPistas * numSectores;
    }
    
    // Acceder a un sector específico por dirección física
    Sector* obtenerSector(const DireccionFisica& direccion) {
        if (direccion.plato >= numeroPlatos) return nullptr;
        
        Plato* plato = &platos[direccion.plato];
        Superficie* superficie = plato->obtenerSuperficie(direccion.superficie);
        if (!superficie) return nullptr;
        
        Pista* pista = superficie->obtenerPista(direccion.pista);
        if (!pista) return nullptr;
        
        return pista->obtenerSector(direccion.sector);
    }
    
    // Encontrar el siguiente sector libre
    DireccionFisica encontrarSectorLibre() {
        for (int p = 0; p < numeroPlatos; p++) {
            for (int sup = 0; sup < 2; sup++) {
                Superficie* superficie = platos[p].obtenerSuperficie(sup);
                for (int pis = 0; pis < pistasPorSuperficie; pis++) {
                    Pista* pista = superficie->obtenerPista(pis);
                    for (int sec = 0; sec < sectoresPorPista; sec++) {
                        Sector* sector = pista->obtenerSector(sec);
                        if (sector && !sector->estaOcupado()) {
                            return DireccionFisica(p, sup, pis, sec);
                        }
                    }
                }
            }
        }
        return DireccionFisica(-1, -1, -1, -1); // No hay espacio
    }
    
    // Mostrar información del disco
    void mostrarInfo() const {
        std::cout << "\n=== INFORMACIÓN DEL DISCO ===" << std::endl;
        std::cout << "Número de platos: " << numeroPlatos << std::endl;
        std::cout << "Pistas por superficie: " << pistasPorSuperficie << std::endl;
        std::cout << "Sectores por pista: " << sectoresPorPista << std::endl;
        std::cout << "Total de sectores: " << totalSectores << std::endl;
        std::cout << "Capacidad total: " << (totalSectores * BYTES_POR_SECTOR) / (1024*1024) << " MB" << std::endl;
        std::cout << "Sectores ocupados: " << sectoresOcupados << std::endl;
        std::cout << "Espacio utilizado: " << (sectoresOcupados * 100.0 / totalSectores) << "%" << std::endl;
    }
    
    // Getters
    int getNumeroPlatos() const { return numeroPlatos; }
    int getPistasPorSuperficie() const { return pistasPorSuperficie; }
    int getSectoresPorPista() const { return sectoresPorPista; }
    int getTotalSectores() const { return totalSectores; }
    
    void incrementarSectoresOcupados() { sectoresOcupados++; }
    void decrementarSectoresOcupados() { if (sectoresOcupados > 0) sectoresOcupados--; }
};

#endif // DISK_STRUCTURE_H