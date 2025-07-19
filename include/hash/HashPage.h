#ifndef HASH_PAGE_H
#define HASH_PAGE_H

#include "../PhysicalAddress.h"
#include "HashConfig.h"
#include <vector>
#include <cstring>

/**
 * @brief Estructura de página para Hash Extensible
 * 
 * CONCEPTOS APLICADOS:
 * - Page Layout: Organización fija de datos en página
 * - Serialización: Conversión objeto ↔ bytes
 * - Metadata: Información de control (header)
 */

// Constantes de página
static constexpr size_t HASH_PAGE_SIZE = 4096;  // 4KB páginas estándar
static constexpr size_t HASH_PAGE_HEADER_SIZE = 32;  // Metadata de página
static constexpr size_t HASH_PAGE_DATA_SIZE = HASH_PAGE_SIZE - HASH_PAGE_HEADER_SIZE;

/**
 * @brief Header de página Hash
 */
struct HashPageHeader {
    uint32_t page_type;        // Tipo: 1=Directory, 2=Bucket
    uint32_t page_size;        // Tamaño total de la página
    uint32_t item_count;       // Número de elementos almacenados
    uint32_t max_items;        // Capacidad máxima
    uint32_t local_depth;      // Local depth (solo para buckets)
    uint32_t global_depth;     // Global depth (solo para directory)
    uint32_t next_page_id;     // Encadenamiento (futuro)
    uint32_t reserved;         // Padding para alineación
};

/**
 * @brief Página base para Hash Extensible
 */
class HashPage {
protected:
    char data_[HASH_PAGE_SIZE];
    HashPageHeader* header_;
    
public:
    HashPage();
    virtual ~HashPage() = default;
    
    // Métodos de acceso al header
    uint32_t GetPageType() const { return header_->page_type; }
    uint32_t GetItemCount() const { return header_->item_count; }
    uint32_t GetMaxItems() const { return header_->max_items; }
    uint32_t GetLocalDepth() const { return header_->local_depth; }
    uint32_t GetGlobalDepth() const { return header_->global_depth; }
    
    void SetPageType(uint32_t type) { header_->page_type = type; }
    void SetItemCount(uint32_t count) { header_->item_count = count; }
    void SetMaxItems(uint32_t max) { header_->max_items = max; }
    void SetLocalDepth(uint32_t depth) { header_->local_depth = depth; }
    void SetGlobalDepth(uint32_t depth) { header_->global_depth = depth; }
    
    // Acceso a datos raw
    char* GetData() { return data_; }
    const char* GetData() const { return data_; }
    char* GetDataPtr() { return data_ + HASH_PAGE_HEADER_SIZE; }
    const char* GetDataPtr() const { return data_ + HASH_PAGE_HEADER_SIZE; }
    
    // Verificación de integridad
    bool IsValid() const;
    void InitPage(uint32_t page_type, uint32_t max_items);
};

/**
 * @brief Página para almacenar buckets
 */
template<typename K, typename V>
class HashBucketPage : public HashPage {
private:
    struct BucketEntry {
        K key;
        V value;
        bool is_deleted;  // Soft delete
        
        BucketEntry() : is_deleted(true) {}
        BucketEntry(const K& k, const V& v) : key(k), value(v), is_deleted(false) {}
    };
    
public:
    static constexpr uint32_t BUCKET_PAGE_TYPE = 2;
    
    HashBucketPage(uint32_t max_items = 0, uint32_t local_depth = 0);
    
    // Operaciones de bucket
    bool Insert(const K& key, const V& value);
    bool Remove(const K& key);
    bool Find(const K& key, V& value) const;
    bool IsFull() const;
    bool IsEmpty() const;
    
    // Gestión de splits
    std::vector<std::pair<K, V>> GetAllPairs() const;
    void Clear();
    
    // Serialización
    void SerializeTo(char* data) const;
    void DeserializeFrom(const char* data);
    
    // Debug
    void Display() const;
    
private:
    BucketEntry* GetEntries() { 
        return reinterpret_cast<BucketEntry*>(GetDataPtr()); 
    }
    const BucketEntry* GetEntries() const { 
        return reinterpret_cast<const BucketEntry*>(GetDataPtr()); 
    }
    
    size_t GetMaxEntries() const {
        return HASH_PAGE_DATA_SIZE / sizeof(BucketEntry);
    }
};

/**
 * @brief Página para almacenar el directorio
 */
class HashDirectoryPage : public HashPage {
private:
    using bucket_page_id_t = uint32_t;
    
public:
    static constexpr uint32_t DIRECTORY_PAGE_TYPE = 1;
    
    HashDirectoryPage(uint32_t global_depth = 1);
    
    // Operaciones de directorio
    bucket_page_id_t GetBucketPageId(uint32_t index) const;
    void SetBucketPageId(uint32_t index, bucket_page_id_t page_id);
    
    uint32_t GetDirectorySize() const;
    void DoubleDirectory();
    
    // Serialización
    void SerializeTo(char* data) const;
    void DeserializeFrom(const char* data);
    
    // Debug
    void Display() const;
    
private:
    bucket_page_id_t* GetDirectory() {
        return reinterpret_cast<bucket_page_id_t*>(GetDataPtr());
    }
    const bucket_page_id_t* GetDirectory() const {
        return reinterpret_cast<const bucket_page_id_t*>(GetDataPtr());
    }
    
    size_t GetMaxDirectorySize() const {
        return HASH_PAGE_DATA_SIZE / sizeof(bucket_page_id_t);
    }
};

// =================================================================
// IMPLEMENTACIONES
// =================================================================

inline HashPage::HashPage() {
    std::memset(data_, 0, HASH_PAGE_SIZE);
    header_ = reinterpret_cast<HashPageHeader*>(data_);
    header_->page_size = HASH_PAGE_SIZE;
}

inline bool HashPage::IsValid() const {
    return header_->page_size == HASH_PAGE_SIZE &&
           header_->item_count <= header_->max_items &&
           (header_->page_type == 1 || header_->page_type == 2);
}

inline void HashPage::InitPage(uint32_t page_type, uint32_t max_items) {
    header_->page_type = page_type;
    header_->max_items = max_items;
    header_->item_count = 0;
    header_->local_depth = 0;
    header_->global_depth = 0;
    header_->next_page_id = 0;
}

#endif // HASH_PAGE_H
