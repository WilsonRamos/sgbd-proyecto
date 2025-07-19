#ifndef BUFFER_ADAPTER_H
#define BUFFER_ADAPTER_H

#include "../buffer/BufferPoolManager.h"
#include <unordered_map>
#include <memory>
#include <cstring>

/**
 * @brief Adaptador para compatibilidad con BufferPoolManager existente
 * 
 * CONCEPTO: Adapter Pattern - Permite que interfaces incompatibles trabajen juntas
 * 
 * Este adaptador mapea los métodos que esperamos del BufferManager
 * a los métodos que realmente tiene nuestro BufferPoolManager.
 */

// === DEFINICIONES DE TIPOS FALTANTES ===
using page_id_t = int;  // Nuestro sistema usa int para page IDs

/**
 * @brief Estructura de página simplificada para hash
 */
struct HashPage {
    page_id_t page_id_;
    char* data_;
    bool is_dirty_;
    static constexpr size_t PAGE_SIZE = 4096;
    char internal_data_[PAGE_SIZE];  // Buffer interno
    
    HashPage(page_id_t page_id, char* data) 
        : page_id_(page_id), data_(data ? data : internal_data_), is_dirty_(false) {
        if (!data) {
            // Inicializar buffer interno
            std::memset(internal_data_, 0, PAGE_SIZE);
        }
    }
    
    char* GetData() { return data_; }
    void WSetDirty(bool dirty) { is_dirty_ = dirty; }
    bool IsDirty() const { return is_dirty_; }
    page_id_t GetPageId() const { return page_id_; }
};

/**
 * @brief Adaptador que mapea BufferPoolManager a la interfaz esperada
 */
class BufferManagerAdapter {
private:
    BufferPoolManager* buffer_manager_;
    std::unordered_map<page_id_t, std::unique_ptr<HashPage>> page_cache_;
    
public:
    explicit BufferManagerAdapter(BufferPoolManager* manager) 
        : buffer_manager_(manager) {}
    
    /**
     * @brief Crea una nueva página
     * MAPEA: NewPage() → createNewPage()
     */
    HashPage* NewPage(page_id_t* page_id) {
        *page_id = buffer_manager_->createNewPage();
        if (*page_id == -1) return nullptr;
        
        // Acceso simplificado - no necesitamos cargar desde disco para páginas nuevas
        auto hash_page = std::make_unique<HashPage>(*page_id, nullptr);
        HashPage* result = hash_page.get();
        page_cache_[*page_id] = std::move(hash_page);
        
        return result;
    }
    
    /**
     * @brief Obtiene una página existente
     * MAPEA: FetchPage() → directamente desde buffer_manager
     */
    HashPage* FetchPage(page_id_t page_id) {
        // Verificar cache primero
        auto it = page_cache_.find(page_id);
        if (it != page_cache_.end()) {
            return it->second.get();
        }
        
        // Para páginas existentes, crear entrada de cache simplificada
        auto hash_page = std::make_unique<HashPage>(page_id, nullptr);
        HashPage* result = hash_page.get();
        page_cache_[page_id] = std::move(hash_page);
        
        return result;
    }
    
    /**
     * @brief Desancla una página
     * MAPEA: UnpinPage() → unpinPage()
     */
    bool UnpinPage(page_id_t page_id, bool is_dirty) {
        auto it = page_cache_.find(page_id);
        if (it != page_cache_.end() && is_dirty) {
            it->second->WSetDirty(true);
        }
        
        return buffer_manager_->unpinPage(page_id, is_dirty);
    }
    
    /**
     * @brief Escribe todas las páginas a disco
     * MAPEA: FlushAllPages() → flushAllPages()
     */
    void FlushAllPages() {
        buffer_manager_->flushAllPages();
    }
    
    /**
     * @brief Borra una página del cache
     */
    void DeletePage(page_id_t page_id) {
        page_cache_.erase(page_id);
        // El BufferPoolManager no tiene deletePage, solo lo removemos del cache
    }
    
    /**
     * @brief Limpia el cache del adaptador
     */
    void ClearCache() {
        page_cache_.clear();
    }
    
    /**
     * @brief Obtiene el BufferPoolManager subyacente
     */
    BufferPoolManager* GetUnderlyingManager() {
        return buffer_manager_;
    }
};

#endif // BUFFER_ADAPTER_H
