#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <string>
#include <functional>
#include <iostream>
#include <iomanip>
#include <sstream>

/**
 * @brief HashFunction - Función hash especializada para Hash Extensible
 * 
 * Implementa funciones hash específicas para el contexto educativo:
 * - Hash para IMEI (strings numéricos largos)
 * - Hash para timestamps (strings de fecha/hora)
 * - Funciones auxiliares para análisis de distribución
 * - Visualización de bits para propósitos educativos
 */
class HashFunction {
public:
    /**
     * @brief Función hash principal para strings (compatible con std::hash)
     */
    static size_t hash(const std::string& key) {
        return std::hash<std::string>{}(key);
    }

    /**
     * @brief Hash especializado para IMEI (15 dígitos)
     * 
     * Los IMEI tienen estructura específica:
     * - 8 primeros dígitos: Type Allocation Code (TAC)
     * - 6 siguientes: Serial Number
     * - 1 último: Check digit
     * 
     * Esta función hash aprovecha esta estructura para mejor distribución
     */
    static size_t hashIMEI(const std::string& imei) {
        if (imei.length() != 15) {
            // Fallback a hash estándar si no es IMEI válido
            return hash(imei);
        }

        // Combinar diferentes partes del IMEI
        size_t tac_hash = std::hash<std::string>{}(imei.substr(0, 8));    // TAC
        size_t serial_hash = std::hash<std::string>{}(imei.substr(8, 6)); // Serial
        size_t check_hash = std::hash<char>{}(imei[14]);                   // Check digit

        // Combinar usando XOR y shifts para mejor distribución
        return tac_hash ^ (serial_hash << 8) ^ (check_hash << 16);
    }

    /**
     * @brief Hash especializado para timestamps
     */
    static size_t hashTimestamp(const std::string& timestamp) {
        // Para timestamps en formato: "YYYY-MM-DD HH:MM:SS"
        if (timestamp.length() >= 19) {
            // Extraer componentes de tiempo
            std::string date_part = timestamp.substr(0, 10);     // YYYY-MM-DD
            std::string time_part = timestamp.substr(11, 8);     // HH:MM:SS
            
            size_t date_hash = hash(date_part);
            size_t time_hash = hash(time_part);
            
            return date_hash ^ (time_hash << 12);
        }
        
        return hash(timestamp);
    }

    /**
     * @brief Obtiene los últimos 'depth' bits de un valor hash
     */
    static size_t getBits(size_t hash_value, int depth) {
        if (depth <= 0) return 0;
        if (depth >= 64) return hash_value;
        
        size_t mask = (1ULL << depth) - 1;
        return hash_value & mask;
    }

    /**
     * @brief Convierte valor hash a representación binaria (educativo)
     */
    static std::string toBinaryString(size_t hash_value, int bits = 8) {
        std::string binary;
        for (int i = bits - 1; i >= 0; i--) {
            binary += ((hash_value >> i) & 1) ? '1' : '0';
        }
        return binary;
    }

    /**
     * @brief Análisis de distribución de hash (educativo)
     */
    static void analyzeHashDistribution(const std::vector<std::string>& keys, int depth = 3) {
        std::cout << "\n🔍 ANÁLISIS DE DISTRIBUCIÓN HASH (Depth = " << depth << "):" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;

        std::map<size_t, int> bucket_distribution;
        size_t total_buckets = 1ULL << depth;

        // Contar distribución
        for (const auto& key : keys) {
            size_t hash_val = hash(key);
            size_t bucket_index = getBits(hash_val, depth);
            bucket_distribution[bucket_index]++;
        }

        // Mostrar distribución
        std::cout << "Bucket | Count | Percentage | Binary | Hash Examples" << std::endl;
        std::cout << "-------|-------|------------|--------|---------------" << std::endl;

        for (size_t i = 0; i < total_buckets; i++) {
            int count = bucket_distribution[i];
            double percentage = (count * 100.0) / keys.size();
            std::string binary = toBinaryString(i, depth);

            std::cout << std::setw(6) << i << " | ";
            std::cout << std::setw(5) << count << " | ";
            std::cout << std::setw(9) << std::fixed << std::setprecision(1) << percentage << "% | ";
            std::cout << std::setw(6) << binary << " | ";

            // Mostrar hasta 3 ejemplos de claves que van a este bucket
            int examples_shown = 0;
            for (const auto& key : keys) {
                if (examples_shown >= 3) break;
                
                size_t hash_val = hash(key);
                if (getBits(hash_val, depth) == i) {
                    if (examples_shown > 0) std::cout << ", ";
                    std::cout << key.substr(0, 8) << "...";
                    examples_shown++;
                }
            }
            std::cout << std::endl;
        }

        // Estadísticas de distribución
        std::cout << "\n📊 ESTADÍSTICAS:" << std::endl;
        double expected_per_bucket = static_cast<double>(keys.size()) / total_buckets;
        double variance = 0.0;

        for (size_t i = 0; i < total_buckets; i++) {
            int count = bucket_distribution[i];
            double diff = count - expected_per_bucket;
            variance += diff * diff;
        }
        variance /= total_buckets;

        std::cout << "   Keys totales: " << keys.size() << std::endl;
        std::cout << "   Buckets totales: " << total_buckets << std::endl;
        std::cout << "   Promedio por bucket: " << std::fixed << std::setprecision(2) << expected_per_bucket << std::endl;
        std::cout << "   Varianza: " << std::fixed << std::setprecision(2) << variance << std::endl;
        std::cout << "   Desviación estándar: " << std::fixed << std::setprecision(2) << std::sqrt(variance) << std::endl;
    }

