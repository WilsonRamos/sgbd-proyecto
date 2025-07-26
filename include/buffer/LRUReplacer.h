#ifndef LRU_REPLACER_H
#define LRU_REPLACER_H

#include <unordered_map>
#include <unordered_set>
#include <list>
#include <iostream>
#include <sstream>

/**
 * @brief LRU (Least Recently Used) Replacer
 * 
 * Implementa algoritmo LRU para selección de víctimas en Buffer Pool
 * Mantiene orden de acceso y permite eviction de frames
 */
class LRUReplacer {
private:
    size_t capacity;                                    // Capacidad máxima
    std::list<int> lru_list;                           // Lista LRU (más reciente al frente)
    std::unordered_map<int, std::list<int>::iterator> frame_map; // Frame ID -> Iterator
    std::unordered_set<int> pinned_frames;             // Frames que no se pueden evict
    
    // Estadísticas
    size_t access_count;
    size_t eviction_count;

public:
    /**
     * @brief Constructor
     */
    LRUReplacer(size_t capacity) 
        : capacity(capacity), access_count(0), eviction_count(0) {
        
        std::cout << "🔄 LRU Replacer inicializado (capacidad: " << capacity << ")" << std::endl;
    }
    
    // ============================================================================
    // OPERACIONES PRINCIPALES
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Evict frame usando política LRU
     * Retorna el frame ID menos recientemente usado, o -1 si no hay frames disponibles
     */
    int evict() {
        // Buscar desde el final de la lista (menos recientemente usado)
        auto it = lru_list.rbegin();
        
        while (it != lru_list.rend()) {
            int frame_id = *it;
            
            // Si el frame no está pinned, lo podemos evict
            if (pinned_frames.find(frame_id) == pinned_frames.end()) {
                // Convertir reverse iterator a forward iterator para erase
                auto forward_it = std::next(it).base();
                
                // Remover de la lista y del mapa
                lru_list.erase(forward_it);
                frame_map.erase(frame_id);
                
                eviction_count++;
                
                std::cout << "🔄 LRU Eviction: Frame " << frame_id << " seleccionado como víctima" << std::endl;
                return frame_id;
            }
            
            ++it;
        }
        
        std::cout << "⚠️ LRU Eviction: No hay frames disponibles para evict (todos pinned)" << std::endl;
        return -1; // No hay frames disponibles
    }
    
    /**
     * @brief Registra acceso a un frame (lo mueve al frente)
     */
    void recordAccess(int frame_id) {
        access_count++;
        
        // Si el frame ya está en la lista, moverlo al frente
        auto it = frame_map.find(frame_id);
        if (it != frame_map.end()) {
            lru_list.erase(it->second);
        }
        
        // Agregar al frente de la lista
        lru_list.push_front(frame_id);
        frame_map[frame_id] = lru_list.begin();
        
        std::cout << "📊 LRU Access: Frame " << frame_id << " movido al frente" << std::endl;
    }
    
    /**
     * @brief Marca un frame como pinned (no evictable)
     */
    void pin(int frame_id) {
        pinned_frames.insert(frame_id);
        std::cout << "📌 LRU Pin: Frame " << frame_id << " marcado como pinned" << std::endl;
    }
    
    /**
     * @brief Desmarca un frame como pinned (evictable)
     */
    void unpin(int frame_id) {
        pinned_frames.erase(frame_id);
        std::cout << "📌 LRU Unpin: Frame " << frame_id << " marcado como unpinned" << std::endl;
        
        // Si no está en la lista LRU, agregarlo
        if (frame_map.find(frame_id) == frame_map.end()) {
            recordAccess(frame_id);
        }
    }
    
    /**
     * @brief Remueve un frame del replacer
     */
    void remove(int frame_id) {
        auto it = frame_map.find(frame_id);
        if (it != frame_map.end()) {
            lru_list.erase(it->second);
            frame_map.erase(it);
        }
        
        pinned_frames.erase(frame_id);
        
        std::cout << "🗑️ LRU Remove: Frame " << frame_id << " removido del replacer" << std::endl;
    }
    
    // ============================================================================
    // INFORMACIÓN Y ESTADO
    // ============================================================================
    
    /**
     * @brief Obtiene el número de frames evictables
     */
    size_t getEvictableCount() const {
        size_t evictable = 0;
        for (int frame_id : lru_list) {
            if (pinned_frames.find(frame_id) == pinned_frames.end()) {
                evictable++;
            }
        }
        return evictable;
    }
    
    /**
     * @brief Obtiene el tamaño actual del replacer
     */
    size_t size() const {
        return lru_list.size();
    }
    
    /**
     * @brief Verifica si el replacer está vacío
     */
    bool empty() const {
        return lru_list.empty();
    }
    
    /**
     * @brief Verifica si un frame está siendo tracked
     */
    bool contains(int frame_id) const {
        return frame_map.find(frame_id) != frame_map.end();
    }
    
