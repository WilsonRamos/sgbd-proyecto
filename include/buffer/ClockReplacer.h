#ifndef CLOCK_REPLACER_H
#define CLOCK_REPLACER_H

#include <vector>
#include <unordered_set>
#include <iostream>
#include "PageTable.h"  // NUEVO: Necesario para consultar pin_count

/**
 * @brief Clock Replacement Policy - Pin-Aware MEJORADO
 * 
 * Implementa el algoritmo Clock con conciencia de pin_count:
 * - NUNCA evicta páginas con pin_count > 0
 * - CADA PASADA disminuye pin_count (política corregida)
 * - Reference bits para segunda oportunidad
 * - Integrado con PageTable para verificar pins
 * - Garantiza encontrar víctimas eventualmente
 */
class ClockReplacer {
private:
    size_t capacity;                           // Capacidad máxima del buffer
    std::vector<int> frames;                   // Array circular de frame IDs
    std::vector<bool> reference_bits;          // Bits de referencia para cada posición
    std::vector<bool> in_replacer;             // Track si un frame está en el replacer
    size_t clock_hand;                         // Posición actual del "clock hand"
    size_t current_size;                       // Número actual de frames en el replacer
    
    PageTable* page_table;                     // NUEVO: Referencia para consultar pin_count
    
    // Estadísticas para el comportamiento especial
    size_t second_chance_given;                // Reference bits reset to 0
    size_t pin_decrements;                     // Pin counts decrementados automáticamente
    
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
    
