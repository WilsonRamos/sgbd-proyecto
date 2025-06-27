#ifndef CLOCK_REPLACER_H
#define CLOCK_REPLACER_H

#include <vector>
#include <unordered_set>
#include <iostream>

/**
 * @brief Clock Replacement Policy
 * 
 * Implementa el algoritmo Clock como aproximación de LRU:
 * - No necesita timestamps separados por página
 * - Cada página tiene un reference bit
 * - Organiza páginas en buffer circular con "clock hand"
 * - Al hacer sweep: si bit=1 → poner a 0, si bit=0 → evict
 */
class ClockReplacer {
private:
    size_t capacity;                           // Capacidad máxima del buffer
    std::vector<int> frames;                   // Array circular de frame IDs
    std::vector<bool> reference_bits;          // Bits de referencia para cada posición
    std::vector<bool> in_replacer;             // Track si un frame está en el replacer
    size_t clock_hand;                         // Posición actual del "clock hand"
    size_t current_size;                       // Número actual de frames en el replacer
    
    /**
     * @brief Busca la posición de un frame en el array circular
     */
    int findFrame(int frame_id) const {
        for (size_t i = 0; i < capacity; ++i) {
            if (frames[i] == frame_id && in_replacer[i]) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    /**
     * @brief Busca una posición libre en el array circular
     */
    int findFreeSlot() const {
        for (size_t i = 0; i < capacity; ++i) {
            if (!in_replacer[i]) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

public:
    /**
     * @brief Constructor
     */
    explicit ClockReplacer(size_t max_capacity) 
        : capacity(max_capacity)
        , frames(max_capacity, -1)
        , reference_bits(max_capacity, false)
        , in_replacer(max_capacity, false)
        , clock_hand(0)
        , current_size(0) {
        
        std::cout << "🕐 Clock Replacer inicializado con capacidad: " << capacity << std::endl;
    }

    /**
     * @brief Registra acceso a un frame (marca reference bit = 1)
     */
    void recordAccess(int frame_id) {
        int pos = findFrame(frame_id);
        
        if (pos != -1) {
            // Frame existe, marcar reference bit
            reference_bits[pos] = true;
            std::cout << "🔄 Clock: Frame " << frame_id << " accedido (ref=1)" << std::endl;
        } else if (current_size < capacity) {
            // Frame nuevo, agregarlo al clock
            int free_slot = findFreeSlot();
            if (free_slot != -1) {
                frames[free_slot] = frame_id;
                reference_bits[free_slot] = true;
                in_replacer[free_slot] = true;
                current_size++;
                std::cout << "➕ Clock: Frame " << frame_id << " agregado (ref=1)" << std::endl;
            }
        }
    }

    /**
     * @brief Elimina un frame del replacer (cuando se hace pin)
     */
    void pin(int frame_id) {
        int pos = findFrame(frame_id);
        if (pos != -1) {
            in_replacer[pos] = false;
            reference_bits[pos] = false;
            frames[pos] = -1;
            current_size--;
            std::cout << "📌 Clock: Frame " << frame_id << " removed (pinned)" << std::endl;
        }
    }

    /**
     * @brief Añade un frame de vuelta al replacer (cuando se hace unpin)
     */
    void unpin(int frame_id) {
        if (findFrame(frame_id) == -1 && current_size < capacity) {
            int free_slot = findFreeSlot();
            if (free_slot != -1) {
                frames[free_slot] = frame_id;
                reference_bits[free_slot] = false;  // Empieza sin reference bit
                in_replacer[free_slot] = true;
                current_size++;
                std::cout << "📍 Clock: Frame " << frame_id << " added back (ref=0)" << std::endl;
            }
        }
    }

    /**
     * @brief Algoritmo Clock: busca víctima para evicción
     */
    bool victim(int& frame_id) {
        if (current_size == 0) {
            return false;
        }
        
        size_t start_pos = clock_hand;
        
        // Algoritmo Clock: sweep hasta encontrar víctima
        do {
            if (in_replacer[clock_hand]) {
                if (reference_bits[clock_hand]) {
                    // Reference bit = 1, ponerlo a 0 y continuar
                    reference_bits[clock_hand] = false;
                    std::cout << "🕐 Clock: Frame " << frames[clock_hand] 
                              << " ref=1→0 (segunda oportunidad)" << std::endl;
                } else {
                    // Reference bit = 0, víctima encontrada!
                    frame_id = frames[clock_hand];
                    in_replacer[clock_hand] = false;
                    frames[clock_hand] = -1;
                    current_size--;
                    
                    // Avanzar clock hand
                    clock_hand = (clock_hand + 1) % capacity;
                    
                    std::cout << "🎯 Clock: Frame " << frame_id 
                              << " seleccionado para evicción (ref=0)" << std::endl;
                    return true;
                }
            }
            
            // Avanzar clock hand
            clock_hand = (clock_hand + 1) % capacity;
            
        } while (clock_hand != start_pos);
        
        return false;  // No se encontró víctima (no debería pasar)
    }

    /**
     * @brief Elimina un frame específico del replacer
     */
    void remove(int frame_id) {
        int pos = findFrame(frame_id);
        if (pos != -1) {
            in_replacer[pos] = false;
            reference_bits[pos] = false;
            frames[pos] = -1;
            current_size--;
            std::cout << "🗑️  Clock: Frame " << frame_id << " removed" << std::endl;
        }
    }

    /**
     * @brief Obtiene el tamaño actual
     */
    size_t size() const {
        return current_size;
    }

    /**
     * @brief Verifica si está vacío
     */
    bool empty() const {
        return current_size == 0;
    }

    /**
     * @brief Obtiene la capacidad máxima
     */
    size_t getCapacity() const {
        return capacity;
    }

    /**
     * @brief Verifica si un frame está en el replacer
     */
    bool contains(int frame_id) const {
        return findFrame(frame_id) != -1;
    }

    /**
     * @brief Limpia todos los frames
     */
    void clear() {
        for (size_t i = 0; i < capacity; ++i) {
            frames[i] = -1;
            reference_bits[i] = false;
            in_replacer[i] = false;
        }
        clock_hand = 0;
        current_size = 0;
        std::cout << "🧹 Clock: Cleared all frames" << std::endl;
    }

    /**
     * @brief Muestra el estado actual del Clock
     */
    void displayInfo() const {
        std::cout << "\n=== CLOCK REPLACER ===" << std::endl;
        std::cout << "Capacidad: " << capacity << std::endl;
        std::cout << "Frames actuales: " << current_size << std::endl;
        std::cout << "Clock Hand posición: " << clock_hand << std::endl;
        
        std::cout << "\nEstado del Clock Buffer:" << std::endl;
        for (size_t i = 0; i < capacity; ++i) {
            std::cout << "Pos[" << i << "]: ";
            if (in_replacer[i]) {
                std::cout << "F" << frames[i] << "(ref=" << (reference_bits[i] ? 1 : 0) << ")";
                if (i == clock_hand) std::cout << " ←HAND";
            } else {
                std::cout << "EMPTY";
                if (i == clock_hand) std::cout << " ←HAND";
            }
            std::cout << std::endl;
        }
    }

    /**
     * @brief Muestra versión compacta para demos
     */
    void displayCompact() const {
        std::cout << "Clock: [";
        for (size_t i = 0; i < capacity; ++i) {
            if (i > 0) std::cout << "|";
            
            if (in_replacer[i]) {
                std::cout << "F" << frames[i] << ":" << (reference_bits[i] ? 1 : 0);
            } else {
                std::cout << "---";
            }
            
            if (i == clock_hand) std::cout << "⟲";
        }
        std::cout << "] (" << current_size << "/" << capacity << ")" << std::endl;
    }

    /**
     * @brief Obtiene estadísticas
     */
    struct Stats {
        size_t current_size;
        size_t capacity;
        size_t clock_hand_pos;
        double utilization;
        int active_refs;  // Frames con reference_bit = 1
    };
    
    Stats getStats() const {
        Stats stats;
        stats.current_size = current_size;
        stats.capacity = capacity;
        stats.clock_hand_pos = clock_hand;
        stats.utilization = capacity > 0 ? 
            (static_cast<double>(current_size) / capacity * 100.0) : 0.0;
        
        stats.active_refs = 0;
        for (size_t i = 0; i < capacity; ++i) {
            if (in_replacer[i] && reference_bits[i]) {
                stats.active_refs++;
            }
        }
        
        return stats;
    }
};

#endif // CLOCK_REPLACER_H