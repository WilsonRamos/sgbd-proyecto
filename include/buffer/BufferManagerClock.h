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
 * @brief Buffer Manager with PIN-AWARE Clock Algorithm MEJORADO
 * 
 * Versión corregida que:
 * - ✅ NUNCA evicta páginas con pin_count > 0
 * - ✅ CADA PASADA disminuye pin_count automáticamente
 * - ✅ Garantiza encontrar víctimas eventualmente
 * - ✅ Usa tu PageTable API correctamente
 * - ✅ Integrado con ClockReplacer pin-aware MEJORADO
 * - ✅ SINCRONIZACIÓN: Dirty bits coherentes en BufferPool y PageTable
 * 
 * DIRTY BIT SINCRONIZATION:
 * - ClockBufferPage::is_dirty (usado para display y evicción)
 * - PageTableEntry::dirty_bit (usado para estadísticas y API)
 * - Ambos se mantienen sincronizados en todas las operaciones
 */
class BufferManagerClock {
private:
    size_t pool_size;                                 // Tamaño del buffer pool
    std::vector<ClockBufferPage> buffer_pool;         // Array de frames (buffer pool)
    std::vector<bool> free_frames;                    // Frames libres
    
    std::unique_ptr<PageTable> page_table;            // Page Table (memoria)
    std::unique_ptr<ClockReplacer> clock_replacer;    // Política Clock PIN-AWARE MEJORADA
    
    DiskManagerExtended* disk_manager;                // Referencia al disk manager extendido
    
