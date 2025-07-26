#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <string>
#include <functional>
#include <iostream>
#include <cstdint>

/**
 * @brief Funciones hash especializadas para Hash Extensible
 * 
 * Proporciona diferentes algoritmos de hash optimizados para:
 * - IMEI (strings numéricos largos)
 * - Strings generales
 * - Enteros
 * - Distribución uniforme
 */
class HashFunction {
public:
    /**
     * @brief Hash estándar usando std::hash
     */
    template<typename T>
    static size_t standardHash(const T& key) {
        return std::hash<T>{}(key);
    }
    
    /**
     * @brief Hash específico para IMEI (mejor distribución)
     */
    static size_t hashIMEI(const std::string& imei) {
        // Algoritmo FNV-1a optimizado para números largos
        const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        const uint64_t FNV_PRIME = 1099511628211ULL;
        
        uint64_t hash = FNV_OFFSET_BASIS;
        
        for (char c : imei) {
            hash ^= static_cast<uint64_t>(c);
            hash *= FNV_PRIME;
        }
        
        return static_cast<size_t>(hash);
    }
    
    /**
     * @brief Hash para strings con mejor distribución
     */
    static size_t hashString(const std::string& str) {
        // Algoritmo djb2 con variaciones
        uint64_t hash = 5381;
        
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
        }
        
