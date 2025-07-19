#ifndef EXTENDIBLE_HASH_DISK_H
#define EXTENDIBLE_HASH_DISK_H

#include "HashPage.h"
#include "HashConfig.h"
#include "BufferAdapter.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <iomanip>

/**
 * @brief Hash Extensible con persistencia en disco
 * 
 * CONCEPTOS APLICADOS:
 * 1. BufferManager Integration: Gestión de páginas en memoria/disco
 * 2. Page-Based Storage: Datos organizados en páginas de tamaño fijo
 * 3. Lazy Loading: Cargar páginas solo cuando se necesitan
 * 4. Write-Through: Cambios se persisten inmediatamente
 * 5. Resource Management: Control automático de memoria
 */
template<typename K, typename V>
class ExtendibleHashDisk {
private:
    // Componentes principales
    BufferManagerAdapter* buffer_adapter_;
    page_id_t directory_page_id_;
    uint32_t global_depth_;
    uint32_t bucket_capacity_;
    
    // Control de límites (NUEVO)
    static constexpr uint32_t MAX_GLOBAL_DEPTH = 8;  // Límite razonable
    static constexpr uint32_t MAX_DIRECTORY_SIZE = 256; // 2^8
    static constexpr uint32_t MIN_BUCKET_CAPACITY = 4;
    static constexpr uint32_t MAX_BUCKET_CAPACITY = 16;
    
    // Estadísticas
    mutable uint32_t total_insertions_;
    mutable uint32_t total_splits_;
    mutable uint32_t directory_expansions_;
    
    // Cache de páginas frecuentemente usadas
    mutable std::unordered_map<page_id_t, std::shared_ptr<HashBucketPage<K, V>>> bucket_cache_;
    
public:
    /**
     * @brief Constructor
     * @param buffer_manager BufferPoolManager para gestión de páginas
     * @param config Configuración del hash
     */
    explicit ExtendibleHashDisk(BufferPoolManager* buffer_manager, 
                               const HashConfig& config = HashConfig());
    
    /**
     * @brief Destructor - persiste cambios pendientes
     */
    ~ExtendibleHashDisk();
    
    // === OPERACIONES PRINCIPALES ===
    bool Insert(const K& key, const V& value);
    bool Remove(const K& key);
    bool Find(const K& key, V& value) const;
    
    // === INFORMACIÓN DEL ESTADO ===
    uint32_t GetGlobalDepth() const { return global_depth_; }
    uint32_t GetDirectorySize() const { return 1U << global_depth_; }
    uint32_t GetTotalElements() const;
    uint32_t GetNumberOfBuckets() const;
    double GetLoadFactor() const;
    
    // === ESTADÍSTICAS ===
    uint32_t GetTotalInsertions() const { return total_insertions_; }
    uint32_t GetTotalSplits() const { return total_splits_; }
    uint32_t GetDirectoryExpansions() const { return directory_expansions_; }
    
    // === PERSISTENCIA ===
    bool Flush();  // Fuerza escritura a disco
    bool Verify(); // Verificación de integridad
    
    // === DEBUG Y VISUALIZACIÓN ===
    void DisplayStructure() const;
    void DisplayStatistics() const;
    void DisplayLimitedStructure(uint32_t max_entries = 20) const;
    
private:
    // === FUNCIONES AUXILIARES ===
    uint32_t Hash(const K& key) const;
    uint32_t GetDirectoryIndex(const K& key) const;
    
    // === GESTIÓN DE PÁGINAS ===
    page_id_t CreateBucketPage(uint32_t local_depth);
    HashDirectoryPage* GetDirectoryPage() const;
    HashBucketPage<K, V>* GetBucketPage(page_id_t page_id) const;
    
    // === OPERACIONES DE SPLIT ===
    bool NeedToDoubleDirectory(uint32_t local_depth) const;
    void DoubleDirectory();
    page_id_t SplitBucket(page_id_t bucket_page_id);
    void RedistributeBucketData(HashBucketPage<K, V>* old_bucket, 
                               HashBucketPage<K, V>* new_bucket);
    
    // === CONTROL DE LÍMITES ===
    bool ShouldStopSplitting() const;
    bool IsValidConfiguration() const;
    
    // === GESTIÓN DE CACHE ===
    void ClearCache();
    void EvictFromCache(page_id_t page_id);
};

// =================================================================
// IMPLEMENTACIONES CRÍTICAS
// =================================================================

