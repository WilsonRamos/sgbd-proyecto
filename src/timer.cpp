#include "timer.h"

// Iniciar el cronómetro
void Timer::start() {
    start_time = std::chrono::high_resolution_clock::now();
}

// Calcular tiempo transcurrido en milisegundos
double Timer::getElapsedTime() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    return duration.count() / 1000.0; // Convertir a milisegundos
}

// Mostrar el tiempo de una operación
void Timer::printElapsedTime(const std::string& operation_name) {
    double elapsed = getElapsedTime();
    std::cout << operation_name << " completada en " << elapsed << " ms" << std::endl;
}