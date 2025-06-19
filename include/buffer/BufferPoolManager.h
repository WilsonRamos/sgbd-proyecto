#ifndef BUFFER_POOL_MANAGER_H
#define BUFFER_POOL_MANAGER_H

#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "PageDirectory.h"
#include "PageTable.h"
#include "LRUReplacer.h"
#include "../DiskManager.h"
#include "../Block.h"

/**
 * @brief Estados de operación en páginas
 */
enum class PageOperation {
    READ,
    WRITE
};

/**
 * @brief Información de una página en el buffer pool
 */
struct BufferPage {
    int page_id;
    std::shared_ptr<Block> block;
    bool is_dirty;
    std::chrono::steady_clock::time_point load_time;
    
    BufferPage() : page_id(-1), is_dirty(false) {
        load_time = std::chrono::steady_clock::now();
    }
    
    BufferPage(int pid, std::shared_ptr<Block> blk) 
        : page_id(pid), block(blk), is_dirty(false) {
        load_time = std::chrono::steady_clock::now();
    }
};

/**
 * @brief Buffer Pool Manager - Componente principal del sistema de gestión de memoria
 * 
 * Integra todos los componentes siguiendo la arquitectura de la conferencia CMU:
 * - Page Table (en memoria) para mapeo PageID → FrameID
 * - Page Directory (en disco) para mapeo PageID → ubicación física
 * - LRU Replacer para política de evicción
 * - Buffer Pool (array de frames en memoria)
 * - Integración con DiskManager existente
 */
class BufferPoolManager {
private:
    size_t pool_size;                                 // Tamaño del buffer pool
    std::vector<BufferPage> buffer_pool;              // Array de frames (buffer pool)
    std::vector<bool> free_frames;                    // Frames libres
    
    std::unique_ptr<PageTable> page_table;            // Page Table (memoria)
    std::unique_ptr<PageDirectory> page_directory;    // Page Directory (disco)
    std::unique_ptr<LRUReplacer> lru_replacer;       // Política LRU
    
    DiskManager* disk_manager;                        // Referencia al disk manager existente
    int next_page_id;                                // Próximo Page ID
    
    // Estadísticas
    size_t read_operations;
    size_t write_operations;
    size_t page_faults;                              // Páginas no encontradas en memoria
    size_t evictions;                                // Páginas evictadas

public:
    /**
     * @brief Constructor
     */
    BufferPoolManager(size_t pool_size, DiskManager* dm, const std::string& base_path = "./disk_simulation")
        : pool_size(pool_size)
        , buffer_pool(pool_size)
        , free_frames(pool_size, true)
        , disk_manager(dm)
        , next_page_id(1)
        , read_operations(0)
        , write_operations(0)
        , page_faults(0)
        , evictions(0)
    {
        // Inicializar componentes
        page_table = std::make_unique<PageTable>();
        page_directory = std::make_unique<PageDirectory>(base_path);
        lru_replacer = std::make_unique<LRUReplacer>(pool_size);
        
        std::cout << "🚀 Buffer Pool Manager inicializado:" << std::endl;
        std::cout << "   - Pool size: " << pool_size << " frames" << std::endl;
        std::cout << "   - Page Table: ✓" << std::endl;
        std::cout << "   - Page Directory: ✓" << std::endl;
        std::cout << "   - LRU Replacer: ✓" << std::endl;
    }

    /**
     * @brief Destructor - guarda estado persistente
     */
    ~BufferPoolManager() {
        flushAllPages();
        page_directory->saveToDisk();
        std::cout << "💾 Buffer Pool Manager: Estado guardado" << std::endl;
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
        
        // 3. Buscar en Page Directory (disco)
        PageLocation location;
        if (!page_directory->findPage(page_id, location)) {
            std::cout << "❌ Página " << page_id << " no existe en disco" << std::endl;
            return nullptr;
        }
        
        std::cout << "📀 Página " << page_id << " encontrada en disco: " 
                  << location.file_id << std::endl;
        
        // 4. Cargar página en buffer pool
        return loadPageFromDisk(page_id, location, operation);
    }

