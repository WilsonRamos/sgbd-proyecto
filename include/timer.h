#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <iostream>

// Clase simple para medir tiempo de ejecución
class Timer {
private:
    // Punto de inicio del tiempo
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    // Iniciar el cronómetro
    void start();
    
    // Obtener el tiempo transcurrido en milisegundos
    double getElapsedTime();
    
    // Mostrar el tiempo transcurrido
    void printElapsedTime(const std::string& operation_name);
};

#endif // TIMER_H