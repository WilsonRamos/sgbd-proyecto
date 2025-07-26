#ifndef KEY_COMPARATOR_H
#define KEY_COMPARATOR_H

#include <string>
#include <cstring>
#include <iostream>

/**
 * @brief Comparador genérico de claves para B+ Tree
 * 
 * Proporciona comparaciones consistentes para diferentes tipos de claves:
 * - Enteros
 * - Strings (incluyendo timestamps)
 * - Números en punto flotante
 * - Tipos personalizados
 */
template<typename KeyType>
class KeyComparator {
public:
    /**
     * @brief Compara dos claves
     * @param a Primera clave
     * @param b Segunda clave
     * @return < 0 si a < b, 0 si a == b, > 0 si a > b
     */
    static int compare(const KeyType& a, const KeyType& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
    
    /**
     * @brief Verifica si dos claves son iguales
     */
    static bool equal(const KeyType& a, const KeyType& b) {
        return compare(a, b) == 0;
    }
    
    /**
     * @brief Verifica si la primera clave es menor
     */
    static bool less(const KeyType& a, const KeyType& b) {
        return compare(a, b) < 0;
    }
    
    /**
     * @brief Verifica si la primera clave es menor o igual
     */
    static bool lessOrEqual(const KeyType& a, const KeyType& b) {
        return compare(a, b) <= 0;
    }
    
    /**
     * @brief Verifica si la primera clave es mayor
     */
    static bool greater(const KeyType& a, const KeyType& b) {
        return compare(a, b) > 0;
    }
    
    /**
     * @brief Verifica si la primera clave es mayor o igual
     */
    static bool greaterOrEqual(const KeyType& a, const KeyType& b) {
        return compare(a, b) >= 0;
    }
};

// ============================================================================
// ESPECIALIZACIÓN PARA STRINGS (TIMESTAMPS, IMEI, ETC.)
// ============================================================================

/**
 * @brief Especialización para std::string
 * 
 * Maneja casos especiales como:
 * - Timestamps en formato ISO
 * - IMEI con diferentes longitudes
 * - Comparación lexicográfica estándar
 */
template<>
class KeyComparator<std::string> {
public:
    static int compare(const std::string& a, const std::string& b) {
        // Para timestamps y strings en general, la comparación lexicográfica funciona bien
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
    
    /**
     * @brief Comparación específica para timestamps ISO
     * Formato esperado: "2025-07-25 15:30:00"
     */
    static int compareTimestamp(const std::string& a, const std::string& b) {
        // Los timestamps en formato ISO se pueden comparar lexicográficamente
        return compare(a, b);
    }
    
    /**
     * @brief Comparación específica para IMEI
     * Maneja diferentes longitudes de IMEI (15 dígitos estándar)
     */
    static int compareIMEI(const std::string& a, const std::string& b) {
        // Normalizar longitud si es necesario
        std::string norm_a = normalizeIMEI(a);
        std::string norm_b = normalizeIMEI(b);
        
        return compare(norm_a, norm_b);
    }
    
    /**
     * @brief Verifica si un string parece ser un timestamp
     */
    static bool isTimestamp(const std::string& str) {
        // Verificación básica para formato "YYYY-MM-DD HH:MM:SS"
        return str.length() >= 19 && 
               str[4] == '-' && str[7] == '-' && 
               str[10] == ' ' && str[13] == ':' && str[16] == ':';
    }
    
    /**
     * @brief Verifica si un string parece ser un IMEI
     */
    static bool isIMEI(const std::string& str) {
        // IMEI típicamente tiene 15 dígitos
        if (str.length() < 14 || str.length() > 16) return false;
        
        // Verificar que todos sean dígitos
        for (char c : str) {
            if (!std::isdigit(c)) return false;
        }
        
        return true;
    }
    
    static bool equal(const std::string& a, const std::string& b) {
        return compare(a, b) == 0;
    }
    
    static bool less(const std::string& a, const std::string& b) {
        return compare(a, b) < 0;
    }
    
    static bool lessOrEqual(const std::string& a, const std::string& b) {
        return compare(a, b) <= 0;
    }
    
    static bool greater(const std::string& a, const std::string& b) {
        return compare(a, b) > 0;
    }
    
    static bool greaterOrEqual(const std::string& a, const std::string& b) {
        return compare(a, b) >= 0;
    }

private:
    /**
     * @brief Normaliza un IMEI para comparación consistente
     */
    static std::string normalizeIMEI(const std::string& imei) {
        // Remover espacios y caracteres no numéricos
        std::string result;
        for (char c : imei) {
            if (std::isdigit(c)) {
                result += c;
            }
        }
        
        // Asegurar longitud estándar (padding con ceros si es necesario)
        while (result.length() < 15) {
            result = "0" + result;
        }
        
        return result;
    }
};

// ============================================================================
// ESPECIALIZACIÓN PARA ENTEROS
// ============================================================================

/**
 * @brief Especialización para int
 */
template<>
class KeyComparator<int> {
public:
    static int compare(const int& a, const int& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
    
    static bool equal(const int& a, const int& b) { return a == b; }
    static bool less(const int& a, const int& b) { return a < b; }
    static bool lessOrEqual(const int& a, const int& b) { return a <= b; }
    static bool greater(const int& a, const int& b) { return a > b; }
    static bool greaterOrEqual(const int& a, const int& b) { return a >= b; }
};

/**
 * @brief Especialización para long
 */
template<>
class KeyComparator<long> {
public:
    static int compare(const long& a, const long& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
    
    static bool equal(const long& a, const long& b) { return a == b; }
    static bool less(const long& a, const long& b) { return a < b; }
    static bool lessOrEqual(const long& a, const long& b) { return a <= b; }
    static bool greater(const long& a, const long& b) { return a > b; }
    static bool greaterOrEqual(const long& a, const long& b) { return a >= b; }
};

// ============================================================================
// ESPECIALIZACIÓN PARA NÚMEROS EN PUNTO FLOTANTE
// ============================================================================

/**
 * @brief Especialización para double con tolerancia de precisión
 */
template<>
class KeyComparator<double> {
private:
    static constexpr double EPSILON = 1e-9;

public:
    static int compare(const double& a, const double& b) {
        double diff = a - b;
        if (diff < -EPSILON) return -1;
        if (diff > EPSILON) return 1;
        return 0;
    }
    
