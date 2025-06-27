#ifndef BUFFER_MANAGER_CLOCK_H
#define BUFFER_MANAGER_CLOCK_H

#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "PageDirectory.h"
#include "PageTable.h"
#include "ClockReplacer.h"
#include "../DiskManagerExtended.h"
#include "../Block.h"
#include "../PhysicalAddress.h"

/**
 * @brief Información de una página en el buffer pool para Clock
 */
struct ClockBufferPage {
    int page_id;
    std::shared_ptr<Block> block;
    bool is_dirty;
    std::chrono::steady_clock::time_point load_time;
    
    ClockBufferPage() : page_id(-1), is_dirty(false) {
        load_time = std::chrono::steady_clock::now();
    }
    
    ClockBufferPage(int pid, std::shared_ptr<Block> blk) 
        : page_id(pid), block(blk), is_dirty(false) {
        load_time = std::chrono::steady_clock::now();
    }
};

/**
 * @brief Buffer Manager with Clock Algorithm - Adaptado a tu API
 * 
 * Versión completamente adaptada que usa:
 * - Tu PageTable API: findPage, insertPage, removePage, etc.
 * - Tu DiskManagerExtended API: readBlock, writeBlock con PhysicalAddress
 * - Tu Block constructor que requiere PhysicalAddress
 * - Page Directory para mapear PageID → PhysicalAddress
 */
class BufferManagerClock {
private:
    size_t pool_size;                                 // Tamaño del buffer pool
    std::vector<ClockBufferPage> buffer_pool;         // Array de frames (buffer pool)
    std::vector<bool> free_frames;                    // Frames libres
    
    std::unique_ptr<PageTable> page_table;            // Page Table (memoria)
    std::unique_ptr<ClockReplacer> clock_replacer;    // Política Clock
    
    DiskManagerExtended* disk_manager;                // Referencia al disk manager extendido
    
    // Estadísticas específicas para Clock
    size_t read_operations;
    size_t write_operations;
    size_t page_faults;                              // Páginas no encontradas en memoria
    size_t evictions;                                // Páginas evictadas
    size_t clock_sweeps;                             // Número de sweeps del clock hand
    size_t second_chances;                           // Reference bits reset to 0

public:
    /**
     * @brief Constructor
     */
    BufferManagerClock(size_t pool_size, DiskManagerExtended* dm)
        : pool_size(pool_size)
        , buffer_pool(pool_size)
        , free_frames(pool_size, true)
        , disk_manager(dm)
        , read_operations(0)
        , write_operations(0)
        , page_faults(0)
        , evictions(0)
        , clock_sweeps(0)
        , second_chances(0)
    {
        page_table = std::make_unique<PageTable>();
        clock_replacer = std::make_unique<ClockReplacer>(pool_size);
        
        std::cout << "🕐 BufferManagerClock inicializado:" << std::endl;
        std::cout << "   📦 Pool size: " << pool_size << " frames" << std::endl;
        std::cout << "   🔄 Algoritmo: Clock (approximation of LRU)" << std::endl;
        std::cout << "   ⚡ Optimizado contra sequential flooding" << std::endl;
    }

    /**
     * @brief Destructor
     */
    ~BufferManagerClock() {
        flushAllDirtyPages();
    }

