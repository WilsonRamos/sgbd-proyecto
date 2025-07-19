#ifndef EXTENDIBLE_HASH_IMPL_H
#define EXTENDIBLE_HASH_IMPL_H

#include "ExtendibleHashDisk.h"

/**
 * @brief Implementaciones de las operaciones principales
 * 
 * CONCEPTOS EXPLICADOS:
 * 1. ACID Properties: Atomicidad en operaciones
 * 2. Page Lifecycle: Pin → Modify → Dirty → Unpin
 * 3. Error Handling: Manejo robusto de errores
 * 4. Resource Management: RAII para páginas
 */

// =================================================================
// OPERACIÓN INSERT CON CONTROL DE LÍMITES
// =================================================================

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::Insert(const K& key, const V& value) {
    total_insertions_++;
    
    try {
        // === PASO 1: Control de límites preventivo ===
        if (ShouldStopSplitting()) {
            std::cout << "⚠️ LÍMITE ALCANZADO: Inserción rechazada para prevenir explosión" << std::endl;
            std::cout << "   Global Depth: " << global_depth_ << "/" << MAX_GLOBAL_DEPTH << std::endl;
            std::cout << "   Directory Size: " << GetDirectorySize() << "/" << MAX_DIRECTORY_SIZE << std::endl;
            return false;
        }
        
        // === PASO 2: Obtener índice del directorio ===
        uint32_t dir_index = GetDirectoryIndex(key);
        
        // === PASO 3: Cargar página de directorio ===
        auto* directory_page = GetDirectoryPage();
        page_id_t bucket_page_id = directory_page->GetBucketPageId(dir_index);
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
        // === PASO 4: Intentar inserción en bucket existente ===
        auto* bucket_page = GetBucketPage(bucket_page_id);
        
        if (bucket_page->Insert(key, value)) {
            // Éxito: Marcar página como modificada
            auto* page = buffer_manager_->FetchPage(bucket_page_id);
            bucket_page->SerializeTo(page->GetData());
            page->WSetDirty(true);
            buffer_manager_->UnpinPage(bucket_page_id, true);
            return true;
        }
        
        // === PASO 5: Bucket lleno → Necesita split ===
        std::cout << "🔄 Bucket lleno (ID: " << bucket_page_id << "), iniciando split..." << std::endl;
        
        // Verificar si necesitamos duplicar directorio
        if (NeedToDoubleDirectory(bucket_page->GetLocalDepth())) {
            DoubleDirectory();
            directory_expansions_++;
        }
        
        // Realizar split
        page_id_t new_bucket_id = SplitBucket(bucket_page_id);
        total_splits_++;
        
        std::cout << "✅ Split completado: " << bucket_page_id << " → " << new_bucket_id << std::endl;
        
        // === PASO 6: Intentar inserción nuevamente ===
        dir_index = GetDirectoryIndex(key);
        directory_page = GetDirectoryPage();
        bucket_page_id = directory_page->GetBucketPageId(dir_index);
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
        bucket_page = GetBucketPage(bucket_page_id);
        bool success = bucket_page->Insert(key, value);
        
        if (success) {
            auto* page = buffer_manager_->FetchPage(bucket_page_id);
            bucket_page->SerializeTo(page->GetData());
            page->WSetDirty(true);
            buffer_manager_->UnpinPage(bucket_page_id, true);
        }
        
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error en Insert: " << e.what() << std::endl;
        return false;
    }
}

// =================================================================
// OPERACIÓN FIND (BÚSQUEDA)
// =================================================================

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::Find(const K& key, V& value) const {
    try {
        // === CONCEPTO: Operación de solo lectura optimizada ===
        
        uint32_t dir_index = GetDirectoryIndex(key);
        
        auto* directory_page = GetDirectoryPage();
        page_id_t bucket_page_id = directory_page->GetBucketPageId(dir_index);
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
        auto* bucket_page = GetBucketPage(bucket_page_id);
        bool found = bucket_page->Find(key, value);
        
        // No necesitamos unpin porque GetBucketPage maneja el cache
        return found;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error en Find: " << e.what() << std::endl;
        return false;
    }
}

