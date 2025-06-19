#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <chrono>

/**
 * @brief Metadatos de una página en el buffer pool
 */
struct PageTableEntry {
    int frame_id;           // ID del frame en el buffer pool
    int pin_count;          // Número de procesos usando la página
    bool dirty_bit;         // Si la página ha sido modificada
    bool valid_bit;         // Si la página es válida en memoria
    std::chrono::steady_clock::time_point last_access_time;  // Para LRU
    
    PageTableEntry() 
        : frame_id(-1), pin_count(0), dirty_bit(false), valid_bit(false)
        , last_access_time(std::chrono::steady_clock::now()) {}
    
    PageTableEntry(int fid) 
        : frame_id(fid), pin_count(0), dirty_bit(false), valid_bit(true)
        , last_access_time(std::chrono::steady_clock::now()) {}
    
    /**
     * @brief Verifica si la página puede ser evictada
     */
    bool canEvict() const {
        return pin_count == 0 && valid_bit;
    }
    
    /**
     * @brief Actualiza el tiempo de acceso
     */
    void updateAccessTime() {
        last_access_time = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief Obtiene el tiempo desde el último acceso en milisegundos
     */
    long long getTimeSinceLastAccess() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_access_time);
        return duration.count();
    }
};

/**
 * @brief Page Table - Mapeo en memoria de PageID a FrameID
 * 
 * - Estructura en memoria (volátil)
 * - Mapea PageID → FrameID
 * - Mantiene metadatos: dirty_bit, pin_count, valid_bit
 * - Usado por el Buffer Pool Manager para localizar páginas en memoria
 */
class PageTable {
private:
    std::unordered_map<int, PageTableEntry> table;  // PageID → PageTableEntry
    int next_frame_id;                              // Próximo frame ID disponible

public:
    /**
     * @brief Constructor
     */
    PageTable() : next_frame_id(0) {}

    /**
     * @brief Inserta una nueva página en la tabla
     */
    bool insertPage(int page_id, int frame_id = -1) {
        if (table.find(page_id) != table.end()) {
            std::cout << "⚠️  Page Table: Página " << page_id << " ya existe" << std::endl;
            return false;
        }
        
        // Asignar frame_id automáticamente si no se especifica
        if (frame_id == -1) {
            frame_id = next_frame_id++;
        } else {
            next_frame_id = std::max(next_frame_id, frame_id + 1);
        }
        
        table[page_id] = PageTableEntry(frame_id);
        
        std::cout << "🗂️  Page Table: Página " << page_id 
                  << " → Frame " << frame_id << std::endl;
        return true;
    }