    /**
     * @brief Crea una nueva página
     */
    int createNewPage() {
        int new_page_id = next_page_id++;
        
        // Buscar frame libre o evictar
        int frame_id = findFreeFrame();
        if (frame_id == -1) {
            std::cout << "❌ No se pudo crear página: buffer pool lleno" << std::endl;
            return -1;
        }
        
        // Crear nuevo bloque
        PhysicalAddress addr = allocateNewPhysicalAddress();
        auto new_block = std::make_shared<Block>(addr, 4096);
        
        // Registrar en Page Directory
        page_directory->registerPage(new_page_id, addr, 4096);
        
        // Añadir a buffer pool
        buffer_pool[frame_id] = BufferPage(new_page_id, new_block);
        buffer_pool[frame_id].is_dirty = true;  // Nueva página siempre es dirty
        free_frames[frame_id] = false;
        
        // Actualizar Page Table
        page_table->insertPage(new_page_id, frame_id);
        page_table->pinPage(new_page_id);
        page_table->markDirty(new_page_id);
        
        // Actualizar LRU
        lru_replacer->recordAccess(frame_id);
        
        std::cout << "✨ Nueva página " << new_page_id << " creada en Frame " 
                  << frame_id << std::endl;
        
        return new_page_id;
    }

    /**
     * @brief Libera una página (unpin)
     */
    bool unpinPage(int page_id, bool mark_dirty = false) {
        PageTableEntry entry;
        if (!page_table->findPage(page_id, entry)) {
            return false;
        }
        
        bool success = page_table->unpinPage(page_id, mark_dirty);
        if (success && mark_dirty) {
            buffer_pool[entry.frame_id].is_dirty = true;
        }
        
        // Si no está pinned, añadir al LRU replacer
        if (success) {
            PageTableEntry updated_entry;
            page_table->findPage(page_id, updated_entry);
            if (updated_entry.pin_count == 0) {
                lru_replacer->unpin(entry.frame_id);
            }
        }
        
        return success;
    }

    /**
     * @brief Elimina una página del sistema
     */
    bool deletePage(int page_id) {
        PageTableEntry entry;
        if (page_table->findPage(page_id, entry)) {
            if (entry.pin_count > 0) {
                std::cout << "❌ No se puede eliminar página " << page_id 
                          << ": está siendo usada" << std::endl;
                return false;
            }
            
            // Remover de estructuras
            lru_replacer->remove(entry.frame_id);
            page_table->removePage(page_id);
            free_frames[entry.frame_id] = true;
            buffer_pool[entry.frame_id] = BufferPage();
        }
        
        // Remover de Page Directory
        page_directory->removePage(page_id);
        
        std::cout << "🗑️  Página " << page_id << " eliminada completamente" << std::endl;
        return true;
    }

    /**
     * @brief Fuerza escritura de todas las páginas dirty
     */
    void flushAllPages() {
        std::cout << "\n💾 Guardando todas las páginas dirty..." << std::endl;
        
        int flushed_count = 0;
        for (size_t i = 0; i < buffer_pool.size(); ++i) {
            if (!free_frames[i] && buffer_pool[i].is_dirty) {
                flushPageToDisk(buffer_pool[i].page_id, i);
                flushed_count++;
            }
        }
        
        std::cout << "💾 " << flushed_count << " páginas guardadas en disco" << std::endl;
    }

    /**
     * @brief Muestra información completa del buffer pool
     */
    void displayBufferPoolInfo() {
        std::cout << "\n═══════════════════════════════════════════════════════" << std::endl;
        std::cout << "🏊 ESTADO COMPLETO DEL BUFFER POOL" << std::endl;
        std::cout << "═══════════════════════════════════════════════════════" << std::endl;
        
        // Información general
        auto stats = getStats();
        std::cout << "📊 Estadísticas Generales:" << std::endl;
        std::cout << "   - Frames totales: " << pool_size << std::endl;
        std::cout << "   - Frames ocupados: " << stats.occupied_frames << std::endl;
        std::cout << "   - Frames libres: " << stats.free_frames << std::endl;
        std::cout << "   - Páginas dirty: " << stats.dirty_pages << std::endl;
        std::cout << "   - Páginas pinned: " << stats.pinned_pages << std::endl;
        std::cout << "   - Utilización: " << std::fixed << std::setprecision(1) 
                  << stats.utilization << "%" << std::endl;
        
        // Buffer Pool detallado
        std::cout << "\n🎯 Buffer Pool (Frames en Memoria):" << std::endl;
        std::cout << std::setw(8) << "Frame" 
                  << std::setw(8) << "PageID"
                  << std::setw(8) << "Status"
                  << std::setw(8) << "Dirty"
                  << std::setw(8) << "Pin"
                  << std::setw(15) << "LoadTime" << std::endl;
        std::cout << std::string(55, '-') << std::endl;
        
        for (size_t i = 0; i < buffer_pool.size(); ++i) {
            if (!free_frames[i]) {
                PageTableEntry entry;
                page_table->findPage(buffer_pool[i].page_id, entry);
                
                auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - buffer_pool[i].load_time).count();
                
                std::cout << std::setw(8) << i
                          << std::setw(8) << buffer_pool[i].page_id
                          << std::setw(8) << "USED"
                          << std::setw(8) << (buffer_pool[i].is_dirty ? "YES" : "NO")
                          << std::setw(8) << entry.pin_count
                          << std::setw(12) << load_time << "ms" << std::endl;
            } else {
                std::cout << std::setw(8) << i
                          << std::setw(8) << "-"
                          << std::setw(8) << "FREE"
                          << std::setw(8) << "-"
                          << std::setw(8) << "-"
                          << std::setw(15) << "-" << std::endl;
            }
        }
        
