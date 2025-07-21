#ifndef KEY_COMPARATOR_H
#define KEY_COMPARATOR_H

#include <string>
#include <chrono>

template<typename T>
class KeyComparator {
public:
    static int compare(const T& a, const T& b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
};

// Especialización para strings
template<>
class KeyComparator<std::string> {
public:
    static int compare(const std::string& a, const std::string& b) {
        return a.compare(b);
    }
};

// Especialización para timestamps (representados como strings ISO)
class TimestampComparator {
public:
    static int compare(const std::string& a, const std::string& b) {
        // Asumir formato ISO 8601: "2025-06-25 00:47:02"
        return a.compare(b);
    }
};

#endif