    /**
     * @brief Muestra información de hash para una clave específica (educativo)
     */
    static void showHashInfo(const std::string& key, int max_depth = 5) {
        size_t hash_val = hash(key);
        
        std::cout << "\n🔍 INFORMACIÓN DE HASH PARA: '" << key << "'" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;
        std::cout << "Hash value: " << hash_val << std::endl;
        std::cout << "Hash hex: 0x" << std::hex << hash_val << std::dec << std::endl;
        std::cout << "Hash binary (64 bits): " << toBinaryString(hash_val, 64) << std::endl;
        std::cout << std::endl;

        std::cout << "Bucket assignment por depth:" << std::endl;
        std::cout << "Depth | Bucket | Binary | Hex" << std::endl;
        std::cout << "------|--------|--------|----" << std::endl;

        for (int depth = 1; depth <= max_depth; depth++) {
            size_t bucket = getBits(hash_val, depth);
            std::string binary = toBinaryString(bucket, depth);
            
            std::cout << std::setw(5) << depth << " | ";
            std::cout << std::setw(6) << bucket << " | ";
            std::cout << std::setw(6) << binary << " | ";
            std::cout << "0x" << std::hex << bucket << std::dec << std::endl;
        }
    }

    /**
     * @brief Predictor de splits (educativo)
     */
    static bool willCauseDirectorySplit(const std::vector<std::string>& current_keys, 
                                       const std::string& new_key, 
                                       int global_depth, 
                                       int bucket_capacity) {
        // Determinar bucket donde iría la nueva clave
        size_t new_key_hash = hash(new_key);
        size_t bucket_index = getBits(new_key_hash, global_depth);

        // Contar cuántas claves actuales van al mismo bucket
        int current_count = 0;
        for (const auto& key : current_keys) {
            size_t key_hash = hash(key);
            if (getBits(key_hash, global_depth) == bucket_index) {
                current_count++;
            }
        }

        return (current_count + 1) > bucket_capacity;
    }

    /**
     * @brief Función hash alternativa para testing
     */
    static size_t simpleHash(const std::string& key) {
        size_t hash_val = 5381;
        for (char c : key) {
            hash_val = ((hash_val << 5) + hash_val) + c;
        }
        return hash_val;
    }

    /**
     * @brief Hash con semilla personalizada
     */
    static size_t hashWithSeed(const std::string& key, size_t seed) {
        size_t hash_val = seed;
        for (char c : key) {
            hash_val ^= std::hash<char>{}(c) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        }
        return hash_val;
    }

    /**
     * @brief Estadísticas de colisión para diferentes profundidades
     */
    static void analyzeCollisions(const std::vector<std::string>& keys) {
        std::cout << "\n⚠️ ANÁLISIS DE COLISIONES:" << std::endl;
        std::cout << "Depth | Buckets | Collisions | Max per bucket" << std::endl;
        std::cout << "------|---------|------------|---------------" << std::endl;

        for (int depth = 1; depth <= 6; depth++) {
            size_t total_buckets = 1ULL << depth;
            std::vector<int> bucket_counts(total_buckets, 0);
            
            // Contar elementos por bucket
            for (const auto& key : keys) {
                size_t hash_val = hash(key);
                size_t bucket_index = getBits(hash_val, depth);
                bucket_counts[bucket_index]++;
            }
            
            // Calcular estadísticas
            int collisions = 0;
            int max_per_bucket = 0;
            
            for (int count : bucket_counts) {
                if (count > 1) {
                    collisions += count - 1;
                }
                max_per_bucket = std::max(max_per_bucket, count);
            }
            
            std::cout << std::setw(5) << depth << " | ";
            std::cout << std::setw(7) << total_buckets << " | ";
            std::cout << std::setw(10) << collisions << " | ";
            std::cout << std::setw(14) << max_per_bucket << std::endl;
        }
    }
};

#endif // HASH_FUNCTION_H