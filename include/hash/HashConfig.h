#ifndef HASH_CONFIG_H
#define HASH_CONFIG_H

#include <iostream>
#include <cstdint>

/**
 * @brief Configuración para el sistema de Hash Extensible
 * 
 * CONCEPTOS APLICADOS:
 * - Configuration Pattern: Centralizar parámetros
 * - Defensive Programming: Validación de límites
 * - Performance Tuning: Parámetros ajustables
 */
struct HashConfig {
    uint32_t bucket_capacity;        // Elementos por bucket
    uint32_t initial_global_depth;   // Profundidad inicial
    bool enable_statistics;          // Activar métricas
    bool enable_cache;               // Cache de páginas frecuentes
    uint32_t max_cache_size;         // Límite de cache
    bool auto_flush;                 // Flush automático
    
    // Límites de seguridad
    static constexpr uint32_t MIN_BUCKET_CAPACITY = 2;
    static constexpr uint32_t MAX_BUCKET_CAPACITY = 32;
    static constexpr uint32_t MAX_GLOBAL_DEPTH = 10;
    
    HashConfig(uint32_t capacity = 6,           // Capacidad conservadora
               uint32_t depth = 1,              // Depth inicial mínimo
               bool stats = true,               // Estadísticas habilitadas
               bool cache = true,               // Cache habilitado
               uint32_t cache_size = 10,        // Cache pequeño
               bool flush = false)              // Flush manual
        : bucket_capacity(ValidateCapacity(capacity))
        , initial_global_depth(ValidateDepth(depth))
        , enable_statistics(stats)
        , enable_cache(cache)
        , max_cache_size(cache_size)
        , auto_flush(flush) {}
    
    // Validación de parámetros
    static uint32_t ValidateCapacity(uint32_t capacity) {
        if (capacity < MIN_BUCKET_CAPACITY) return MIN_BUCKET_CAPACITY;
        if (capacity > MAX_BUCKET_CAPACITY) return MAX_BUCKET_CAPACITY;
        return capacity;
    }
    
    static uint32_t ValidateDepth(uint32_t depth) {
        if (depth == 0) return 1;  // Mínimo 1
        if (depth > MAX_GLOBAL_DEPTH) return MAX_GLOBAL_DEPTH;
        return depth;
    }
    
    // Configuraciones predefinidas
    static HashConfig Conservative() {
        return HashConfig(8, 1, true, true, 5, false);  // Muy conservador
    }
    
    static HashConfig Balanced() {
        return HashConfig(6, 2, true, true, 10, false); // Balanceado
    }
    
    static HashConfig Performance() {
        return HashConfig(4, 3, false, true, 20, true); // Rendimiento
    }
    
    /**
     * @brief Configuración conservadora para evitar explosión
     */
    static HashConfig GetConservativeConfig() {
        return HashConfig(4, 1, true, false, 5, false); // Muy conservador
    }
    
    void Display() const {
        std::cout << "=== CONFIGURACIÓN HASH EXTENSIBLE ===" << std::endl;
        std::cout << "Capacidad por bucket: " << bucket_capacity << std::endl;
        std::cout << "Global depth inicial: " << initial_global_depth << std::endl;
        std::cout << "Estadísticas: " << (enable_statistics ? "Sí" : "No") << std::endl;
        std::cout << "Cache habilitado: " << (enable_cache ? "Sí" : "No") << std::endl;
        std::cout << "Tamaño de cache: " << max_cache_size << std::endl;
        std::cout << "Auto-flush: " << (auto_flush ? "Sí" : "No") << std::endl;
    }
};

#endif // HASH_CONFIG_H
