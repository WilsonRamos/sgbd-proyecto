#ifndef LRU_REPLACER_H
#define LRU_REPLACER_H

#include <list>
#include <unordered_map>
#include <iostream>

/**
 * @brief LRU Replacement Policy
 * 
 * Implementa la política LRU (Least Recently Used) para el Buffer Pool:
 * - Mantiene orden de acceso de frames
 * - Proporciona el frame menos recientemente usado para evicción
 * - Integra con Page Table para verificar pin status
 */
class LRUReplacer {
private:
    size_t capacity;                                              // Capacidad máxima
    std::list<int> lru_list;                                     // Lista LRU (más reciente al frente)
    std::unordered_map<int, std::list<int>::iterator> frame_map; // Frame → Iterator en lista

public:
    /**
     * @brief Constructor
     */
    explicit LRUReplacer(size_t max_capacity) : capacity(max_capacity) {}

    /**
     * @brief Añade un frame al replacer o actualiza su posición
     */
    void recordAccess(int frame_id) {
        auto it = frame_map.find(frame_id);
        
        if (it != frame_map.end()) {
            // Frame ya existe, moverlo al frente
            lru_list.erase(it->second);
        } else if (lru_list.size() >= capacity) {
            // Lista llena, no agregar más hasta que se libere espacio
            return;
        }
        
        // Añadir al frente (más reciente)
        lru_list.push_front(frame_id);
        frame_map[frame_id] = lru_list.begin();
        
        std::cout << "🔄 LRU: Frame " << frame_id << " accedido" << std::endl;
    }

    /**
     * @brief Elimina un frame del replacer (cuando se hace pin)
     */
    void pin(int frame_id) {
        auto it = frame_map.find(frame_id);
        if (it != frame_map.end()) {
            lru_list.erase(it->second);
            frame_map.erase(it);
            std::cout << "📌 LRU: Frame " << frame_id << " removed (pinned)" << std::endl;
        }
    }

    /**
     * @brief Añade un frame de vuelta al replacer (cuando se hace unpin)
     */
    void unpin(int frame_id) {
        if (frame_map.find(frame_id) == frame_map.end() && lru_list.size() < capacity) {
            lru_list.push_front(frame_id);
            frame_map[frame_id] = lru_list.begin();
            std::cout << "📍 LRU: Frame " << frame_id << " added back (unpinned)" << std::endl;
        }
    }

    /**
     * @brief Obtiene el frame menos recientemente usado para evicción
     */
    bool victim(int& frame_id) {
        if (lru_list.empty()) {
            return false;
        }
        
        // El último elemento es el menos recientemente usado
        frame_id = lru_list.back();
        lru_list.pop_back();
        frame_map.erase(frame_id);
        
        std::cout << "🎯 LRU: Frame " << frame_id << " seleccionado para evicción" << std::endl;
        return true;
    }

    /**
     * @brief Elimina un frame específico del replacer
     */
    void remove(int frame_id) {
        auto it = frame_map.find(frame_id);
        if (it != frame_map.end()) {
            lru_list.erase(it->second);
            frame_map.erase(it);
            std::cout << "🗑️  LRU: Frame " << frame_id << " removed" << std::endl;
        }
    }

    /**
     * @brief Obtiene el tamaño actual
     */
    size_t size() const {
        return lru_list.size();
    }

    /**
     * @brief Verifica si está vacío
     */
    bool empty() const {
        return lru_list.empty();
    }

    /**
     * @brief Obtiene la capacidad máxima
     */
    size_t getCapacity() const {
        return capacity;
    }

    /**
     * @brief Redimensiona la capacidad
     */
    void resize(size_t new_capacity) {
        capacity = new_capacity;
        
        // Si la nueva capacidad es menor, eliminar frames del final
        while (lru_list.size() > capacity) {
            int frame_to_remove = lru_list.back();
            lru_list.pop_back();
            frame_map.erase(frame_to_remove);
        }
    }

    /**
     * @brief Verifica si un frame está en el replacer
     */
    bool contains(int frame_id) const {
        return frame_map.find(frame_id) != frame_map.end();
    }

    /**
     * @brief Obtiene información sobre el orden LRU
     */
    std::vector<int> getLRUOrder() const {
        return std::vector<int>(lru_list.begin(), lru_list.end());
    }

    /**
     * @brief Obtiene el frame más recientemente usado
     */
    int getMostRecentlyUsed() const {
        return lru_list.empty() ? -1 : lru_list.front();
    }

    /**
     * @brief Obtiene el frame menos recientemente usado (sin removerlo)
     */
    int getLeastRecentlyUsed() const {
        return lru_list.empty() ? -1 : lru_list.back();
    }

    /**
     * @brief Limpia todos los frames
     */
    void clear() {
        lru_list.clear();
        frame_map.clear();
        std::cout << "🧹 LRU: Cleared all frames" << std::endl;
    }

    /**
     * @brief Muestra información del estado actual
     */
    void displayInfo() const {
        std::cout << "\n=== LRU REPLACER ===" << std::endl;
        std::cout << "Capacidad: " << capacity << std::endl;
        std::cout << "Frames actuales: " << lru_list.size() << std::endl;
        
        if (!lru_list.empty()) {
            std::cout << "Orden LRU (más reciente → menos reciente): ";
            for (auto it = lru_list.begin(); it != lru_list.end(); ++it) {
                if (it != lru_list.begin()) std::cout << " → ";
                std::cout << "F" << *it;
            }
            std::cout << std::endl;
            
            std::cout << "Próximo candidato para evicción: F" << getLeastRecentlyUsed() << std::endl;
        } else {
            std::cout << "No hay frames en el replacer" << std::endl;
        }
    }

    /**
     * @brief Muestra versión compacta para demos
     */
    void displayCompact() const {
        std::cout << "LRU: [";
        bool first = true;
        for (int frame : lru_list) {
            if (!first) std::cout << "→";
            std::cout << "F" << frame;
            first = false;
        }
        std::cout << "] (" << lru_list.size() << "/" << capacity << ")" << std::endl;
    }

    /**
     * @brief Obtiene estadísticas
     */
    struct Stats {
        size_t current_size;
        size_t capacity;
        int mru_frame;      // Most Recently Used
        int lru_frame;      // Least Recently Used
        double utilization; // Porcentaje de utilización
    };
    
    Stats getStats() const {
        Stats stats;
        stats.current_size = lru_list.size();
        stats.capacity = capacity;
        stats.mru_frame = getMostRecentlyUsed();
        stats.lru_frame = getLeastRecentlyUsed();
        stats.utilization = capacity > 0 ? 
            (static_cast<double>(lru_list.size()) / capacity * 100.0) : 0.0;
        return stats;
    }
};

#endif // LRU_REPLACER_H