    /**
     * @brief Verifica si un frame está pinned
     */
    bool isPinned(int frame_id) const {
        return pinned_frames.find(frame_id) != pinned_frames.end();
    }
    
    // ============================================================================
    // ESTADÍSTICAS Y DEBUG
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas del LRU Replacer
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        ss << "=== ESTADÍSTICAS LRU REPLACER ===\n";
        ss << "Capacidad: " << capacity << "\n";
        ss << "Frames tracked: " << lru_list.size() << "\n";
        ss << "Frames pinned: " << pinned_frames.size() << "\n";
        ss << "Frames evictables: " << getEvictableCount() << "\n";
        ss << "Total accesos: " << access_count << "\n";
        ss << "Total evictions: " << eviction_count << "\n";
        
        if (eviction_count > 0) {
            double avg_accesses = (double)access_count / eviction_count;
            ss << "Promedio accesos por eviction: " << std::fixed << std::setprecision(2) << avg_accesses << "\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Muestra el estado actual del LRU
     */
    void display() const {
        std::cout << "\n🔄 ESTADO DEL LRU REPLACER" << std::endl;
        std::cout << "===========================" << std::endl;
        
        std::cout << "LRU Order (más reciente → menos reciente):" << std::endl;
        int position = 0;
        for (int frame_id : lru_list) {
            std::cout << "  [" << position << "] Frame " << frame_id;
            if (isPinned(frame_id)) {
                std::cout << " (PINNED)";
            }
            std::cout << std::endl;
            position++;
        }
        
        if (lru_list.empty()) {
            std::cout << "  (Lista vacía)" << std::endl;
        }
        
        std::cout << "\n" << getStatistics() << std::endl;
    }
    
    /**
     * @brief Obtiene el frame menos recientemente usado (sin evict)
     */
    int getLRUFrame() const {
        if (lru_list.empty()) {
            return -1;
        }
        
        // Buscar el primer frame no pinned desde el final
        auto it = lru_list.rbegin();
        while (it != lru_list.rend()) {
            if (pinned_frames.find(*it) == pinned_frames.end()) {
                return *it;
            }
            ++it;
        }
        
        return -1; // Todos están pinned
    }
    
    /**
     * @brief Obtiene el frame más recientemente usado
     */
    int getMRUFrame() const {
        return lru_list.empty() ? -1 : lru_list.front();
    }
    
    // ============================================================================
    // ANÁLISIS Y OPTIMIZACIÓN
    // ============================================================================
    
    /**
     * @brief Analiza la distribución de accesos
     */
    std::string getAccessDistribution() const {
        std::ostringstream ss;
        
        ss << "Distribución de accesos:\n";
        ss << "  Frames en LRU list: " << lru_list.size() << "\n";
        ss << "  Frames pinned: " << pinned_frames.size() << "\n";
        ss << "  Frames evictables: " << getEvictableCount() << "\n";
        
        if (!lru_list.empty()) {
            ss << "  Frame más reciente: " << getMRUFrame() << "\n";
            ss << "  Frame menos reciente: " << getLRUFrame() << "\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Verifica la integridad de la estructura LRU
     */
    bool validateIntegrity() const {
        // Verificar que el tamaño del mapa coincide con la lista
        if (frame_map.size() != lru_list.size()) {
            std::cout << "❌ LRU Integrity: Map size (" << frame_map.size() 
                      << ") != List size (" << lru_list.size() << ")" << std::endl;
            return false;
        }
        
        // Verificar que todos los elementos en la lista están en el mapa
        for (int frame_id : lru_list) {
            if (frame_map.find(frame_id) == frame_map.end()) {
                std::cout << "❌ LRU Integrity: Frame " << frame_id 
                          << " en lista pero no en mapa" << std::endl;
                return false;
            }
        }
        
        // Verificar que no hay duplicados en la lista
        std::unordered_set<int> seen;
        for (int frame_id : lru_list) {
            if (seen.find(frame_id) != seen.end()) {
                std::cout << "❌ LRU Integrity: Frame " << frame_id 
                          << " duplicado en lista" << std::endl;
                return false;
            }
            seen.insert(frame_id);
        }
        
        std::cout << "✅ LRU Integrity: Estructura validada correctamente" << std::endl;
        return true;
    }
    
    // ============================================================================
    // GETTERS ADICIONALES
    // ============================================================================
    
    size_t getCapacity() const { return capacity; }
    size_t getAccessCount() const { return access_count; }
    size_t getEvictionCount() const { return eviction_count; }
    size_t getPinnedCount() const { return pinned_frames.size(); }
    
    /**
     * @brief Obtiene lista de frames pinned
     */
    std::vector<int> getPinnedFrames() const {
        return std::vector<int>(pinned_frames.begin(), pinned_frames.end());
    }
    
    /**
     * @brief Obtiene lista de frames en orden LRU
     */
    std::vector<int> getLRUOrder() const {
        return std::vector<int>(lru_list.begin(), lru_list.end());
    }
};

#endif // LRU_REPLACER_H