        return static_cast<size_t>(hash);
    }
    
    /**
     * @brief Hash para enteros con multiplicación
     */
    static size_t hashInteger(int key) {
        // Multiplicación por número primo grande
        uint64_t hash = static_cast<uint64_t>(key);
        hash *= 2654435761ULL; // Número primo
        return static_cast<size_t>(hash);
    }
    
    /**
     * @brief Hash híbrido que detecta el tipo de dato
     */
    static size_t hybridHash(const std::string& key) {
        if (isNumeric(key)) {
            if (key.length() >= 14) {
                // Parece un IMEI
                return hashIMEI(key);
            } else {
                // Número regular
                try {
                    int num = std::stoi(key);
                    return hashInteger(num);
                } catch (...) {
                    return hashString(key);
                }
            }
        } else {
            // String normal
            return hashString(key);
        }
    }

    // ============================================================================
    // FUNCIONES PARA HASH EXTENSIBLE
    // ============================================================================
    
    /**
     * @brief Calcula hash con máscara para profundidad específica
     */
    static size_t hashWithDepth(const std::string& key, int depth) {
        size_t hash_value = hybridHash(key);
        size_t mask = (1ULL << depth) - 1;
        return hash_value & mask;
    }
    
    /**
     * @brief Obtiene los últimos N bits del hash
     */
    static size_t getLastNBits(const std::string& key, int n) {
        return hashWithDepth(key, n);
    }
    
    /**
     * @brief Verifica si dos claves van al mismo bucket
     */
    static bool sameBucket(const std::string& key1, const std::string& key2, int depth) {
        return hashWithDepth(key1, depth) == hashWithDepth(key2, depth);
    }
    
    /**
     * @brief Calcula la distribución de hash para análisis
     */
    static void analyzeDistribution(const std::vector<std::string>& keys, int depth) {
        std::cout << "\n📊 ANÁLISIS DE DISTRIBUCIÓN HASH:" << std::endl;
        std::cout << "Profundidad: " << depth << std::endl;
        std::cout << "Claves analizadas: " << keys.size() << std::endl;
        
        std::map<size_t, int> distribution;
        size_t total_buckets = 1ULL << depth;
        
        for (const auto& key : keys) {
            size_t bucket = hashWithDepth(key, depth);
            distribution[bucket]++;
        }
        
        std::cout << "Buckets utilizados: " << distribution.size() << "/" << total_buckets << std::endl;
        
        // Estadísticas de distribución
        int min_load = INT_MAX;
        int max_load = 0;
        double sum_squares = 0;
        
        for (size_t i = 0; i < total_buckets; i++) {
            int load = distribution[i];
            min_load = std::min(min_load, load);
            max_load = std::max(max_load, load);
            sum_squares += load * load;
        }
        
        double mean = static_cast<double>(keys.size()) / total_buckets;
        double variance = (sum_squares / total_buckets) - (mean * mean);
        double std_dev = std::sqrt(variance);
        
        std::cout << "Estadísticas de carga:" << std::endl;
        std::cout << "  Mínima: " << min_load << std::endl;
        std::cout << "  Máxima: " << max_load << std::endl;
        std::cout << "  Promedio: " << std::fixed << std::setprecision(2) << mean << std::endl;
        std::cout << "  Desviación estándar: " << std::fixed << std::setprecision(2) << std_dev << std::endl;
        
        // Factor de uniformidad (menor es mejor)
        double uniformity = std_dev / mean;
        std::cout << "  Factor de uniformidad: " << std::fixed << std::setprecision(3) << uniformity << std::endl;
        
        if (uniformity < 0.2) {
            std::cout << "  ✅ Distribución excelente" << std::endl;
        } else if (uniformity < 0.5) {
            std::cout << "  ✅ Distribución buena" << std::endl;
        } else if (uniformity < 1.0) {
            std::cout << "  ⚠️ Distribución aceptable" << std::endl;
        } else {
            std::cout << "  ❌ Distribución pobre" << std::endl;
        }
    }

    // ============================================================================
    // UTILIDADES Y DEBUGGING
    // ============================================================================
    
    /**
     * @brief Muestra información de hash para una clave
     */
    static void debugHash(const std::string& key, int max_depth = 4) {
        std::cout << "\n🔍 DEBUG HASH PARA CLAVE: '" << key << "'" << std::endl;
        std::cout << "Tipo detectado: " << (isNumeric(key) ? "Numérico" : "String") << std::endl;
        
        if (isNumeric(key) && key.length() >= 14) {
            std::cout << "Subtipo: IMEI" << std::endl;
        }
        
        size_t base_hash = hybridHash(key);
        std::cout << "Hash base: " << base_hash << " (0x" << std::hex << base_hash << std::dec << ")" << std::endl;
        
        std::cout << "Buckets por profundidad:" << std::endl;
        for (int depth = 0; depth <= max_depth; depth++) {
            size_t bucket = hashWithDepth(key, depth);
            size_t total_buckets = 1ULL << depth;
            std::cout << "  Profundidad " << depth << ": bucket " << bucket 
                      << " de " << total_buckets << " (" 
                      << std::fixed << std::setprecision(1) 
                      << (100.0 * bucket / total_buckets) << "%)" << std::endl;
        }
        
        // Mostrar representación binaria
        std::cout << "Bits del hash: ";
        for (int i = max_depth - 1; i >= 0; i--) {
            size_t bit = (base_hash >> i) & 1;
            std::cout << bit;
        }
        std::cout << std::endl;
    }
    
    /**
     * @brief Compara diferentes algoritmos de hash
     */
    static void compareHashAlgorithms(const std::string& key) {
        std::cout << "\n🆚 COMPARACIÓN DE ALGORITMOS HASH:" << std::endl;
        std::cout << "Clave: '" << key << "'" << std::endl;
        
        size_t std_hash = standardHash(key);
        size_t str_hash = hashString(key);
        size_t hyb_hash = hybridHash(key);
        
        std::cout << "std::hash: " << std_hash << std::endl;
        std::cout << "hashString: " << str_hash << std::endl;
        std::cout << "hybridHash: " << hyb_hash << std::endl;
        
        if (isNumeric(key) && key.length() >= 14) {
            size_t imei_hash = hashIMEI(key);
            std::cout << "hashIMEI: " << imei_hash << std::endl;
        }
        
        // Comparar distribución en primeros 4 bits
        std::cout << "\nPrimeros 4 bits:" << std::endl;
        std::cout << "std::hash: " << (std_hash & 15) << std::endl;
        std::cout << "hashString: " << (str_hash & 15) << std::endl;
        std::cout << "hybridHash: " << (hyb_hash & 15) << std::endl;
    }
    
    /**
     * @brief Genera claves de prueba para testing
     */
    static std::vector<std::string> generateTestKeys(int count = 1000) {
        std::vector<std::string> keys;
        keys.reserve(count);
        
        // Base IMEI
        std::string base_imei = "86801807023740";
        
        for (int i = 0; i < count; i++) {
            // Generar variaciones del IMEI
            std::string imei = base_imei + std::to_string(i % 100);
            if (imei.length() > 15) {
                imei = imei.substr(0, 15);
            }
            keys.push_back(imei);
        }
        
        return keys;
    }
    
    /**
     * @brief Test de calidad del hash
     */
    static void testHashQuality(int sample_size = 10000) {
        std::cout << "\n🧪 TEST DE CALIDAD DE HASH:" << std::endl;
        std::cout << "Generando " << sample_size << " claves de prueba..." << std::endl;
        
        auto test_keys = generateTestKeys(sample_size);
        
        for (int depth = 1; depth <= 4; depth++) {
            std::cout << "\n--- Profundidad " << depth << " ---" << std::endl;
            analyzeDistribution(test_keys, depth);
        }
    }