    // Estadísticas específicas para Clock
    size_t read_operations;
    size_t write_operations;
    size_t page_faults;                              // Páginas no encontradas en memoria
    size_t evictions;                                // Páginas evictadas
    size_t clock_sweeps;                             // Número de sweeps del clock hand
    size_t failed_evictions;                         // Evictions fallidas por pins

public:
    /**
     * @brief Constructor - CORREGIDO para pin-awareness MEJORADA
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
        , failed_evictions(0)
    {
        page_table = std::make_unique<PageTable>();
        
        // ✅ CORREGIDO: ClockReplacer MEJORADO ahora recibe PageTable
        clock_replacer = std::make_unique<ClockReplacer>(pool_size, page_table.get());
        
        std::cout << "🕐 BufferManagerClock PIN-AWARE MEJORADO inicializado:" << std::endl;
        std::cout << "   📦 Pool size: " << pool_size << " frames" << std::endl;
        std::cout << "   🔄 Algoritmo: Clock PIN-AWARE MEJORADO (garantía de víctimas)" << std::endl;
        std::cout << "   ⚡ CADA pasada disminuye pin_count automáticamente" << std::endl;
        std::cout << "   🛡️  Protección total contra evicción incorrecta" << std::endl;
        std::cout << "   🎯 Eventualmente encuentra víctimas SIEMPRE" << std::endl;
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
        // 1. Verificar si la página está en memoria
        PageTableEntry entry;
        if (page_table->findPage(page_id, entry) && entry.valid_bit) {
            // HIT: Página en memoria
            page_table->pinPage(page_id);  // ✅ Incrementar pin count
            
            // Notificar acceso al Clock Replacer
            clock_replacer->recordAccess(entry.frame_id);
            
            std::cout << "✅ Clock HIT: Página " << page_id 
                      << " en frame " << entry.frame_id 
                      << " (new pin_count=" << (entry.pin_count + 1) << ")" << std::endl;
            
            return buffer_pool[entry.frame_id].block;
        }
        
        // 2. MISS: Cargar página del disco
        page_faults++;
        std::cout << "❌ Clock MISS: Página " << page_id << " no en memoria" << std::endl;
        
        // 3. Buscar frame libre o evictar
        int frame_id = getFreeFrame();
        if (frame_id == -1) {
            std::cout << "⚠️  Buffer pool lleno, evictando con Clock PIN-AWARE MEJORADO..." << std::endl;
            frame_id = evictPage();
            if (frame_id == -1) {
                std::cout << "❌ Error: No se pudo evictar ninguna página (algoritmo falló)" << std::endl;
                std::cout << "   🔍 Esto NO debería ocurrir con el algoritmo MEJORADO" << std::endl;
                return nullptr;
            }
        }
        
        // 4. Cargar página del disco usando tu API
        auto page_block = loadPageFromDisk(page_id);
        if (!page_block) {
            std::cout << "❌ Error: No se pudo leer página " << page_id << " del disco" << std::endl;
            return nullptr;
        }
        
        // 5. Colocar en buffer pool
        buffer_pool[frame_id] = ClockBufferPage(page_id, page_block);
        free_frames[frame_id] = false;
        
        // 6. Actualizar Page Table usando tu API
        if (!page_table->insertPage(page_id, frame_id)) {
            std::cout << "❌ Error agregando página " << page_id << " al Page Table" << std::endl;
            return nullptr;
        }
        
        // 7. Pin la página y notificar al Clock Replacer
        page_table->pinPage(page_id);
        clock_replacer->recordAccess(frame_id);
        
        std::cout << "✅ Clock LOAD: Página " << page_id 
                  << " cargada en frame " << frame_id 
                  << " (pinned=1)" << std::endl;
        
        return buffer_pool[frame_id].block;
    }

    /**
     * @brief Desancla una página (permite evicción)
     */
    bool unpinPage(int page_id, bool is_dirty) {
        PageTableEntry entry;
        if (!page_table->findPage(page_id, entry) || !entry.valid_bit) {
            std::cout << "❌ Clock UNPIN: Página " << page_id << " no en memoria" << std::endl;
            return false;
        }
        
        // ✅ SINCRONIZACIÓN: Actualizar ambos dirty bits
        if (is_dirty) {
            // 1. Actualizar dirty en BufferManagerClock
            buffer_pool[entry.frame_id].is_dirty = true;
            // 2. Actualizar dirty en PageTable (se hace en unpinPage())
            std::cout << "💾 Clock: Página " << page_id << " marcada como dirty (ambos lugares)" << std::endl;
        }
        
        // Desanclar usando tu API (esto actualiza PageTable::dirty_bit)
        bool was_unpinned = page_table->unpinPage(page_id, is_dirty);
        
        if (was_unpinned) {
            // Si el pin count llegó a 0, agregar al clock replacer
            PageTableEntry updated_entry;
            if (page_table->findPageReadOnly(page_id, updated_entry) && updated_entry.pin_count == 0) {
                clock_replacer->unpin(entry.frame_id);
                std::cout << "🔓 Clock UNPIN: Página " << page_id 
                          << " disponible para evicción (pin_count=0)" << std::endl;
            } else {
                std::cout << "📌 Clock: Página " << page_id 
                          << " aún pinned (pin_count=" << updated_entry.pin_count << ")" << std::endl;
            }
        }
        
        return was_unpinned;
    }

    /**
     * @brief Crea una nueva página
     */
    std::shared_ptr<Block> newPage(int& page_id) {
        // 1. Generar nuevo page_id usando tu API
        page_id = disk_manager->allocateNewPageId();
        
        // 2. Buscar frame libre o evictar
        int frame_id = getFreeFrame();
        if (frame_id == -1) {
            std::cout << "⚠️  Buffer pool lleno, evictando para nueva página..." << std::endl;
            frame_id = evictPage();
            if (frame_id == -1) {
                std::cout << "❌ Error: No se pudo evictar para nueva página" << std::endl;
                return nullptr;
            }
        }
        
        // 3. Crear nuevo bloque
        PhysicalAddress addr;
        if (!disk_manager->getAddressForPageId(page_id, addr)) {
            std::cout << "❌ Error obteniendo dirección para nueva página " << page_id << std::endl;
            return nullptr;
        }
        
        auto new_block = std::make_shared<Block>(addr, 4096);
        
        // 4. Colocar en buffer pool
        buffer_pool[frame_id] = ClockBufferPage(page_id, new_block);
        buffer_pool[frame_id].is_dirty = true;  // Nueva página siempre es dirty
        free_frames[frame_id] = false;
        
        // 5. Actualizar Page Table
        if (!page_table->insertPage(page_id, frame_id)) {
            std::cout << "❌ Error agregando nueva página " << page_id << " al Page Table" << std::endl;
            return nullptr;
        }
        
        // ✅ SINCRONIZACIÓN: Marcar dirty en PageTable también
        page_table->markDirty(page_id);
        
        // 6. Pin la página y notificar al Clock Replacer
        page_table->pinPage(page_id);
        clock_replacer->recordAccess(frame_id);
        
        std::cout << "✨ Clock NEW: Página " << page_id 
                  << " creada en frame " << frame_id 
                  << " (dirty=true en ambos lugares, pinned=1)" << std::endl;
        
        return new_block;
    }