    /**
     * @brief Busca una página en la tabla
     */
    bool findPage(int page_id, PageTableEntry& entry) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            entry = it->second;
            // Actualizar tiempo de acceso
            it->second.updateAccessTime();
            return true;
        }
        return false;
    }

    /**
     * @brief Elimina una página de la tabla
     */
    bool removePage(int page_id) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            std::cout << "🗂️  Page Table: Eliminada página " << page_id 
                      << " (Frame " << it->second.frame_id << ")" << std::endl;
            table.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Verifica si una página está en memoria
     */
    bool pageInMemory(int page_id) const {
        auto it = table.find(page_id);
        return it != table.end() && it->second.valid_bit;
    }

    /**
     * @brief Obtiene el frame ID de una página
     */
    int getFrameId(int page_id) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            it->second.updateAccessTime();
            return it->second.frame_id;
        }
        return -1;
    }

    /**
     * @brief Incrementa el pin count de una página
     */
    bool pinPage(int page_id) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            it->second.pin_count++;
            it->second.updateAccessTime();
            std::cout << "📌 Pin página " << page_id 
                      << " (count: " << it->second.pin_count << ")" << std::endl;
            return true;
        }
        return false;
    }

    /**
     * @brief Decrementa el pin count de una página
     */
    bool unpinPage(int page_id, bool mark_dirty = false) {
        auto it = table.find(page_id);
        if (it != table.end() && it->second.pin_count > 0) {
            it->second.pin_count--;
            if (mark_dirty) {
                it->second.dirty_bit = true;
            }
            std::cout << "📍 Unpin página " << page_id 
                      << " (count: " << it->second.pin_count;
            if (mark_dirty) std::cout << ", DIRTY";
            std::cout << ")" << std::endl;
            return true;
        }
        return false;
    }

    /**
     * @brief Marca una página como dirty
     */
    bool markDirty(int page_id) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            it->second.dirty_bit = true;
            it->second.updateAccessTime();
            std::cout << "💾 Página " << page_id << " marcada como DIRTY" << std::endl;
            return true;
        }
        return false;
    }

    /**
     * @brief Limpia el dirty bit de una página
     */
    bool clearDirty(int page_id) {
        auto it = table.find(page_id);
        if (it != table.end()) {
            it->second.dirty_bit = false;
            std::cout << "✨ Página " << page_id << " marcada como CLEAN" << std::endl;
            return true;
        }
        return false;
    }

    /**
     * @brief Obtiene todas las páginas que pueden ser evictadas
     */
    std::vector<int> getEvictablePages() const {
        std::vector<int> evictable;
        for (const auto& entry : table) {
            if (entry.second.canEvict()) {
                evictable.push_back(entry.first);
            }
        }
        return evictable;
    }

    /**
     * @brief Encuentra la página menos recientemente usada (LRU) que puede ser evictada
     */
    int getLRUEvictablePage() const {
        int lru_page = -1;
        long long max_time = -1;
        
        for (const auto& entry : table) {
            if (entry.second.canEvict()) {
                long long time_since_access = entry.second.getTimeSinceLastAccess();
                if (time_since_access > max_time) {
                    max_time = time_since_access;
                    lru_page = entry.first;
                }
            }
        }
        
        return lru_page;
    }

    /**
     * @brief Obtiene estadísticas de la tabla
     */
    struct Stats {
        size_t total_pages;
        size_t pinned_pages;
        size_t dirty_pages;
        size_t evictable_pages;
    };
    
    Stats getStats() const {
        Stats stats = {0, 0, 0, 0};
        
        for (const auto& entry : table) {
            stats.total_pages++;
            if (entry.second.pin_count > 0) stats.pinned_pages++;
            if (entry.second.dirty_bit) stats.dirty_pages++;
            if (entry.second.canEvict()) stats.evictable_pages++;
        }
        
        return stats;
    }

    /**
     * @brief Muestra información detallada de la tabla
     */
    void displayInfo() const {
        std::cout << "\n=== PAGE TABLE (MEMORIA) ===" << std::endl;
        
        auto stats = getStats();
        std::cout << "Total páginas: " << stats.total_pages << std::endl;
        std::cout << "Páginas pinned: " << stats.pinned_pages << std::endl;
        std::cout << "Páginas dirty: " << stats.dirty_pages << std::endl;
        std::cout << "Páginas evictables: " << stats.evictable_pages << std::endl;
        
        if (!table.empty()) {
            std::cout << "\n" << std::setw(8) << "PageID" 
                      << std::setw(8) << "FrameID"
                      << std::setw(6) << "Pin"
                      << std::setw(7) << "Dirty"
                      << std::setw(7) << "Valid"
                      << std::setw(12) << "LastAccess" << std::endl;
            std::cout << std::string(48, '-') << std::endl;
            
            for (const auto& entry : table) {
                std::cout << std::setw(8) << entry.first
                          << std::setw(8) << entry.second.frame_id
                          << std::setw(6) << entry.second.pin_count
                          << std::setw(7) << (entry.second.dirty_bit ? "YES" : "NO")
                          << std::setw(7) << (entry.second.valid_bit ? "YES" : "NO")
                          << std::setw(12) << entry.second.getTimeSinceLastAccess() << "ms"
                          << std::endl;
            }
        }
    }

    /**
     * @brief Muestra resumen compacto para demos
     */
    void displayCompact() const {
        std::cout << "Page Table: [";
        bool first = true;
        for (const auto& entry : table) {
            if (!first) std::cout << ", ";
            std::cout << entry.first << "→F" << entry.second.frame_id;
            if (entry.second.pin_count > 0) std::cout << "(P" << entry.second.pin_count << ")";
            if (entry.second.dirty_bit) std::cout << "(D)";
            first = false;
        }
        std::cout << "]" << std::endl;
    }

    // Getters
    size_t getPageCount() const { return table.size(); }
    bool isEmpty() const { return table.empty(); }
    
    /**
     * @brief Obtiene todas las páginas en la tabla
     */
    std::vector<int> getAllPageIds() const {
        std::vector<int> page_ids;
        for (const auto& entry : table) {
            page_ids.push_back(entry.first);
        }
        return page_ids;
    }
};

#endif // PAGE_TABLE_H