    /**
     * @brief Obtiene una página del buffer pool o del disco
     */
    std::shared_ptr<Block> fetchPage(int page_id) {
        // 1. Verificar si la página está en memoria usando TU API
        PageTableEntry entry;
        if (page_table->findPage(page_id, entry) && entry.valid_bit) {
            // HIT: Página en memoria
            page_table->pinPage(page_id);  // Incrementar pin count usando TU API
            
            // Notificar acceso al Clock Replacer
            clock_replacer->recordAccess(entry.frame_id);
            
            std::cout << "✅ Clock HIT: Página " << page_id 
                      << " en frame " << entry.frame_id << std::endl;
            
            return buffer_pool[entry.frame_id].block;
        }
        
        // 2. MISS: Cargar página del disco
        page_faults++;
        std::cout << "❌ Clock MISS: Página " << page_id << " no en memoria" << std::endl;
        
        // 3. Buscar frame libre o evictar
        int frame_id = getFreeFrame();
        if (frame_id == -1) {
            std::cout << "⚠️  Buffer pool lleno, evictando con Clock..." << std::endl;
            frame_id = evictPage();
            if (frame_id == -1) {
                std::cout << "❌ Error: No se pudo evictar ninguna página" << std::endl;
                return nullptr;
            }
        }
        
        // 4. Cargar página del disco usando TU API
        auto page_block = loadPageFromDisk(page_id);
        if (!page_block) {
            std::cout << "❌ Error: No se pudo leer página " << page_id << " del disco" << std::endl;
            return nullptr;
        }
        
        // 5. Colocar en buffer pool
        buffer_pool[frame_id] = ClockBufferPage(page_id, page_block);
        free_frames[frame_id] = false;
        
        // 6. Actualizar Page Table usando TU API
        page_table->insertPage(page_id, frame_id);
        page_table->pinPage(page_id);  // Pin la página
        
        // 7. Agregar al Clock Replacer (pero no registrar acceso aún)
        clock_replacer->unpin(frame_id);
        
        read_operations++;
        std::cout << "📖 Clock: Página " << page_id 
                  << " cargada en frame " << frame_id << std::endl;
        
        return page_block;
    }

    /**
     * @brief Libera el pin de una página
     */
    bool unpinPage(int page_id, bool is_dirty = false) {
        PageTableEntry entry;
        if (!page_table->findPageReadOnly(page_id, entry) || !entry.valid_bit) {
            return false;
        }
        
        if (entry.pin_count <= 0) {
            std::cout << "⚠️  Warning: Página " << page_id << " ya está unpinned" << std::endl;
            return false;
        }
        
        // Usar TU API
        page_table->unpinPage(page_id, is_dirty);
        if (is_dirty) {
            buffer_pool[entry.frame_id].is_dirty = true;
        }
        
        // Si pin_count llega a 0, agregar al Clock Replacer
        PageTableEntry updated_entry;
        if (page_table->findPageReadOnly(page_id, updated_entry) && updated_entry.pin_count == 0) {
            clock_replacer->unpin(entry.frame_id);
        }
        
        std::cout << "📍 Clock: Página " << page_id << " unpinned (dirty=" 
                  << (is_dirty ? "YES" : "NO") << ")" << std::endl;
        
        return true;
    }

    /**
     * @brief Crea una nueva página
     */
    std::shared_ptr<Block> newPage(int& page_id) {
        // 1. Crear nueva página usando TU API
        page_id = disk_manager->allocateNewPageId();
        if (page_id == -1) {
            std::cout << "❌ Error: No se pudo allocar nueva página" << std::endl;
            return nullptr;
        }
        
        // 2. Buscar frame libre o evictar
        int frame_id = getFreeFrame();
        if (frame_id == -1) {
            frame_id = evictPage();
            if (frame_id == -1) {
                std::cout << "❌ Error: No se pudo evictar página para nueva" << std::endl;
                return nullptr;
            }
        }
        
        // 3. Crear bloque usando TU API (requiere PhysicalAddress)
        PhysicalAddress temp_addr(0, 0, 0, 1);  // Dirección temporal
        auto new_block = std::make_shared<Block>(temp_addr, 4096);
        
        // 4. Colocar en buffer pool
        buffer_pool[frame_id] = ClockBufferPage(page_id, new_block);
        buffer_pool[frame_id].is_dirty = true;  // Nueva página siempre es dirty
        free_frames[frame_id] = false;
        
        // 5. Actualizar Page Table usando TU API
        page_table->insertPage(page_id, frame_id);
        page_table->pinPage(page_id);
        page_table->markDirty(page_id);
        
        std::cout << "➕ Clock: Nueva página " << page_id 
                  << " creada en frame " << frame_id << std::endl;
        
        return new_block;
    }

