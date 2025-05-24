#include "timer.h"
#include <thread>  // Para simular operaciones que toman tiempo

int main() {
    std::cout << "=== SGBD Project - Paso 1: Probando Timer ===\n" << std::endl;
    
    // Crear un objeto Timer
    Timer cronometro;
    
    // Probar operación rápida
    std::cout << "🚀 Ejecutando operación rápida..." << std::endl;
    cronometro.start();
    
    // Simular trabajo rápido (1 millisegundo)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    cronometro.printElapsedTime("⚡ Operación rápida");
    
    std::cout << std::endl;
    
    // Probar operación lenta
    std::cout << "🐌 Ejecutando operación lenta..." << std::endl;
    cronometro.start();
    
    // Simular trabajo lento (100 millisegundos)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    cronometro.printElapsedTime("🐢 Operación lenta");
    
    std::cout << "\n Timer funcionando correctamente!" << std::endl;
    
    return 0;
}