#ifndef KEY_COMPARATOR_H
#define KEY_COMPARATOR_H

#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <optional>  // ← AGREGADO: Header para std::optional
#include <vector>    // ← AGREGADO: Para el método analyzeKeys

/**
 * @brief KeyComparator - Comparador especializado para B+ Tree
 * 
 * Implementa comparaciones específicas para diferentes tipos de datos:
 * - Strings generales (lexicográfico)
 * - Timestamps (cronológico)
 * - IMEI (numérico como string)
 * - Números como strings
 * 
 * Utilizado por B+ Tree para ordenamiento y búsquedas por rango
 */
template<typename KeyType>
class KeyComparator {
public:
    /**
     * @brief Comparación principal
     * Retorna: -1 si a < b, 0 si a == b, 1 si a > b
     */
    static int compare(const KeyType& a, const KeyType& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    /**
     * @brief Operadores de comparación
     */
    static bool less(const KeyType& a, const KeyType& b) {
        return compare(a, b) < 0;
    }

    static bool equal(const KeyType& a, const KeyType& b) {
        return compare(a, b) == 0;
    }

    static bool greater(const KeyType& a, const KeyType& b) {
        return compare(a, b) > 0;
    }

    static bool lessEqual(const KeyType& a, const KeyType& b) {
        return compare(a, b) <= 0;
    }

    static bool greaterEqual(const KeyType& a, const KeyType& b) {
        return compare(a, b) >= 0;
    }
};

/**
 * @brief Especialización para std::string
 */
template<>
class KeyComparator<std::string> {
private:
    // ← MOVIDO AQUÍ: Declaraciones privadas ANTES de ser usadas
    
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

    /**
     * @brief Parsea timestamp a time_point
     */
    static std::optional<std::chrono::system_clock::time_point> parseTimestamp(const std::string& timestamp) {
        try {
            // Formato esperado: "YYYY-MM-DD HH:MM:SS"
            if (timestamp.length() < 19) return std::nullopt;

            std::istringstream ss(timestamp);
            std::tm tm = {};
            
            // Intentar parsear
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            
            if (ss.fail()) {
                // Intentar formato alternativo
                ss.clear();
                ss.str(timestamp);
                ss >> std::get_time(&tm, "%d/%m/%Y %H:%M:%S");
                
                if (ss.fail()) return std::nullopt;
            }

            auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            return time_point;

        } catch (...) {
            return std::nullopt;
        }
    }

    /**
     * @brief Parsea fecha a time_point
     */
    static std::optional<std::chrono::system_clock::time_point> parseDate(const std::string& date) {
        try {
            std::istringstream ss(date);
            std::tm tm = {};
            
            // Formato YYYY-MM-DD
            if (date.find('-') != std::string::npos) {
                ss >> std::get_time(&tm, "%Y-%m-%d");
            }
            // Formato DD/MM/YYYY
            else if (date.find('/') != std::string::npos) {
                ss >> std::get_time(&tm, "%d/%m/%Y");
            }
            else {
                return std::nullopt;
            }
            
            if (ss.fail()) return std::nullopt;

            auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            return time_point;

        } catch (...) {
            return std::nullopt;
        }
    }

public:
    // ← AHORA LAS FUNCIONES PÚBLICAS PUEDEN USAR LAS PRIVADAS
    
