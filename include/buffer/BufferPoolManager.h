#ifndef BUFFER_POOL_MANAGER_H
#define BUFFER_POOL_MANAGER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "../DiskManagerExtended.h"
#include "../Block.h"
#include "PageTable.h"
#include "LRUReplacer.h"
// ✅ USAR PageLocation de PageDirectory.h - NO REDEFINIR
#include "PageDirectory.h"

/**
 * @brief Operaciones posibles en páginas
 */
enum class PageOperation {
    READ,
    WRITE
};

/**
 * @brief Frame en el buffer pool
 */
struct BufferPage {
    std::shared_ptr<Block> block;
    int page_id;
    bool is_dirty;
    bool is_pinned;
    size_t access_count;
    
    BufferPage() : page_id(-1), is_dirty(false), is_pinned(false), access_count(0) {
        // ✅ CREAR Block con dirección física dummy y tamaño por defecto
        PhysicalAddress dummy_addr(0, 0, 0, 0);
        block = std::make_shared<Block>(dummy_addr, 4096);
    }
};

/**
 * @brief Buffer Pool Manager
 * 
 * Gestiona el buffer pool con políticas LRU/Clock
 * Coordina entre memoria y disco
 * Mantiene Page Table y estadísticas
 */
class BufferPoolManager {
private:
    std::vector<BufferPage> buffer_pool;            // Pool de frames
    std::unique_ptr<PageTable> page_table;          // Page Table para mapeo
    std::unique_ptr<LRUReplacer> lru_replacer;      // Algoritmo de reemplazo
    
    // ✅ CORREGIR ORDEN DE INICIALIZACIÓN
    DiskManagerExtended* disk_manager;              // Referencia al DiskManager
    size_t pool_size;                               // Tamaño del pool
    size_t next_free_frame;                         // Próximo frame libre
    
    // Estadísticas
    mutable size_t read_operations;
    mutable size_t write_operations;
    mutable size_t page_faults;
    mutable size_t evictions;

public:
    /**
     * @brief Constructor - ✅ ORDEN CORREGIDO
     */
    BufferPoolManager(size_t pool_size, DiskManagerExtended* disk_mgr)
        : disk_manager(disk_mgr)                    // Primero
        , pool_size(pool_size)                      // Segundo
        , next_free_frame(0)
        , read_operations(0)
        , write_operations(0)
        , page_faults(0)
        , evictions(0)
    {
        // Inicializar buffer pool
        buffer_pool.resize(pool_size);
        
        // Inicializar componentes
        page_table = std::make_unique<PageTable>();
        lru_replacer = std::make_unique<LRUReplacer>(pool_size);
        
        std::cout << "🚀 Buffer Pool Manager inicializado:" << std::endl;
        std::cout << "   - Pool size: " << pool_size << " frames" << std::endl;
        std::cout << "   - Page Table: ✓" << std::endl;
        std::cout << "   - Page Directory: ✓ (gestionado por DiskManager)" << std::endl;
        std::cout << "   - LRU Replacer: ✓" << std::endl;
    }

    /**
     * @brief Destructor - guarda estado persistente
     */
    ~BufferPoolManager() {
        flushAllPages();
        std::cout << "💾 Buffer Pool Manager: Estado guardado" << std::endl;
    }

    // ============================================================================
    // OPERACIONES PRINCIPALES
    // ============================================================================
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Verifica si una página está en el buffer
     */
    bool isPageInBuffer(int page_id) const {
        PageTableEntry entry;
        return page_table->findPage(page_id, entry);
    }
    
