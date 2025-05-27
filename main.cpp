#include "disk_structure.h"
#include <iostream>
#include <iomanip>

using namespace std;

void pruebaEscrituraLectura(Disco& disco) {
    cout << "\n=== PRUEBA DE ESCRITURA Y LECTURA ===" << endl;
    
    // Encontrar un sector libre
    DireccionFisica dir = disco.encontrarSectorLibre();
    cout << "Sector libre encontrado en: ";
    dir.imprimir();
    
    // Escribir datos en el sector
    Sector* sector = disco.obtenerSector(dir);
    if (sector) {
        const char* datos = "Hola, este es mi primer dato en el disco!";
        if (sector->escribir(datos, strlen(datos) + 1)) {
            cout << "Datos escritos exitosamente." << endl;
            disco.incrementarSectoresOcupados();
        }
        
        // Leer los datos
        char buffer[BYTES_POR_SECTOR];
        if (sector->leer(buffer, BYTES_POR_SECTOR)) {
            cout << "Datos leídos: " << buffer << endl;
        }
    }
}

void pruebaMultiplesEscrituras(Disco& disco) {
    cout << "\n=== PRUEBA DE MÚLTIPLES ESCRITURAS ===" << endl;
    
    const int NUM_ESCRITURAS = 10;
    vector<DireccionFisica> direcciones;
    
    // Escribir múltiples sectores
    for (int i = 0; i < NUM_ESCRITURAS; i++) {
        DireccionFisica dir = disco.encontrarSectorLibre();
        if (dir.plato == -1) {
            cout << "No hay más espacio libre!" << endl;
            break;
        }
        
        Sector* sector = disco.obtenerSector(dir);
        if (sector) {
            string datos = "Registro #" + to_string(i);
            sector->escribir(datos.c_str(), datos.length() + 1);
            disco.incrementarSectoresOcupados();
            direcciones.push_back(dir);
            
            cout << "Escrito registro " << i << " en ";
            dir.imprimir();
        }
    }
    
    // Leer algunos de los sectores escritos
    cout << "\nLeyendo algunos sectores escritos:" << endl;
    for (int i = 0; i < min(3, (int)direcciones.size()); i++) {
        Sector* sector = disco.obtenerSector(direcciones[i]);
        if (sector) {
            char buffer[BYTES_POR_SECTOR];
            sector->leer(buffer, BYTES_POR_SECTOR);
            cout << "Contenido en ";
            direcciones[i].imprimir();
            cout << "  -> " << buffer << endl;
        }
    }
}

void mostrarMapaDisco(Disco& disco) {
    cout << "\n=== MAPA DEL DISCO (Primeras 3 pistas) ===" << endl;
    cout << "O = Ocupado, . = Libre" << endl;
    
    for (int plato = 0; plato < min(2, disco.getNumeroPlatos()); plato++) {
        for (int sup = 0; sup < 2; sup++) {
            cout << "\nPlato " << plato << ", Superficie " << sup << ":" << endl;
            
            for (int pista = 0; pista < min(3, disco.getPistasPorSuperficie()); pista++) {
                cout << "Pista " << setw(2) << pista << ": ";
                
                for (int sec = 0; sec < disco.getSectoresPorPista(); sec++) {
                    DireccionFisica dir(plato, sup, pista, sec);
                    Sector* sector = disco.obtenerSector(dir);
                    
                    if (sector && sector->estaOcupado()) {
                        cout << "O";
                    } else {
                        cout << ".";
                    }
                }
                cout << endl;
            }
        }
    }
}

int main() {
    // Crear un disco pequeño para pruebas
    // 2 platos, 10 pistas por superficie, 20 sectores por pista
    Disco disco(2, 10, 20);
    
    // Mostrar información inicial
    disco.mostrarInfo();
    
    // Realizar pruebas
    pruebaEscrituraLectura(disco);
    pruebaMultiplesEscrituras(disco);
    
    // Mostrar información actualizada
    disco.mostrarInfo();
    
    // Mostrar mapa visual del disco
    mostrarMapaDisco(disco);
    
    return 0;
}