    /**
     * @brief Fuerza escritura de una página al disco
     */
    bool flushPage(int page_id) {
        PageTableEntry entry;
        if (!page_table->findPageReadOnly(page_id, entry) || !entry.valid_bit) {
            std::cout << "❌ Clock FLUSH: Página " << page_id << " no en memoria" << std::endl;
            return false;
        }
        
        if (buffer_pool[entry.frame_id].is_dirty) {
            if (savePageToDisk(page_id, buffer_pool[entry.frame_id].block)) {
                // ✅ SINCRONIZACIÓN: Limpiar dirty bit en ambos lugares
                buffer_pool[entry.frame_id].is_dirty = false;  // BufferManagerClock
                page_table->clearDirty(page_id);               // PageTable
                write_operations++;
                std::cout << "💾 Clock FLUSH: Página " << page_id << " escrita al disco (ambos dirty bits limpiados)" << std::endl;
                return true;
            } else {
                std::cout << "❌ Error escribiendo página " << page_id << " al disco" << std::endl;
                return false;
            }
        }
        
        std::cout << "ℹ️  Clock: Página " << page_id << " no necesita flush (clean)" << std::endl;
        return true;
    }

    /**
     * @brief Fuerza escritura de todas las páginas dirty
     */
    void flushAllDirtyPages() {
        std::cout << "\n💾 Clock: Flushing todas las páginas dirty..." << std::endl;
        
        int flushed = 0;
        for (size_t i = 0; i < pool_size; ++i) {
            if (!free_frames[i] && buffer_pool[i].is_dirty) {
                if (flushPage(buffer_pool[i].page_id)) {
                    flushed++;
                }
            }
        }
        
        std::cout << "✅ Clock: " << flushed << " páginas dirty escritas al disco" << std::endl;
    }