    /**
     * @brief Solicita una página para operación (READ/WRITE)
     */
    std::shared_ptr<Block> requestPage(int page_id, PageOperation operation) {
        std::cout << "\n🔍 Solicitando página " << page_id 
                  << " para " << (operation == PageOperation::READ ? "LECTURA" : "ESCRITURA") 
                  << std::endl;
        
        // 1. Verificar si está en Page Table (memoria)
        PageTableEntry entry;
        if (page_table->findPage(page_id, entry)) {
            std::cout << "✅ Página " << page_id << " encontrada en memoria (Frame " 
                      << entry.frame_id << ")" << std::endl;
            
            // Pin la página
            page_table->pinPage(page_id);
            
            // Actualizar LRU
            lru_replacer->recordAccess(entry.frame_id);
            
            // Marcar como dirty si es operación de escritura
            if (operation == PageOperation::WRITE) {
                page_table->markDirty(page_id);
                buffer_pool[entry.frame_id].is_dirty = true;
            }
            
            if (operation == PageOperation::READ) {
                read_operations++;
            } else {
                write_operations++;
            }
            
            return buffer_pool[entry.frame_id].block;
        }
        
        // 2. Page fault - no está en memoria
        page_faults++;
        std::cout << "❌ PAGE FAULT: Página " << page_id << " no está en memoria" << std::endl;
        
        // 3. Buscar en Page Directory (disco) - consultado desde DiskManager
        PageLocation location;
        if (!disk_manager->findPageLocation(page_id, location)) {
            std::cout << "❌ Página " << page_id << " no existe en disco" << std::endl;
            return nullptr;
        }
        
        std::cout << "📀 Página " << page_id << " encontrada en disco: " 
                  << location.file_id << std::endl;
        
        // 4. Obtener frame libre
        int frame_id = getAvailableFrame();
        if (frame_id == -1) {
            std::cout << "❌ No hay frames disponibles" << std::endl;
            return nullptr;
        }
        
        // 5. ✅ CREAR Block con dirección física apropiada
        PhysicalAddress addr(0, 0, 0, page_id); // Simplificado para educación
        Block loaded_block(addr, 4096);
        
        // Simular lectura desde disco
        if (!disk_manager->readBlock(addr, loaded_block)) {
            std::cout << "❌ Error leyendo página desde disco" << std::endl;
            return nullptr;
        }
        
        // 6. Actualizar buffer pool
        buffer_pool[frame_id].block = std::make_shared<Block>(loaded_block);
        buffer_pool[frame_id].page_id = page_id;
        buffer_pool[frame_id].is_dirty = false;
        buffer_pool[frame_id].is_pinned = true;
        buffer_pool[frame_id].access_count++;
        
        // 7. Actualizar Page Table
        page_table->insertPage(page_id, frame_id);
        if (operation == PageOperation::WRITE) {
            page_table->markDirty(page_id);
            buffer_pool[frame_id].is_dirty = true;
        }
        
        // 8. Actualizar LRU
        lru_replacer->recordAccess(frame_id);
        
        if (operation == PageOperation::READ) {
            read_operations++;
        } else {
            write_operations++;
        }
        
        std::cout << "✅ Página " << page_id << " cargada en Frame " << frame_id << std::endl;
        return buffer_pool[frame_id].block;
    }
    
    /**
     * @brief Libera una página (unpin)
     */
    bool releasePage(int page_id) {
        PageTableEntry entry;
        if (!page_table->findPage(page_id, entry)) {
            return false;
        }
        
        page_table->unpinPage(page_id);
        buffer_pool[entry.frame_id].is_pinned = false;
        
        // Notificar al replacer que el frame está disponible
        lru_replacer->unpin(entry.frame_id);
        
        std::cout << "📌 Página " << page_id << " liberada (Frame " << entry.frame_id << ")" << std::endl;
        return true;
    }
    