    /**
     * @brief Comparación lexicográfica estándar
     */
    static int compare(const std::string& a, const std::string& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    /**
     * @brief Comparación de timestamps
     * Formato esperado: "YYYY-MM-DD HH:MM:SS"
     */
    static int compareTimestamp(const std::string& a, const std::string& b) {
        // Intentar parsear como timestamp
        auto time_a = parseTimestamp(a);
        auto time_b = parseTimestamp(b);

        if (time_a.has_value() && time_b.has_value()) {
            if (time_a.value() < time_b.value()) return -1;
            if (time_a.value() > time_b.value()) return 1;
            return 0;
        }

        // Fallback a comparación lexicográfica
        return compare(a, b);
    }

    /**
     * @brief Comparación numérica (para IMEI, IDs, etc.)
     */
    static int compareNumeric(const std::string& a, const std::string& b) {
        if (isNumeric(a) && isNumeric(b)) {
            // Si ambos son numéricos, comparar por longitud primero
            if (a.length() != b.length()) {
                return (a.length() < b.length()) ? -1 : 1;
            }
            // Si tienen la misma longitud, comparación lexicográfica es equivalente a numérica
            return compare(a, b);
        }

        // Fallback a comparación lexicográfica
        return compare(a, b);
    }

    /**
     * @brief Comparación de fechas (formato DD/MM/YYYY o YYYY-MM-DD)
     */
    static int compareDate(const std::string& a, const std::string& b) {
        auto date_a = parseDate(a);
        auto date_b = parseDate(b);

        if (date_a.has_value() && date_b.has_value()) {
            if (date_a.value() < date_b.value()) return -1;
            if (date_a.value() > date_b.value()) return 1;
            return 0;
        }

        return compare(a, b);
    }

    /**
     * @brief Operadores de comparación
     */
    static bool less(const std::string& a, const std::string& b) {
        return compare(a, b) < 0;
    }

    static bool equal(const std::string& a, const std::string& b) {
        return compare(a, b) == 0;
    }

    static bool greater(const std::string& a, const std::string& b) {
        return compare(a, b) > 0;
    }

    static bool lessEqual(const std::string& a, const std::string& b) {
        return compare(a, b) <= 0;
    }

    static bool greaterEqual(const std::string& a, const std::string& b) {
        return compare(a, b) >= 0;
    }

    /**
     * @brief Operadores especializados para timestamps
     */
    static bool timestampLess(const std::string& a, const std::string& b) {
        return compareTimestamp(a, b) < 0;
    }

    static bool timestampGreater(const std::string& a, const std::string& b) {
        return compareTimestamp(a, b) > 0;
    }

    static bool timestampEqual(const std::string& a, const std::string& b) {
        return compareTimestamp(a, b) == 0;
    }

    /**
     * @brief Verifica si una clave está en un rango
     */
    static bool inRange(const std::string& key, const std::string& start, const std::string& end, 
                       bool include_start = true, bool include_end = true) {
        int cmp_start = compare(key, start);
        int cmp_end = compare(key, end);

        bool start_ok = include_start ? (cmp_start >= 0) : (cmp_start > 0);
        bool end_ok = include_end ? (cmp_end <= 0) : (cmp_end < 0);

        return start_ok && end_ok;
    }

    /**
     * @brief Verifica si una clave está en un rango de timestamps
     */
    static bool inTimestampRange(const std::string& key, const std::string& start, const std::string& end,
                                bool include_start = true, bool include_end = true) {
        int cmp_start = compareTimestamp(key, start);
        int cmp_end = compareTimestamp(key, end);

        bool start_ok = include_start ? (cmp_start >= 0) : (cmp_start > 0);
        bool end_ok = include_end ? (cmp_end <= 0) : (cmp_end < 0);

        return start_ok && end_ok;
    }

    /**
     * @brief Información de comparación (educativo)
     */
    static void showComparisonInfo(const std::string& a, const std::string& b) {
        std::cout << "\n🔍 INFORMACIÓN DE COMPARACIÓN:" << std::endl;
        std::cout << "A: '" << a << "'" << std::endl;
        std::cout << "B: '" << b << "'" << std::endl;
        
        int lex_cmp = compare(a, b);
        std::cout << "Comparación lexicográfica: " << lex_cmp;
        if (lex_cmp < 0) std::cout << " (A < B)";
        else if (lex_cmp > 0) std::cout << " (A > B)";
        else std::cout << " (A == B)";
        std::cout << std::endl;

        // Si parecen timestamps
        if (a.length() >= 19 && b.length() >= 19) {
            int ts_cmp = compareTimestamp(a, b);
            std::cout << "Comparación temporal: " << ts_cmp;
            if (ts_cmp < 0) std::cout << " (A antes que B)";
            else if (ts_cmp > 0) std::cout << " (A después que B)";
            else std::cout << " (A == B)";
            std::cout << std::endl;
        }

        // Si parecen numéricos
        if (isNumeric(a) && isNumeric(b)) {
            int num_cmp = compareNumeric(a, b);
            std::cout << "Comparación numérica: " << num_cmp;
            if (num_cmp < 0) std::cout << " (A < B)";
            else if (num_cmp > 0) std::cout << " (A > B)";
            else std::cout << " (A == B)";
            std::cout << std::endl;
        }
    }

    /**
     * @brief Análisis de un conjunto de claves
     */
    static void analyzeKeys(const std::vector<std::string>& keys) {
        if (keys.empty()) return;

        std::cout << "\n📊 ANÁLISIS DE CLAVES:" << std::endl;
        std::cout << "Total de claves: " << keys.size() << std::endl;

        // Clasificar tipos
        int numeric_count = 0;
        int timestamp_count = 0;
        int date_count = 0;

        for (const auto& key : keys) {
            if (isNumeric(key)) numeric_count++;
            if (parseTimestamp(key).has_value()) timestamp_count++;
            if (parseDate(key).has_value()) date_count++;
        }

        std::cout << "Claves numéricas: " << numeric_count << std::endl;
        std::cout << "Claves timestamp: " << timestamp_count << std::endl;
        std::cout << "Claves fecha: " << date_count << std::endl;

        // Mostrar rango
        if (keys.size() > 1) {
            auto sorted_keys = keys;
            std::sort(sorted_keys.begin(), sorted_keys.end());
            
            std::cout << "Rango lexicográfico:" << std::endl;
            std::cout << "  Min: '" << sorted_keys.front() << "'" << std::endl;
            std::cout << "  Max: '" << sorted_keys.back() << "'" << std::endl;

            // Si son timestamps, mostrar rango temporal
            if (timestamp_count > 0) {
                std::sort(sorted_keys.begin(), sorted_keys.end(), 
                    [](const std::string& a, const std::string& b) {
                        return compareTimestamp(a, b) < 0;
                    });
                
                std::cout << "Rango temporal:" << std::endl;
                std::cout << "  Más temprano: '" << sorted_keys.front() << "'" << std::endl;
                std::cout << "  Más tardío: '" << sorted_keys.back() << "'" << std::endl;
            }
        }
    }
};

#endif // KEY_COMPARATOR_H