    /**
     * @brief Muestra estadísticas del buffer manager
     */
    void displayStatistics() const {
        std::cout << "\n📊 ESTADÍSTICAS CLOCK BUFFER MANAGER MEJORADO:" << std::endl;
        std::cout << "   Pool size: " << pool_size << " frames" << std::endl;
        std::cout << "   Frames ocupados: " << (pool_size - std::count(free_frames.begin(), free_frames.end(), true)) << std::endl;
        std::cout << "   Read operations: " << read_operations << std::endl;
        std::cout << "   Write operations: " << write_operations << std::endl;
        std::cout << "   Page Faults: " << page_faults << std::endl;
        std::cout << "   Evictions exitosas: " << evictions << std::endl;
        std::cout << "   Evictions fallidas (pinned): " << failed_evictions << std::endl;
        std::cout << "   Clock Sweeps: " << clock_sweeps << std::endl;
        
        std::cout << "\n📈 RENDIMIENTO:" << std::endl;
        if (read_operations + page_faults > 0) {
            double hit_rate = static_cast<double>(read_operations - page_faults) / 
                             (read_operations + page_faults) * 100.0;
            std::cout << "   Hit Rate: " << std::fixed << std::setprecision(2) 
                      << hit_rate << "%" << std::endl;
        }
        
        std::cout << "\n🔄 CLOCK REPLACER PIN-AWARE MEJORADO:" << std::endl;
        auto stats = clock_replacer->getStats();
        std::cout << "   Frames en uso: " << stats.current_size << "/" << stats.capacity << std::endl;
        std::cout << "   Utilización: " << std::fixed << std::setprecision(1) 
                  << stats.utilization << "%" << std::endl;
        std::cout << "   Clock Hand pos: " << stats.clock_hand_pos << std::endl;
        std::cout << "   Reference bits activos: " << stats.active_refs << std::endl;
        std::cout << "   Segunda oportunidades dadas: " << stats.second_chances << std::endl;
        std::cout << "   Pin decrements (automáticos): " << stats.pin_decrements << std::endl;
        
        std::cout << "\n📋 PAGE TABLE:" << std::endl;
        std::cout << "   Entradas totales: " << page_table->getPageCount() << std::endl;
        
        // 📊 PIN ANALYSIS
        auto all_pages = page_table->getAllPageIds();
        int pinned_pages = 0;
        int max_pin_count = 0;
        for (int page_id : all_pages) {
            PageTableEntry entry;
            if (page_table->findPageReadOnly(page_id, entry)) {
                if (entry.pin_count > 0) {
                    pinned_pages++;
                }
                max_pin_count = std::max(max_pin_count, entry.pin_count);
            }
        }
        
        std::cout << "   Páginas pinned: " << pinned_pages << "/" << all_pages.size() << std::endl;
        std::cout << "   Pin count máximo: " << max_pin_count << std::endl;
        
        if (failed_evictions == 0) {
            std::cout << "\n✅ GARANTÍA CUMPLIDA: Algoritmo MEJORADO siempre encuentra víctimas" << std::endl;
        } else {
            std::cout << "\n⚠️  Evictions fallidas: " << failed_evictions 
                      << " (revisar configuración MAX_PASSES)" << std::endl;
        }
    }

    /**
     * @brief Muestra estado detallado del Clock con tabla formateada
     */
    void displayClockState() const {
        std::cout << "\n🕐 CLOCK BUFFER STATE (PIN-AWARE MEJORADO):" << std::endl;
        std::cout << "Frame  PageID  Status  Dirty  PinCount  RefBit  ClockPos" << std::endl;
        std::cout << "-----  ------  ------  -----  --------  ------  --------" << std::endl;
        
        auto frames_info = getFramesInfo();
        for (size_t i = 0; i < frames_info.size(); ++i) {
            const auto& info = frames_info[i];
            
            std::cout << std::setw(5) << i << "  ";
            std::cout << std::setw(6) << (info.is_free ? "-" : std::to_string(info.page_id)) << "  ";
            std::cout << std::setw(6) << (info.is_free ? "FREE" : "USED") << "  ";
            std::cout << std::setw(5) << (info.is_free ? "-" : (info.is_dirty ? "YES" : "NO")) << "  ";
            std::cout << std::setw(8) << (info.is_free ? "-" : std::to_string(info.pin_count)) << "  ";
            std::cout << std::setw(6) << (info.is_free ? "-" : (info.reference_bit ? "1" : "0")) << "  ";
            std::cout << std::setw(8) << (info.is_clock_hand ? "←HAND" : "") << std::endl;
        }
        
        std::cout << "\n🛡️ Protección PIN-AWARE MEJORADA:" << std::endl;
        std::cout << "✅ NUNCA evicta páginas con pin_count > 0" << std::endl;
        std::cout << "🔄 CADA pasada decrementa pin_count automáticamente" << std::endl;
        std::cout << "⚡ Algoritmo Clock con reference bits activo" << std::endl;
        std::cout << "🎯 GARANTÍA: Eventualmente encuentra víctimas SIEMPRE" << std::endl;
    }