private:
    /**
     * @brief Verifica si un string es numérico
     */
    static bool isNumeric(const std::string& str) {
        if (str.empty()) return false;
        
        for (char c : str) {
            if (!std::isdigit(c)) return false;
        }
        
        return true;
    }
};

// ============================================================================
// CLASE PARA HASH PERSONALIZADO (OPCIONAL)
// ============================================================================

/**
 * @brief Hash personalizable para diferentes casos de uso
 */
template<typename KeyType>
class CustomHashFunction {
private:
    std::function<size_t(const KeyType&)> hash_func;
    std::string algorithm_name;

public:
    CustomHashFunction(std::function<size_t(const KeyType&)> func, 
                      const std::string& name = "Custom") 
        : hash_func(func), algorithm_name(name) {}
    
    size_t operator()(const KeyType& key) const {
        return hash_func(key);
    }
    
    const std::string& getName() const {
        return algorithm_name;
    }
    
    void benchmark(const std::vector<KeyType>& keys) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& key : keys) {
            volatile size_t hash = hash_func(key); // volatile para evitar optimización
            (void)hash; // Suprimir warning
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "⏱️ " << algorithm_name << ": " << duration.count() 
                  << " μs para " << keys.size() << " hashes" << std::endl;
        std::cout << "   Promedio: " << (double)duration.count() / keys.size() 
                  << " μs/hash" << std::endl;
    }
};

// ============================================================================
// FACTORY DE FUNCIONES HASH
// ============================================================================

/**
 * @brief Factory para crear diferentes tipos de funciones hash
 */
class HashFunctionFactory {
public:
    enum class HashType {
        STANDARD,
        STRING_OPTIMIZED,
        IMEI_OPTIMIZED,
        HYBRID,
        CUSTOM
    };
    
    static std::function<size_t(const std::string&)> create(HashType type) {
        switch (type) {
            case HashType::STANDARD:
                return [](const std::string& key) { return HashFunction::standardHash(key); };
            
            case HashType::STRING_OPTIMIZED:
                return [](const std::string& key) { return HashFunction::hashString(key); };
            
            case HashType::IMEI_OPTIMIZED:
                return [](const std::string& key) { return HashFunction::hashIMEI(key); };
            
            case HashType::HYBRID:
                return [](const std::string& key) { return HashFunction::hybridHash(key); };
            
            default:
                return [](const std::string& key) { return HashFunction::hybridHash(key); };
        }
    }
    
    static std::string getTypeName(HashType type) {
        switch (type) {
            case HashType::STANDARD: return "Standard";
            case HashType::STRING_OPTIMIZED: return "String Optimized";
            case HashType::IMEI_OPTIMIZED: return "IMEI Optimized";
            case HashType::HYBRID: return "Hybrid";
            case HashType::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
};

#endif // HASH_FUNCTION_H