template<typename K, typename V>
ExtendibleHashDisk<K, V>::ExtendibleHashDisk(BufferPoolManager* buffer_manager, 
                                            const HashConfig& config)
    : buffer_manager_(buffer_manager)
    , global_depth_(std::max(1U, std::min(config.initial_global_depth, MAX_GLOBAL_DEPTH)))
    , bucket_capacity_(std::clamp(config.bucket_capacity, MIN_BUCKET_CAPACITY, MAX_BUCKET_CAPACITY))
    , total_insertions_(0)
    , total_splits_(0)
    , directory_expansions_(0) {
    
    if (!buffer_manager_) {
        throw std::invalid_argument("BufferManager no puede ser null");
    }
    
    // === CONCEPTO: Inicialización de estructura persistente ===
    
    // 1. Crear página de directorio
    auto* directory_page = buffer_manager_->NewPage(&directory_page_id_);
    if (!directory_page) {
        throw std::runtime_error("No se pudo crear página de directorio");
    }
    
    // 2. Inicializar directorio
    auto* hash_dir = reinterpret_cast<HashDirectoryPage*>(directory_page->GetData());
    hash_dir->InitPage(HashDirectoryPage::DIRECTORY_PAGE_TYPE, GetDirectorySize());
    hash_dir->SetGlobalDepth(global_depth_);
    
    // 3. Crear bucket inicial
    page_id_t initial_bucket_id = CreateBucketPage(global_depth_);
    
    // 4. Configurar directorio inicial
    for (uint32_t i = 0; i < GetDirectorySize(); ++i) {
        hash_dir->SetBucketPageId(i, initial_bucket_id);
    }
    
    // 5. Marcar páginas como modificadas (Write-Through)
    directory_page->WSetDirty(true);
    buffer_manager_->UnpinPage(directory_page_id_, true);
    
    std::cout << "✅ Hash Extensible inicializado con BufferManager" << std::endl;
    std::cout << "   📄 Directory Page ID: " << directory_page_id_ << std::endl;
    std::cout << "   📊 Global Depth: " << global_depth_ << std::endl;
    std::cout << "   🪣 Bucket Capacity: " << bucket_capacity_ << std::endl;
}

template<typename K, typename V>
ExtendibleHashDisk<K, V>::~ExtendibleHashDisk() {
    Flush();
    ClearCache();
}

template<typename K, typename V>
uint32_t ExtendibleHashDisk<K, V>::Hash(const K& key) const {
    // === CONCEPTO: Función hash mejorada para mejor distribución ===
    if constexpr (std::is_integral_v<K>) {
        uint32_t k = static_cast<uint32_t>(key);
        // Multiplicación por número primo para mejor distribución
        k = ((k >> 16) ^ k) * 0x45d9f3b;
        k = ((k >> 16) ^ k) * 0x45d9f3b;
        k = (k >> 16) ^ k;
        return k;
    } else if constexpr (std::is_same_v<K, std::string>) {
        std::hash<std::string> hasher;
        return static_cast<uint32_t>(hasher(key));
    } else {
        std::hash<K> hasher;
        return static_cast<uint32_t>(hasher(key));
    }
}

template<typename K, typename V>
uint32_t ExtendibleHashDisk<K, V>::GetDirectoryIndex(const K& key) const {
    uint32_t hash_value = Hash(key);
    uint32_t mask = (1U << global_depth_) - 1;
    return hash_value & mask;
}

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::ShouldStopSplitting() const {
    // === CONCEPTO: Control de límites para evitar explosión ===
    return global_depth_ >= MAX_GLOBAL_DEPTH || 
           GetDirectorySize() >= MAX_DIRECTORY_SIZE ||
           GetNumberOfBuckets() > (MAX_DIRECTORY_SIZE / 2);
}

template<typename K, typename V>
page_id_t ExtendibleHashDisk<K, V>::CreateBucketPage(uint32_t local_depth) {
    // === CONCEPTO: Factory pattern para páginas ===
    page_id_t new_page_id;
    auto* page = buffer_manager_->NewPage(&new_page_id);
    
    if (!page) {
        throw std::runtime_error("No se pudo crear nueva página de bucket");
    }
    
    // Inicializar bucket page
    auto* bucket_page = reinterpret_cast<HashBucketPage<K, V>*>(page->GetData());
    bucket_page->InitPage(HashBucketPage<K, V>::BUCKET_PAGE_TYPE, bucket_capacity_);
    bucket_page->SetLocalDepth(local_depth);
    
    page->WSetDirty(true);
    buffer_manager_->UnpinPage(new_page_id, true);
    
    return new_page_id;
}

template<typename K, typename V>
HashDirectoryPage* ExtendibleHashDisk<K, V>::GetDirectoryPage() const {
    auto* page = buffer_manager_->FetchPage(directory_page_id_);
    if (!page) {
        throw std::runtime_error("No se pudo cargar página de directorio");
    }
    return reinterpret_cast<HashDirectoryPage*>(page->GetData());
}

template<typename K, typename V>
HashBucketPage<K, V>* ExtendibleHashDisk<K, V>::GetBucketPage(page_id_t page_id) const {
    // === CONCEPTO: Cache management para páginas frecuentes ===
    auto cache_it = bucket_cache_.find(page_id);
    if (cache_it != bucket_cache_.end()) {
        return cache_it->second.get();
    }
    
    auto* page = buffer_manager_->FetchPage(page_id);
    if (!page) {
        throw std::runtime_error("No se pudo cargar página de bucket: " + std::to_string(page_id));
    }
    
    auto bucket_page = std::make_shared<HashBucketPage<K, V>>();
    bucket_page->DeserializeFrom(page->GetData());
    
    // Agregar al cache (limitado a 10 páginas)
    if (bucket_cache_.size() >= 10) {
        auto oldest = bucket_cache_.begin();
        bucket_cache_.erase(oldest);
    }
    bucket_cache_[page_id] = bucket_page;
    
    return bucket_page.get();
}

// ====================================================================
// INCLUIR IMPLEMENTACIONES
// ====================================================================
#include "ExtendibleHashImpl.h"

#endif // EXTENDIBLE_HASH_DISK_H
