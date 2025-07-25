#include <iostream>
#include <exception>
#include "../include/SGBDDistributed.h"

/**
 * @brief Main para ejecutar el sistema distribuido de forma independiente
 * 
 * Este archivo permite ejecutar solo el sistema distribuido sin 
 * necesidad del SGBD principal completo.
 */
int main() {
    std::cout << "🌟 SISTEMA SGBD DISTRIBUIDO STANDALONE 🌟" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Sistema especializado con:" << std::endl;
    std::cout << "✅ Hash Extensible para IMEI (Servidor S1)" << std::endl;
    std::cout << "✅ B+ Tree para Timestamp (Servidor S2)" << std::endl;
    std::cout << "✅ Routing automático vs manual" << std::endl;
    std::cout << "✅ Consultas SQL interactivas" << std::endl;
    std::cout << "✅ Dataset GPS completo" << std::endl;
    
    try {
        // Crear sistema distribuido con dataset GPS
        SGBDDistributed distributed_system("data/data-GPS.csv");
        
        // Ejecutar sistema interactivo
        distributed_system.run();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error fatal: " << e.what() << std::endl;
        std::cerr << "💡 Posibles soluciones:" << std::endl;
        std::cerr << "   1. Verificar que data/data-GPS.csv existe" << std::endl;
        std::cerr << "   2. Compilar con: make distributed" << std::endl;
        std::cerr << "   3. Ejecutar desde directorio raíz del proyecto" << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Error desconocido en el sistema" << std::endl;
        return 2;
    }
    
    std::cout << "\n✨ Sistema distribuido finalizado correctamente" << std::endl;
    return 0;
}