    /**
     * @brief Hace flush de una página específica
     */
    bool flushPage(int page_id) {
        PageTableEntry entry;
        if (!page_table->findPageReadOnly(page_id, entry) || !entry.valid_bit) {
            return false;
        }
        
        int frame_id = entry.frame_id;
        if (buffer_pool[frame_id].is_dirty || entry.dirty_bit) {
            // Escribir página al disco usando TU API
            bool success = savePageToDisk(page_id, buffer_pool[frame_id].block);
            if (success) {
                buffer_pool[frame_id].is_dirty = false;
                page_table->clearDirty(page_id);
                write_operations++;
                std::cout << "💾 Clock: Página " << page_id << " flushed al disco" << std::endl;
                return true;
            } else {
                std::cout << "❌ Error flushing página " << page_id << std::endl;
                return false;
            }
        }
        
        return true;  // No estaba dirty, no necesita flush
    }

    /**
     * @brief Hace flush de todas las páginas dirty
     */
    void flushAllDirtyPages() {
        std::cout << "\n💾 Flushing todas las páginas dirty..." << std::endl;
        
        size_t flushed_count = 0;
        for (size_t i = 0; i < pool_size; ++i) {
            if (!free_frames[i] && buffer_pool[i].is_dirty) {
                if (flushPage(buffer_pool[i].page_id)) {
                    flushed_count++;
                }
            }
        }
        
        std::cout << "💾 Clock: " << flushed_count << " páginas flushed" << std::endl;
    }

    /**
     * @brief Elimina una página del buffer pool y del disco
     */
    bool deletePage(int page_id) {
        // Verificar si está en memoria
        PageTableEntry entry;
        if (page_table->findPageReadOnly(page_id, entry) && entry.valid_bit) {
            if (entry.pin_count > 0) {
                std::cout << "❌ Error: No se puede eliminar página pinned " << page_id << std::endl;
                return false;
            }
            
            // Remover del buffer pool
            int frame_id = entry.frame_id;
            free_frames[frame_id] = true;
            buffer_pool[frame_id] = ClockBufferPage();  // Reset
            
            // Remover del Clock Replacer
            clock_replacer->remove(frame_id);
        }
        
        // Remover del Page Table usando TU API
        page_table->removePage(page_id);
        
        // Note: No deallocamos del disco porque tu API no tiene deallocatePage
        std::cout << "🗑️  Clock: Página " << page_id << " eliminada del buffer" << std::endl;
        return true;
    }

    /**
     * @brief Obtiene estadísticas del Buffer Manager Clock
     */
    void displayStatistics() const {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "ESTADÍSTICAS BUFFER MANAGER CLOCK" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        std::cout << "📊 OPERACIONES:" << std::endl;
        std::cout << "   Lecturas: " << read_operations << std::endl;
        std::cout << "   Escrituras: " << write_operations << std::endl;
        std::cout << "   Page Faults: " << page_faults << std::endl;
        std::cout << "   Evictions: " << evictions << std::endl;
        std::cout << "   Clock Sweeps: " << clock_sweeps << std::endl;
        std::cout << "   Second Chances: " << second_chances << std::endl;
        
        std::cout << "\n📈 RENDIMIENTO:" << std::endl;
        if (read_operations + page_faults > 0) {
            double hit_rate = static_cast<double>(read_operations - page_faults) / 
                             (read_operations + page_faults) * 100.0;
            std::cout << "   Hit Rate: " << std::fixed << std::setprecision(2) 
                      << hit_rate << "%" << std::endl;
        }
        
        std::cout << "\n🔄 CLOCK REPLACER:" << std::endl;
        auto stats = clock_replacer->getStats();
        std::cout << "   Frames en uso: " << stats.current_size << "/" << stats.capacity << std::endl;
        std::cout << "   Utilización: " << std::fixed << std::setprecision(1) 
                  << stats.utilization << "%" << std::endl;
        std::cout << "   Clock Hand pos: " << stats.clock_hand_pos << std::endl;
        std::cout << "   Reference bits activos: " << stats.active_refs << std::endl;
        
        std::cout << "\n📋 PAGE TABLE:" << std::endl;
        std::cout << "   Entradas totales: " << page_table->getPageCount() << std::endl;
        
        std::cout << "\n💾 BUFFER POOL:" << std::endl;
        size_t used_frames = 0;
        size_t dirty_pages = 0;
        for (size_t i = 0; i < pool_size; ++i) {
            if (!free_frames[i]) {
                used_frames++;
                if (buffer_pool[i].is_dirty) dirty_pages++;
            }
        }
        std::cout << "   Frames usados: " << used_frames << "/" << pool_size << std::endl;
        std::cout << "   Páginas dirty: " << dirty_pages << std::endl;
    }