// =================================================================
// OPERACIÓN REMOVE (ELIMINACIÓN)
// =================================================================

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::Remove(const K& key) {
    try {
        uint32_t dir_index = GetDirectoryIndex(key);
        
        auto* directory_page = GetDirectoryPage();
        page_id_t bucket_page_id = directory_page->GetBucketPageId(dir_index);
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
        auto* bucket_page = GetBucketPage(bucket_page_id);
        bool removed = bucket_page->Remove(key);
        
        if (removed) {
            auto* page = buffer_manager_->FetchPage(bucket_page_id);
            bucket_page->SerializeTo(page->GetData());
            page->WSetDirty(true);
            buffer_manager_->UnpinPage(bucket_page_id, true);
        }
        
        return removed;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error en Remove: " << e.what() << std::endl;
        return false;
    }
}

// =================================================================
// SPLIT DE BUCKET CON CONTROL DE RECURSOS
// =================================================================

template<typename K, typename V>
page_id_t ExtendibleHashDisk<K, V>::SplitBucket(page_id_t bucket_page_id) {
    // === CONCEPTO: Operación transaccional de split ===
    
    // 1. Cargar bucket original
    auto* old_bucket = GetBucketPage(bucket_page_id);
    uint32_t old_local_depth = old_bucket->GetLocalDepth();
    
    // 2. Crear nuevo bucket
    page_id_t new_bucket_id = CreateBucketPage(old_local_depth + 1);
    auto* new_bucket = GetBucketPage(new_bucket_id);
    
    // 3. Incrementar local depth del bucket original
    old_bucket->SetLocalDepth(old_local_depth + 1);
    
    // 4. Redistribuir datos
    RedistributeBucketData(old_bucket, new_bucket);
    
    // 5. Actualizar directorio
    auto* directory_page = GetDirectoryPage();
    uint32_t dir_size = GetDirectorySize();
    uint32_t step = 1U << old_local_depth;
    
    for (uint32_t i = 0; i < dir_size; i += step) {
        if (directory_page->GetBucketPageId(i) == bucket_page_id) {
            uint32_t bit_check = 1U << old_local_depth;
            if (i & bit_check) {
                // Actualizar referencias al nuevo bucket
                for (uint32_t j = 0; j < (1U << (global_depth_ - (old_local_depth + 1))); ++j) {
                    uint32_t index = i + j * (1U << (old_local_depth + 1));
                    if (index < dir_size) {
                        directory_page->SetBucketPageId(index, new_bucket_id);
                    }
                }
            }
        }
    }
    
    // 6. Persistir cambios
    auto* dir_page = buffer_manager_->FetchPage(directory_page_id_);
    directory_page->SerializeTo(dir_page->GetData());
    dir_page->WSetDirty(true);
    buffer_manager_->UnpinPage(directory_page_id_, true);
    
    auto* old_page = buffer_manager_->FetchPage(bucket_page_id);
    old_bucket->SerializeTo(old_page->GetData());
    old_page->WSetDirty(true);
    buffer_manager_->UnpinPage(bucket_page_id, true);
    
    auto* new_page = buffer_manager_->FetchPage(new_bucket_id);
    new_bucket->SerializeTo(new_page->GetData());
    new_page->WSetDirty(true);
    buffer_manager_->UnpinPage(new_bucket_id, true);
    
    return new_bucket_id;
}

// =================================================================
// REDISTRIBUCIÓN DE DATOS
// =================================================================

template<typename K, typename V>
void ExtendibleHashDisk<K, V>::RedistributeBucketData(
    HashBucketPage<K, V>* old_bucket, 
    HashBucketPage<K, V>* new_bucket) {
    
    // === CONCEPTO: Rehashing con nuevo local depth ===
    
    auto all_data = old_bucket->GetAllPairs();
    old_bucket->Clear();
    
    uint32_t new_local_depth = old_bucket->GetLocalDepth();
    
    for (const auto& pair : all_data) {
        uint32_t hash_value = Hash(pair.first);
        uint32_t bit_position = 1U << (new_local_depth - 1);
        
        if (hash_value & bit_position) {
            new_bucket->Insert(pair.first, pair.second);
        } else {
            old_bucket->Insert(pair.first, pair.second);
        }
    }
}

// =================================================================
// DUPLICACIÓN DE DIRECTORIO
// =================================================================