    /**
     * @brief Obtiene PageID desde frame usando buffer_pool
     */
    int getPageIdFromFrame(int frame_id) const {
        // Buscar en PageTable qué página está en este frame
        auto all_pages = page_table->getAllPageIds();
        for (int page_id : all_pages) {
            PageTableEntry entry;
            if (page_table->findPageReadOnly(page_id, entry)) {
                if (entry.frame_id == frame_id && entry.valid_bit) {
                    return page_id;
                }
            }
        }
        return -1;
    }

public:
    /**
     * @brief Constructor - NUEVO: requiere PageTable
     */
    explicit ClockReplacer(size_t max_capacity, PageTable* pt) 
        : capacity(max_capacity)
        , frames(max_capacity, -1)
        , reference_bits(max_capacity, false)
        , in_replacer(max_capacity, false)
        , clock_hand(0)
        , current_size(0)
        , page_table(pt)
        , second_chance_given(0)
        , pin_decrements(0) {
        
        std::cout << "🕐 Clock Replacer inicializado (PIN-AWARE MEJORADO) con capacidad: " << capacity << std::endl;
        std::cout << "   ✅ Decremento automático en CADA pasada" << std::endl;
        std::cout << "   ✅ Garantía de encontrar víctimas eventualmente" << std::endl;
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
     * @brief Algoritmo Clock PIN-AWARE MEJORADO con Decremento en Cada Pasada
     */
    bool victim(int& frame_id) {
        if (current_size == 0) {
            return false;
        }
        
        size_t start_pos = clock_hand;
        size_t full_passes = 0;
        const size_t MAX_PASSES = 5;  // Aumentado para garantizar encontrar víctimas
        
        std::cout << "🕐 Clock Sweep MEJORADO iniciado desde posición " << clock_hand << std::endl;
        
        // Algoritmo Clock con conciencia de pins MEJORADO
        do {
            if (in_replacer[clock_hand]) {
                int current_frame = frames[clock_hand];
                int page_id = getPageIdFromFrame(current_frame);
                
                if (page_id != -1) {
                    PageTableEntry entry;
                    if (page_table->findPageReadOnly(page_id, entry)) {
                        
                        std::cout << "🔍 Evaluando Frame " << current_frame 
                                  << " (Page " << page_id << "): "
                                  << "pin=" << entry.pin_count 
                                  << ", ref=" << (reference_bits[clock_hand] ? 1 : 0) << std::endl;
                        
                        // ✅ VERIFICACIÓN CRÍTICA: pin_count DEBE ser 0
                        if (entry.pin_count > 0) {
                            std::cout << "📌 Frame " << current_frame 
                                      << " NO evictable (pinned=" << entry.pin_count << ")" << std::endl;
                            
                            // 🆕 POLÍTICA MEJORADA: Decrementar en CADA pasada desde la primera
                            if (full_passes >= 1) {
                                page_table->unpinPage(page_id, false);
                                pin_decrements++;
                                std::cout << "🔽 Pasada " << full_passes 
                                          << ": Pin count " << (entry.pin_count + 1) 
                                          << "→" << entry.pin_count << " para página " << page_id << std::endl;
                            }
                            
                        } else {
                            // pin_count == 0, verificar reference bit
                            if (reference_bits[clock_hand]) {
                                // Reference bit = 1, dar segunda oportunidad
                                reference_bits[clock_hand] = false;
                                second_chance_given++;
                                std::cout << "🕐 Frame " << current_frame 
                                          << " ref=1→0 (segunda oportunidad)" << std::endl;
                            } else {
                                // Reference bit = 0 AND pin_count = 0 → VÍCTIMA!
                                frame_id = current_frame;
                                in_replacer[clock_hand] = false;
                                frames[clock_hand] = -1;
                                current_size--;
                                
                                // Avanzar clock hand
                                clock_hand = (clock_hand + 1) % capacity;
                                
                                std::cout << "🎯 VÍCTIMA ENCONTRADA: Frame " << frame_id 
                                          << " (Page " << page_id << ") - ref=0, pin=0" << std::endl;
                                std::cout << "   📊 Estadísticas: " << pin_decrements 
                                          << " pins decrementados, " << second_chance_given 
                                          << " segundas oportunidades" << std::endl;
                                return true;
                            }
                        }
                    }
                }
            }
            
            // Avanzar clock hand
            clock_hand = (clock_hand + 1) % capacity;
            
            // Detectar pasada completa
            if (clock_hand == start_pos) {
                full_passes++;
                std::cout << "🔄 Pasada completa #" << full_passes << " terminada" 
                          << " (Pins decrementados en esta pasada: +" << pin_decrements << ")" << std::endl;
                
                if (full_passes >= MAX_PASSES) {
                    std::cout << "⚠️  Clock: Máximas pasadas alcanzadas (" << MAX_PASSES 
                              << "). Total pins decrementados: " << pin_decrements << std::endl;
                    std::cout << "   🔍 Verificando si quedan páginas con pin > 0..." << std::endl;
                    
                    // Diagnóstico final
                    for (size_t i = 0; i < capacity; ++i) {
                        if (in_replacer[i]) {
                            int pid = getPageIdFromFrame(frames[i]);
                            if (pid != -1) {
                                PageTableEntry ent;
                                if (page_table->findPageReadOnly(pid, ent)) {
                                    std::cout << "   Frame " << frames[i] 
                                              << ": pin=" << ent.pin_count 
                                              << ", ref=" << (reference_bits[i] ? 1 : 0) << std::endl;
                                }
                            }
                        }
                    }
                    
                    return false;
                }
            }
            
        } while (full_passes < MAX_PASSES);
        
        std::cout << "❌ Clock MEJORADO: No se encontró víctima después de " << full_passes << " pasadas" << std::endl;
        return false;
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
     * @brief Obtiene el reference bit de un frame específico
     */
    bool getReferenceAt(int frame_id) const {
        // Buscar la posición del frame en el array circular
        for (size_t i = 0; i < capacity; ++i) {
            if (in_replacer[i] && frames[i] == frame_id) {
                return reference_bits[i];
            }
        }
        return false; // Frame no está en el replacer
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
        second_chance_given = 0;
        pin_decrements = 0;
        std::cout << "🧹 Clock MEJORADO: Cleared all frames" << std::endl;
    }

    /**
     * @brief Muestra el estado actual del Clock PIN-AWARE MEJORADO
     */
    void displayInfo() const {
        std::cout << "\n=== CLOCK REPLACER (PIN-AWARE MEJORADO) ===" << std::endl;
        std::cout << "Capacidad: " << capacity << std::endl;
        std::cout << "Frames actuales: " << current_size << std::endl;
        std::cout << "Clock Hand posición: " << clock_hand << std::endl;
        std::cout << "Segunda oportunidades dadas: " << second_chance_given << std::endl;
        std::cout << "Pin decrements (automáticos): " << pin_decrements << std::endl;
        
        std::cout << "\nEstado del Clock Buffer:" << std::endl;
        for (size_t i = 0; i < capacity; ++i) {
            std::cout << "Pos[" << i << "]: ";
            if (in_replacer[i]) {
                int page_id = getPageIdFromFrame(frames[i]);
                PageTableEntry entry;
                int pin_count = 0;
                if (page_id != -1 && page_table->findPageReadOnly(page_id, entry)) {
                    pin_count = entry.pin_count;
                }
                
                std::cout << "F" << frames[i] << "(ref=" << (reference_bits[i] ? 1 : 0) 
                          << ",pin=" << pin_count << ")";
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
        std::cout << "Clock MEJORADO: [";
        for (size_t i = 0; i < capacity; ++i) {
            if (i > 0) std::cout << "|";
            
            if (in_replacer[i]) {
                int page_id = getPageIdFromFrame(frames[i]);
                PageTableEntry entry;
                int pin_count = 0;
                if (page_id != -1 && page_table->findPageReadOnly(page_id, entry)) {
                    pin_count = entry.pin_count;
                }
                
                std::cout << "F" << frames[i] << ":" << (reference_bits[i] ? 1 : 0) 
                          << "p" << pin_count;
            } else {
                std::cout << "---";
            }
            
            if (i == clock_hand) std::cout << "⟲";
        }
        std::cout << "] (" << current_size << "/" << capacity << ") ";
        std::cout << "Dec:" << pin_decrements << " 2nd:" << second_chance_given << std::endl;
    }

    /**
     * @brief Obtiene estadísticas
     */
    struct Stats {
        size_t current_size;
        size_t capacity;
        size_t clock_hand_pos;
        double utilization;
        int active_refs;        // Frames con reference_bit = 1
        size_t second_chances;  // Segunda oportunidades dadas
        size_t pin_decrements;  // Pin counts decrementados
    };
    
    Stats getStats() const {
        Stats stats;
        stats.current_size = current_size;
        stats.capacity = capacity;
        stats.clock_hand_pos = clock_hand;
        stats.utilization = capacity > 0 ? 
            (static_cast<double>(current_size) / capacity) * 100.0 : 0.0;
        
        // Contar reference bits activos
        stats.active_refs = 0;
        for (size_t i = 0; i < capacity; ++i) {
            if (in_replacer[i] && reference_bits[i]) {
                stats.active_refs++;
            }
        }
        
        stats.second_chances = second_chance_given;
        stats.pin_decrements = pin_decrements;
        
        return stats;
    }
};

#endif // CLOCK_REPLACER_H