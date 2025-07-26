#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <string>
#include <functional>
#include <cstdint>

/**
 * @brief Funciones hash especializadas para Hash Extensible
 * 
 * Proporciona diferentes algoritmos de hash optimizados para:
 * - IMEI (strings numéricos largos)
 * - Strings generales
 * - Distribución uniforme en buckets
 */
class HashFunction {
public:
    /**
     * @brief Hash estándar usando std::hash
     */
    static size_t standardHash(const std::string& key) {
        return std::hash<std::string>{}(key);
    }
    
    /**
     * @brief Hash optimizado para IMEI (15 dígitos)
     * Usa características específicas de IMEI para mejor distribución
     */
    static size_t imeiHash(const std::string& imei) {
        if (imei.length() < 10) {
            return standardHash(imei);
        }
        
        // Combinar diferentes partes del IMEI
        uint64_t hash = 0;
        
        // Parte 1: TAC (Type Allocation Code) - primeros 8 dígitos
        for (int i = 0; i < 8 && i < imei.length(); i++) {
            hash = hash * 31 + (imei[i] - '0');
        }
        
        // Parte 2: SNR (Serial Number) - siguientes 6 dígitos
        for (int i = 8; i < 14 && i < imei.length(); i++) {
            hash = hash * 37 + (imei[i] - '0');
        }
        
        // Parte 3: Check digit (último dígito)
        if (imei.length() >= 15) {
            hash = hash * 41 + (imei[14] - '0');
        }
        
        return static_cast<size_t>(hash);
    }
    
    /**
     * @brief Hash Fowler-Noll-Vo (FNV-1a) - distribución uniforme
     */
    static size_t fnvHash(const std::string& key) {
        const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        const uint64_t FNV_PRIME = 1099511628211ULL;
        
        uint64_t hash = FNV_OFFSET_BASIS;
        
        for (char c : key) {
            hash ^= static_cast<uint64_t>(c);
            hash *= FNV_PRIME;
        }
        
        return static_cast<size_t>(hash);
    }
    
    /**
     * @brief Hash MurmurHash3 simplificado - alta calidad
     */
    static size_t murmurHash(const std::string& key) {
        const uint32_t seed = 0x9747b28c;
        const uint32_t m = 0x5bd1e995;
        const int r = 24;
        
        uint32_t len = static_cast<uint32_t>(key.length());
        uint32_t h = seed ^ len;
        
        const unsigned char* data = reinterpret_cast<const unsigned char*>(key.c_str());
        
        while (len >= 4) {
            uint32_t k = *(uint32_t*)data;
            
            k *= m;
            k ^= k >> r;
            k *= m;
            
            h *= m;
            h ^= k;
            
            data += 4;
            len -= 4;
        }
        
        // Handle remaining bytes
        switch (len) {
            case 3: h ^= data[2] << 16;
            case 2: h ^= data[1] << 8;
            case 1: h ^= data[0];
                    h *= m;
        }
        
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;
        
        return static_cast<size_t>(h);
    }
    
    /**
     * @brief Hash con máscara para profundidad específica
     */
    static size_t hashWithDepth(const std::string& key, int depth) {
        size_t hash = fnvHash(key);
        size_t mask = (1 << depth) - 1;  // Máscara de 'depth' bits
        return hash & mask;
    }
    
    /**
     * @brief Hash para distribución en directorio de Hash Extensible
     */
    static size_t directoryHash(const std::string& key, int global_depth) {
        return hashWithDepth(key, global_depth);
    }
    
    /**
     * @brief Hash para comparación y debugging
     */
    static std::string hashInfo(const std::string& key) {
        std::string info = "Hash info for '" + key.substr(0, 20) + "':\n";
        info += "  Standard: " + std::to_string(standardHash(key)) + "\n";
        info += "  FNV-1a:   " + std::to_string(fnvHash(key)) + "\n";
        info += "  Murmur:   " + std::to_string(murmurHash(key)) + "\n";
        
        if (key.length() >= 10 && std::all_of(key.begin(), key.end(), ::isdigit)) {
            info += "  IMEI:     " + std::to_string(imeiHash(key)) + "\n";
        }
        
        return info;
    }
    
    /**
     * @brief Verifica calidad de distribución para un conjunto de claves
     */
    static double calculateDistributionQuality(const std::vector<std::string>& keys, int num_buckets) {
        std::vector<int> bucket_counts(num_buckets, 0);
        
        for (const auto& key : keys) {
            size_t hash = fnvHash(key);
            int bucket = hash % num_buckets;
            bucket_counts[bucket]++;
        }
        
        // Calcular desviación estándar de la distribución
        double mean = static_cast<double>(keys.size()) / num_buckets;
        double variance = 0.0;
        
        for (int count : bucket_counts) {
            double diff = count - mean;
            variance += diff * diff;
        }
        
        variance /= num_buckets;
        double stddev = std::sqrt(variance);
        
        // Retornar coeficiente de variación (lower is better)
        return stddev / mean;
    }
};

/**
 * @brief Función hash por defecto para Hash Extensible
 */
inline size_t defaultExtensibleHash(const std::string& key) {
    return HashFunction::fnvHash(key);
}

/**
 * @brief Función hash especializada para IMEI
 */
inline size_t imeiExtensibleHash(const std::string& imei) {
    return HashFunction::imeiHash(imei);
}

#endif // HASH_FUNCTION_H