    /**
     * @brief Fuerza escritura de una página a disco
     */
    bool flushPage(int page_id) {
        PageTableEntry entry;
        if (!page_table->findPage(page_id, entry)) {
            return false;
        }
        
        if (buffer_pool[entry.frame_id].is_dirty) {
            // Obtener ubicación física de la página
            PageLocation location;
            if (disk_manager->findPageLocation(page_id, location)) {
                // Crear dirección física desde la ubicación
                PhysicalAddress addr(0, 0, 0, page_id); // Simplificado
                
                // Escribir a disco
                if (disk_manager->writeBlock(addr, *buffer_pool[entry.frame_id].block)) {
                    buffer_pool[entry.frame_id].is_dirty = false;
                    page_table->clearDirty(page_id);
                    write_operations++;
                    
                    std::cout << "💾 Página " << page_id << " escrita a disco" << std::endl;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * @brief Fuerza escritura de todas las páginas dirty
     */
    void flushAllPages() {
        std::cout << "💾 Escribiendo todas las páginas dirty a disco..." << std::endl;
        
        int flushed_count = 0;
        for (size_t i = 0; i < buffer_pool.size(); i++) {
            if (buffer_pool[i].page_id != -1 && buffer_pool[i].is_dirty) {
                if (flushPage(buffer_pool[i].page_id)) {
                    flushed_count++;
                }
            }
        }
        
        std::cout << "✅ " << flushed_count << " páginas escritas a disco" << std::endl;
    }
    
    // ============================================================================
    // GESTIÓN DE FRAMES
    // ============================================================================
    
    /**
     * @brief Obtiene un frame disponible
     */
    int getAvailableFrame() {
        // 1. Buscar frame libre
        for (size_t i = 0; i < buffer_pool.size(); i++) {
            if (buffer_pool[i].page_id == -1) {
                return static_cast<int>(i);
            }
        }
        
        // 2. No hay frames libres - usar LRU para encontrar víctima
        int victim_frame = lru_replacer->evict();
        if (victim_frame == -1) {
            return -1; // Todos los frames están pinned
        }
        
        // 3. Si la víctima está dirty, escribirla a disco
        if (buffer_pool[victim_frame].is_dirty) {
            flushPage(buffer_pool[victim_frame].page_id);
        }
        
        // 4. Remover de Page Table
        page_table->removePage(buffer_pool[victim_frame].page_id);
        
        // 5. Limpiar frame
        buffer_pool[victim_frame].page_id = -1;
        buffer_pool[victim_frame].is_dirty = false;
        buffer_pool[victim_frame].is_pinned = false;
        
        evictions++;
        std::cout << "🔄 Frame " << victim_frame << " liberado (eviction)" << std::endl;
        
        return victim_frame;
    }
    
    // ============================================================================
    // ESTADÍSTICAS Y INFORMACIÓN
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas del buffer pool
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        int used_frames = 0;
        int dirty_frames = 0;
        int pinned_frames = 0;
        
        for (const auto& frame : buffer_pool) {
            if (frame.page_id != -1) {
                used_frames++;
                if (frame.is_dirty) dirty_frames++;
                if (frame.is_pinned) pinned_frames++;
            }
        }
        
        ss << "=== ESTADÍSTICAS BUFFER POOL ===\n";
        ss << "Pool Size: " << pool_size << " frames\n";
        ss << "Frames Usados: " << used_frames << "/" << pool_size << "\n";
        ss << "Frames Dirty: " << dirty_frames << "\n";
        ss << "Frames Pinned: " << pinned_frames << "\n";
        ss << "\n--- Operaciones ---\n";
        ss << "Lecturas: " << read_operations << "\n";
        ss << "Escrituras: " << write_operations << "\n";
        ss << "Page Faults: " << page_faults << "\n";
        ss << "Evictions: " << evictions << "\n";
        
        if (read_operations + write_operations > 0) {
            double hit_rate = 1.0 - (double)page_faults / (read_operations + write_operations);
            ss << "Hit Rate: " << std::fixed << std::setprecision(2) << (hit_rate * 100) << "%\n";
        }
        
        return ss.str();
    }
    
    /**
     * @brief Muestra estado actual del buffer pool
     */
    void display() const {
        std::cout << "\n🗂️ ESTADO DEL BUFFER POOL" << std::endl;
        std::cout << "==========================" << std::endl;
        
        for (size_t i = 0; i < buffer_pool.size(); i++) {
            const auto& frame = buffer_pool[i];
            std::cout << "Frame[" << i << "]: ";
            
            if (frame.page_id == -1) {
                std::cout << "LIBRE" << std::endl;
            } else {
                std::cout << "PageID=" << frame.page_id 
                         << " | Dirty=" << (frame.is_dirty ? "✓" : "✗")
                         << " | Pinned=" << (frame.is_pinned ? "✓" : "✗")
                         << " | Access=" << frame.access_count << std::endl;
            }
        }
        
        std::cout << "\n" << getStatistics() << std::endl;
    }

    // ============================================================================
    // GETTERS
    // ============================================================================
    
    size_t getPoolSize() const { return pool_size; }
    size_t getPageFaults() const { return page_faults; }
    size_t getReadOperations() const { return read_operations; }
    size_t getWriteOperations() const { return write_operations; }
    size_t getEvictions() const { return evictions; }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Obtiene información de uso del buffer
     */
    double getUtilization() const {
        int used_frames = 0;
        for (const auto& frame : buffer_pool) {
            if (frame.page_id != -1) {
                used_frames++;
            }
        }
        return (double)used_frames / pool_size;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Verifica si el buffer pool está lleno
     */
    bool isFull() const {
        for (const auto& frame : buffer_pool) {
            if (frame.page_id == -1) {
                return false;
            }
        }
        return true;
    }
};

#endif // BUFFER_POOL_MANAGER_H