        // Mostrar componentes
        page_table->displayInfo();
        page_directory->displayInfo();
        lru_replacer->displayInfo();
        
        // Estadísticas de rendimiento
        std::cout << "\n📈 Rendimiento:" << std::endl;
        std::cout << "   - Operaciones de lectura: " << read_operations << std::endl;
        std::cout << "   - Operaciones de escritura: " << write_operations << std::endl;
        std::cout << "   - Page faults: " << page_faults << std::endl;
        std::cout << "   - Eviciones: " << evictions << std::endl;
        if (read_operations + write_operations > 0) {
            double hit_rate = 1.0 - (static_cast<double>(page_faults) / 
                                   (read_operations + write_operations));
            std::cout << "   - Hit rate: " << std::fixed << std::setprecision(1) 
                      << (hit_rate * 100.0) << "%" << std::endl;
        }
    }

    /**
     * @brief Resumen compacto para demos
     */
    void displayCompactStatus() {
        std::cout << "\n📋 Buffer Pool Status: ";
        int used_frames = 0;
        for (size_t i = 0; i < buffer_pool.size(); ++i) {
            if (!free_frames[i]) {
                used_frames++;
                std::cout << "[F" << i << ":P" << buffer_pool[i].page_id;
                if (buffer_pool[i].is_dirty) std::cout << "D";
                std::cout << "] ";
            }
        }
        std::cout << " (" << used_frames << "/" << pool_size << " frames)" << std::endl;
        
        page_table->displayCompact();
        lru_replacer->displayCompact();
    }

    // Getters para estadísticas
    struct BufferStats {
        size_t total_frames;
        size_t occupied_frames;
        size_t free_frames;
        size_t dirty_pages;
        size_t pinned_pages;
        double utilization;
        size_t total_operations;
        size_t page_faults;
        size_t evictions;
    };
    
    BufferStats getStats() const {
        BufferStats stats = {};
        stats.total_frames = pool_size;
        
        auto page_stats = page_table->getStats();
        stats.occupied_frames = page_stats.total_pages;
        stats.free_frames = pool_size - stats.occupied_frames;
        stats.dirty_pages = page_stats.dirty_pages;
        stats.pinned_pages = page_stats.pinned_pages;
        stats.utilization = pool_size > 0 ? 
            (static_cast<double>(stats.occupied_frames) / pool_size * 100.0) : 0.0;
        stats.total_operations = read_operations + write_operations;
        stats.page_faults = page_faults;
        stats.evictions = evictions;
        
        return stats;
    }