    /**
     * @brief Información detallada de un frame para tabla formateada
     */
    struct FrameInfo {
        int page_id;
        bool is_free;
        bool is_dirty;
        int pin_count;
        bool reference_bit;
        bool is_clock_hand;
    };

    /**
     * @brief Obtiene información detallada de todos los frames para mostrar tabla
     */
    std::vector<FrameInfo> getFramesInfo() const {
        std::vector<FrameInfo> frames_info;
        auto stats = clock_replacer->getStats();
        
        for (size_t i = 0; i < pool_size; ++i) {
            FrameInfo info;
            info.is_free = free_frames[i];
            info.is_clock_hand = (i == stats.clock_hand_pos);
            
            if (!free_frames[i]) {
                // Frame ocupado
                info.page_id = buffer_pool[i].page_id;
                info.is_dirty = buffer_pool[i].is_dirty;
                
                // Obtener pin_count del page table
                PageTableEntry entry;
                if (page_table->findPageReadOnly(buffer_pool[i].page_id, entry)) {
                    info.pin_count = entry.pin_count;
                } else {
                    info.pin_count = 0;
                }
                
                // Obtener reference bit del clock replacer
                info.reference_bit = clock_replacer->getReferenceAt(static_cast<int>(i));
            } else {
                // Frame libre
                info.page_id = -1;
                info.is_dirty = false;
                info.pin_count = 0;
                info.reference_bit = false;
            }
            
            frames_info.push_back(info);
        }
        
        return frames_info;
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
     * @brief HELPER: Sincroniza dirty bit en ambos lugares
     */
    void syncDirtyBit(int page_id, int frame_id, bool dirty) {
        buffer_pool[frame_id].is_dirty = dirty;
        if (dirty) {
            page_table->markDirty(page_id);
        } else {
            page_table->clearDirty(page_id);
        }
    }

    /**
     * @brief Evicta una página usando el algoritmo Clock PIN-AWARE MEJORADO
     */
    int evictPage() {
        clock_sweeps++;
        
        int victim_frame;
        if (!clock_replacer->victim(victim_frame)) {
            failed_evictions++;
            std::cout << "❌ Clock PIN-AWARE MEJORADO: No se encontró víctima" << std::endl;
            std::cout << "   🔍 Esto NO debería ocurrir con el algoritmo MEJORADO" << std::endl;
            std::cout << "   ⚠️  Revisar configuración MAX_PASSES o pin_count iniciales" << std::endl;
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
        
        // Limpiar Page Table entry usando tu API
        int victim_page_id = buffer_pool[victim_frame].page_id;
        page_table->removePage(victim_page_id);
        
        // ✅ SINCRONIZACIÓN: Limpiar frame (esto resetea is_dirty automáticamente)
        buffer_pool[victim_frame] = ClockBufferPage();  // Constructor resetea is_dirty=false
        free_frames[victim_frame] = true;
        
        std::cout << "🎯 Clock PIN-AWARE MEJORADO: Frame " << victim_frame 
                  << " (página " << victim_page_id << ") evictado correctamente" << std::endl;
        
        return victim_frame;
    }

    /**
     * @brief Carga una página del disco usando tu API
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
        
        // 3. Crear Block y leer usando tu API
        auto block = std::make_shared<Block>(addr, 4096);
        if (!disk_manager->readBlock(addr, *block)) {
            std::cout << "❌ Error leyendo bloque desde disco para página " << page_id << std::endl;
            return nullptr;
        }
        
        return block;
    }

    /**
     * @brief Guarda una página al disco usando tu API
     */
    bool savePageToDisk(int page_id, std::shared_ptr<Block> block) {
        // 1. Obtener dirección física
        PhysicalAddress addr;
        if (!disk_manager->getAddressForPageId(page_id, addr)) {
            std::cout << "❌ No se pudo obtener dirección física para página " << page_id << std::endl;
            return false;
        }
        
        // 2. Escribir usando tu API
        return disk_manager->writeBlock(addr, *block);
    }
};

#endif // BUFFER_MANAGER_CLOCK_H