template<typename K, typename V>
void ExtendibleHashDisk<K, V>::DoubleDirectory() {
    // === CONCEPTO: Expansión exponencial controlada ===
    
    if (global_depth_ >= MAX_GLOBAL_DEPTH) {
        throw std::runtime_error("Límite máximo de global depth alcanzado");
    }
    
    auto* directory_page = GetDirectoryPage();
    uint32_t old_size = GetDirectorySize();
    
    global_depth_++;
    uint32_t new_size = GetDirectorySize();
    
    // Duplicar entradas existentes
    for (uint32_t i = 0; i < old_size; ++i) {
        page_id_t bucket_id = directory_page->GetBucketPageId(i);
        directory_page->SetBucketPageId(old_size + i, bucket_id);
    }
    
    directory_page->SetGlobalDepth(global_depth_);
    
    auto* page = buffer_manager_->FetchPage(directory_page_id_);
    directory_page->SerializeTo(page->GetData());
    page->WSetDirty(true);
    buffer_manager_->UnpinPage(directory_page_id_, true);
    
    std::cout << "📂 Directorio duplicado: " << old_size << " → " << new_size 
              << " (Global Depth: " << global_depth_ << ")" << std::endl;
}

// =================================================================
// GESTIÓN DE CACHE Y RECURSOS
// =================================================================

template<typename K, typename V>
void ExtendibleHashDisk<K, V>::ClearCache() {
    bucket_cache_.clear();
}

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::Flush() {
    try {
        ClearCache();  // Forzar reload desde disco
        buffer_manager_->FlushAllPages();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error en Flush: " << e.what() << std::endl;
        return false;
    }
}

// =================================================================
// VERIFICACIONES Y LÍMITES
// =================================================================

template<typename K, typename V>
bool ExtendibleHashDisk<K, V>::NeedToDoubleDirectory(uint32_t local_depth) const {
    return local_depth >= global_depth_;
}

template<typename K, typename V>
uint32_t ExtendibleHashDisk<K, V>::GetTotalElements() const {
    // === CONCEPTO: Estadísticas agregadas ===
    uint32_t total = 0;
    std::unordered_set<page_id_t> visited_buckets;
    
    try {
        auto* directory_page = GetDirectoryPage();
        uint32_t dir_size = GetDirectorySize();
        
        for (uint32_t i = 0; i < dir_size; ++i) {
            page_id_t bucket_id = directory_page->GetBucketPageId(i);
            
            if (visited_buckets.find(bucket_id) == visited_buckets.end()) {
                auto* bucket = GetBucketPage(bucket_id);
                total += bucket->GetItemCount();
                visited_buckets.insert(bucket_id);
            }
        }
        
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error calculando elementos totales: " << e.what() << std::endl;
    }
    
    return total;
}

// =================================================================
// VISUALIZACIÓN CON LÍMITES
// =================================================================

template<typename K, typename V>
void ExtendibleHashDisk<K, V>::DisplayLimitedStructure(uint32_t max_entries) const {
    std::cout << "\n=== HASH EXTENSIBLE (LIMITADO A " << max_entries << ") ===" << std::endl;
    std::cout << "Global Depth: " << global_depth_ << "/" << MAX_GLOBAL_DEPTH << std::endl;
    std::cout << "Directory Size: " << GetDirectorySize() << "/" << MAX_DIRECTORY_SIZE << std::endl;
    std::cout << "Buckets únicos: " << GetNumberOfBuckets() << std::endl;
    std::cout << "Elementos totales: " << GetTotalElements() << std::endl;
    
    try {
        auto* directory_page = GetDirectoryPage();
        uint32_t dir_size = std::min(GetDirectorySize(), max_entries);
        
        for (uint32_t i = 0; i < dir_size; ++i) {
            page_id_t bucket_id = directory_page->GetBucketPageId(i);
            auto* bucket = GetBucketPage(bucket_id);
            
            std::cout << "Dir[" << std::setw(3) << i << "] → Bucket(ID:" 
                      << bucket_id << ", LD:" << bucket->GetLocalDepth() 
                      << ", " << bucket->GetItemCount() << "/" << bucket_capacity_ << ")";
            
            bucket->Display();
            std::cout << std::endl;
        }
        
        if (GetDirectorySize() > max_entries) {
            std::cout << "... y " << (GetDirectorySize() - max_entries) << " entradas más" << std::endl;
        }
        
        buffer_manager_->UnpinPage(directory_page_id_, false);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error en display: " << e.what() << std::endl;
    }
}

#endif // EXTENDIBLE_HASH_IMPL_H
