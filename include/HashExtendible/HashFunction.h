#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <string>
#include <cstdint>

class HashFunction {
public:
    static uint32_t hashString(const std::string& key) {
        uint32_t hash = 0;
        for (char c : key) {
            hash = hash * 31 + static_cast<uint32_t>(c);
        }
        return hash;
    }
    
    static uint32_t hashInt(int key) {
        return static_cast<uint32_t>(key * 2654435761U);
    }
    
    static uint32_t hashIMEI(const std::string& imei) {
        // Hash optimizado para IMEIs - usar últimos 8 dígitos
        if (imei.length() < 8) return hashString(imei);
        
        std::string suffix = imei.substr(imei.length() - 8);
        return hashString(suffix);
    }
    
    static uint32_t getBits(uint32_t hash, int num_bits) {
        if (num_bits <= 0) return 0;
        return hash & ((1U << num_bits) - 1);
    }
};

#endif