    /**
     * @brief Muestra el estado actual del Clock
     */
    void displayClockState() const {
        std::cout << "\n🕐 ESTADO DEL CLOCK ALGORITHM:" << std::endl;
        clock_replacer->displayInfo();
        
        std::cout << "\n📄 PÁGINAS EN BUFFER POOL:" << std::endl;
        for (size_t i = 0; i < pool_size; ++i) {
            std::cout << "Frame[" << i << "]: ";
            if (!free_frames[i]) {
                PageTableEntry entry;
                bool found = page_table->findPageReadOnly(buffer_pool[i].page_id, entry);
                std::cout << "Page " << buffer_pool[i].page_id 
                          << " (pins=" << (found ? entry.pin_count : 0)
                          << ", dirty=" << (buffer_pool[i].is_dirty ? "Y" : "N") << ")";
            } else {
                std::cout << "LIBRE";
            }
            std::cout << std::endl;
        }
    }

    /**
     * @brief Muestra versión compacta del estado
     */
    void displayCompactState() const {
        std::cout << "\n🕐 Clock State: ";
        clock_replacer->displayCompact();
    }

private:
    /**
     * @brief Busca un frame libre
     */
    int getFreeFrame() {
        for (size_t i = 0; i < pool_size; ++i) {
            if (free_frames[i]) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Evicta una página usando el algoritmo Clock
     */
    int evictPage() {
        clock_sweeps++;
        
        int victim_frame;
        if (!clock_replacer->victim(victim_frame)) {
            std::cout << "❌ Clock: No se encontró víctima para evictar" << std::endl;
            return -1;
        }
        
        evictions++;
        
        // Si la página víctima está dirty, hacer flush
        if (buffer_pool[victim_frame].is_dirty) {
            int victim_page_id = buffer_pool[victim_frame].page_id;
            if (!flushPage(victim_page_id)) {
                std::cout << "❌ Error flushing página víctima " << victim_page_id << std::endl;
                return -1;
            }
        }
        
        // Limpiar Page Table entry usando TU API
        int victim_page_id = buffer_pool[victim_frame].page_id;
        page_table->removePage(victim_page_id);
        
        // Limpiar frame
        buffer_pool[victim_frame] = ClockBufferPage();
        free_frames[victim_frame] = true;
        
        std::cout << "🎯 Clock: Frame " << victim_frame 
                  << " (página " << victim_page_id << ") evictado" << std::endl;
        
        return victim_frame;
    }

    /**
     * @brief Carga una página del disco usando TU API
     */
    std::shared_ptr<Block> loadPageFromDisk(int page_id) {
        // 1. Buscar ubicación en Page Directory
        PageLocation location;
        if (!disk_manager->findPageLocation(page_id, location)) {
            std::cout << "❌ Página " << page_id << " no encontrada en Page Directory" << std::endl;
            return nullptr;
        }
        
        // 2. Convertir ubicación a PhysicalAddress
        PhysicalAddress addr;
        if (!disk_manager->getAddressForPageId(page_id, addr)) {
            std::cout << "❌ No se pudo obtener dirección física para página " << page_id << std::endl;
            return nullptr;
        }
        
        // 3. Crear Block y leer usando TU API
        auto block = std::make_shared<Block>(addr, 4096);
        if (!disk_manager->readBlock(addr, *block)) {
            std::cout << "❌ Error leyendo bloque desde disco para página " << page_id << std::endl;
            return nullptr;
        }
        
        return block;
    }

    /**
     * @brief Guarda una página al disco usando TU API
     */
    bool savePageToDisk(int page_id, std::shared_ptr<Block> block) {
        // 1. Obtener dirección física
        PhysicalAddress addr;
        if (!disk_manager->getAddressForPageId(page_id, addr)) {
            std::cout << "❌ No se pudo obtener dirección física para página " << page_id << std::endl;
            return false;
        }
        
        // 2. Escribir usando TU API
        return disk_manager->writeBlock(addr, *block);
    }
};

#endif // BUFFER_MANAGER_CLOCK_H