private:
    /**
     * @brief Carga una página desde disco al buffer pool
     */
    std::shared_ptr<Block> loadPageFromDisk(int page_id, const PageLocation& location, PageOperation operation) {
        // Buscar frame libre o evictar
        int frame_id = findFreeFrame();
        if (frame_id == -1) {
            std::cout << "❌ No se pudo cargar página: buffer pool lleno" << std::endl;
            return nullptr;
        }
        
        // Simular carga desde disco
        std::cout << "📀 Cargando página " << page_id << " desde disco..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simular latencia de disco
        
        // Crear bloque y cargarlo
        PhysicalAddress addr;
        // Parsear file_id para obtener PhysicalAddress
        // Formato: "P0_S0_T0_SEC1"
        parsePhysicalAddressFromFileId(location.file_id, addr);
        
        auto block = std::make_shared<Block>(addr, location.size);
        
        // Usar FileSystemSimulator directamente (acceso a través de DiskManager)
        if (disk_manager && disk_manager->getFileSystemPtr()->readBlock(addr, *block)) {
            // Añadir al buffer pool
            buffer_pool[frame_id] = BufferPage(page_id, block);
            free_frames[frame_id] = false;
            
            // Actualizar Page Table
            page_table->insertPage(page_id, frame_id);
            page_table->pinPage(page_id);
            
            if (operation == PageOperation::WRITE) {
                page_table->markDirty(page_id);
                buffer_pool[frame_id].is_dirty = true;
            }
            
            // Actualizar LRU
            lru_replacer->recordAccess(frame_id);
            
            std::cout << "✅ Página " << page_id << " cargada en Frame " << frame_id << std::endl;
            
            if (operation == PageOperation::READ) {
                read_operations++;
            } else {
                write_operations++;
            }
            
            return block;
        }
        
        std::cout << "❌ Error cargando página " << page_id << " desde disco" << std::endl;
        return nullptr;
    }

    /**
     * @brief Encuentra un frame libre o evicta uno
     */
    int findFreeFrame() {
        // Buscar frame libre
        for (size_t i = 0; i < free_frames.size(); ++i) {
            if (free_frames[i]) {
                return static_cast<int>(i);
            }
        }
        
        // No hay frames libres, aplicar política LRU
        int victim_frame;
        if (lru_replacer->victim(victim_frame)) {
            return evictPage(victim_frame);
        }
        
        return -1;
    }

    /**
     * @brief Evicta una página de un frame específico
     */
    int evictPage(int frame_id) {
        if (frame_id < 0 || frame_id >= static_cast<int>(buffer_pool.size()) || 
            free_frames[frame_id]) {
            return -1;
        }
        
        int page_id = buffer_pool[frame_id].page_id;
        std::cout << "🎯 Evictando página " << page_id << " del Frame " << frame_id << std::endl;
        
        // Si está dirty, escribir a disco
        if (buffer_pool[frame_id].is_dirty) {
            flushPageToDisk(page_id, frame_id);
        }
        
        // Remover de Page Table y LRU
        page_table->removePage(page_id);
        lru_replacer->remove(frame_id);
        
        // Liberar frame
        buffer_pool[frame_id] = BufferPage();
        free_frames[frame_id] = true;
        
        evictions++;
        std::cout << "✅ Frame " << frame_id << " liberado por evicción" << std::endl;
        
        return frame_id;
    }

    /**
     * @brief Escribe una página a disco
     */
    void flushPageToDisk(int page_id, int frame_id) {
        if (disk_manager && buffer_pool[frame_id].block) {
            PhysicalAddress addr = buffer_pool[frame_id].block->getAddress();
            disk_manager->getFileSystemPtr()->writeBlock(addr, *buffer_pool[frame_id].block);
            
            buffer_pool[frame_id].is_dirty = false;
            page_table->clearDirty(page_id);
            
            std::cout << "💾 Página " << page_id << " escrita a disco" << std::endl;
        }
    }

    /**
     * @brief Asigna nueva dirección física (integración con sistema existente)
     */
    PhysicalAddress allocateNewPhysicalAddress() {
        // Implementación simple - en un sistema real esto estaría coordinado con DiskManager
        static int sector_counter = 0;
        return PhysicalAddress(0, 0, 0, sector_counter++);
    }

    /**
     * @brief Parsea un file_id para obtener PhysicalAddress
     */
    void parsePhysicalAddressFromFileId(const std::string& file_id, PhysicalAddress& addr) {
        // Formato esperado: "P0_S0_T0_SEC1"
        std::istringstream iss(file_id);
        std::string part;
        
        if (std::getline(iss, part, '_') && part.size() > 1) {
            addr.setPlatter(std::stoi(part.substr(1)));
        }
        if (std::getline(iss, part, '_') && part.size() > 1) {
            addr.setSurface(std::stoi(part.substr(1)));
        }
        if (std::getline(iss, part, '_') && part.size() > 1) {
            addr.setTrack(std::stoi(part.substr(1)));
        }
        if (std::getline(iss, part) && part.size() > 3) {
            addr.setSector(std::stoi(part.substr(3)));
        }
    }
};

#endif // BUFFER_POOL_MANAGER_H