    static bool equal(const double& a, const double& b) {
        return std::abs(a - b) < EPSILON;
    }
    
    static bool less(const double& a, const double& b) {
        return (a - b) < -EPSILON;
    }
    
    static bool lessOrEqual(const double& a, const double& b) {
        return (a - b) <= EPSILON;
    }
    
    static bool greater(const double& a, const double& b) {
        return (a - b) > EPSILON;
    }
    
    static bool greaterOrEqual(const double& a, const double& b) {
        return (a - b) >= -EPSILON;
    }
};

// ============================================================================
// UTILIDADES PARA DEBUGGING Y TESTING
// ============================================================================

/**
 * @brief Utilidades para testing y debug de comparaciones
 */
class ComparatorUtils {
public:
    /**
     * @brief Prueba las comparaciones con diferentes tipos
     */
    template<typename KeyType>
    static void testComparisons(const KeyType& a, const KeyType& b) {
        std::cout << "\n🧪 PRUEBA DE COMPARACIÓN:" << std::endl;
        std::cout << "Clave A: " << a << std::endl;
        std::cout << "Clave B: " << b << std::endl;
        
        int result = KeyComparator<KeyType>::compare(a, b);
        std::cout << "Resultado: " << result << " (";
        
        if (result < 0) {
            std::cout << "A < B";
        } else if (result > 0) {
            std::cout << "A > B";
        } else {
            std::cout << "A == B";
        }
        std::cout << ")" << std::endl;
        
        std::cout << "Verificaciones:" << std::endl;
        std::cout << "  A == B: " << KeyComparator<KeyType>::equal(a, b) << std::endl;
        std::cout << "  A < B:  " << KeyComparator<KeyType>::less(a, b) << std::endl;
        std::cout << "  A <= B: " << KeyComparator<KeyType>::lessOrEqual(a, b) << std::endl;
        std::cout << "  A > B:  " << KeyComparator<KeyType>::greater(a, b) << std::endl;
        std::cout << "  A >= B: " << KeyComparator<KeyType>::greaterOrEqual(a, b) << std::endl;
    }
    
    /**
     * @brief Prueba específica para timestamps
     */
    static void testTimestampComparisons() {
        std::cout << "\n📅 PRUEBA DE TIMESTAMPS:" << std::endl;
        
        std::vector<std::string> timestamps = {
            "2025-06-25 00:47:00",
            "2025-06-25 00:47:15",
            "2025-06-25 00:48:00",
            "2025-06-25 01:00:00",
            "2025-06-26 00:00:00"
        };
        
        for (size_t i = 0; i < timestamps.size() - 1; i++) {
            std::cout << "Comparando: " << timestamps[i] << " vs " << timestamps[i+1] << std::endl;
            int result = KeyComparator<std::string>::compare(timestamps[i], timestamps[i+1]);
            std::cout << "  Resultado: " << result << " (debería ser -1)" << std::endl;
        }
    }
    
    /**
     * @brief Prueba específica para IMEIs
     */
    static void testIMEIComparisons() {
        std::cout << "\n📱 PRUEBA DE IMEIs:" << std::endl;
        
        std::vector<std::string> imeis = {
            "868018070237400",
            "868018070237401",
            "868018070237402",
            "868018070237410",
            "868018070237420"
        };
        
        for (size_t i = 0; i < imeis.size() - 1; i++) {
            std::cout << "Comparando: " << imeis[i] << " vs " << imeis[i+1] << std::endl;
            int result = KeyComparator<std::string>::compareIMEI(imeis[i], imeis[i+1]);
            std::cout << "  Resultado: " << result << " (debería ser -1)" << std::endl;
        }
    }
    
    /**
     * @brief Valida que una secuencia de claves esté ordenada
     */
    template<typename KeyType>
    static bool validateSortedSequence(const std::vector<KeyType>& keys) {
        for (size_t i = 1; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(keys[i-1], keys[i]) > 0) {
                std::cout << "❌ Error: Secuencia no ordenada en posición " << i << std::endl;
                std::cout << "   " << keys[i-1] << " > " << keys[i] << std::endl;
                return false;
            }
        }
        
        std::cout << "✅ Secuencia correctamente ordenada (" << keys.size() << " claves)" << std::endl;
        return true;
    }
};

// ============================================================================
// MACROS ÚTILES PARA COMPARACIONES
// ============================================================================

#define KEY_EQUAL(a, b) KeyComparator<decltype(a)>::equal(a, b)
#define KEY_LESS(a, b) KeyComparator<decltype(a)>::less(a, b)
#define KEY_GREATER(a, b) KeyComparator<decltype(a)>::greater(a, b)
#define KEY_COMPARE(a, b) KeyComparator<decltype(a)>::compare(a, b)

#endif // KEY_COMPARATOR_H