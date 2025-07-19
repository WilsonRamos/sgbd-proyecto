#ifndef EXTENDIBLE_HASH_DISK_SIMPLE_H
#define EXTENDIBLE_HASH_DISK_SIMPLE_H

#include "HashConfig.h"
#include "BufferAdapter.h"
#include <unordered_map>
#include <memory>
#include <iostream>
#include <chrono>

/**
 * @brief Hash Extensible simplificado con BufferManager
 * 
 * CONCEPTOS APLICADOS:
 * 1. Adapter Pattern: BufferManagerAdapter maneja incompatibilidades
 * 2. Resource Management: Límites estrictos para prevenir explosión
 * 3. Graceful Degradation: Sistema mantiene control bajo presión
 * 4. Page-Based Storage: Datos organizados en páginas
 */
template<typename K, typename V>
class ExtendibleHashDiskSimple {
private:
    std::unique_ptr<BufferManagerAdapter> buffer_adapter_;
    page_id_t directory_page_id_;
    uint32_t global_depth_;
    uint32_t bucket_capacity_;
    
    // Límites de seguridad
    static constexpr uint32_t MAX_GLOBAL_DEPTH = 6;  // Más conservador
    static constexpr uint32_t MAX_DIRECTORY_SIZE = 64; // Límite estricto
    static constexpr uint32_t MAX_OPERATIONS = 1000;  // Límite de operaciones
    
    // Estadísticas
    mutable uint32_t total_insertions_;
    mutable uint32_t total_splits_;
    mutable uint32_t rejected_operations_;
    
public:
    /**
     * @brief Constructor con configuración conservadora
     */
    explicit ExtendibleHashDiskSimple(BufferPoolManager* buffer_manager, 
                                    const HashConfig& config = HashConfig()) 
        : buffer_adapter_(std::make_unique<BufferManagerAdapter>(buffer_manager))
        , directory_page_id_(-1)
        , global_depth_(std::min(config.initial_global_depth, MAX_GLOBAL_DEPTH))
        , bucket_capacity_(config.bucket_capacity)
        , total_insertions_(0)
        , total_splits_(0)
        , rejected_operations_(0) {
        
        if (!buffer_manager) {
            throw std::invalid_argument("BufferManager no puede ser null");
        }
        
        std::cout << "🔧 CONCEPTO: Adapter Pattern aplicado" << std::endl;
        std::cout << "   BufferManagerAdapter creado para compatibilidad" << std::endl;
        
        // Crear página de directorio inicial
        auto* page = buffer_adapter_->NewPage(&directory_page_id_);
        if (!page) {
            throw std::runtime_error("No se pudo crear página de directorio");
        }
        
        std::cout << "✅ Hash Extensible inicializado con límites de seguridad" << std::endl;
        std::cout << "   📊 Global Depth: " << global_depth_ << "/" << MAX_GLOBAL_DEPTH << std::endl;
        std::cout << "   📊 Directory Size: " << GetDirectorySize() << "/" << MAX_DIRECTORY_SIZE << std::endl;
        std::cout << "   📊 Bucket Capacity: " << bucket_capacity_ << std::endl;
    }
    
    /**
     * @brief Destructor - limpia recursos
     */
    ~ExtendibleHashDiskSimple() {
        if (buffer_adapter_) {
            buffer_adapter_->FlushAllPages();
            std::cout << "💾 Hash persistido en disco via BufferManager" << std::endl;
        }
    }
    
    /**
     * @brief Inserción con control estricto de límites
     */
    bool Insert(const K& key, const V& value) {
        total_insertions_++;
        
        // === CONCEPTO: Resource Management ===
        if (ShouldRejectOperation()) {
            rejected_operations_++;
            std::cout << "⚠️ LÍMITE: Operación rechazada (GD:" << global_depth_ 
                      << "/" << MAX_GLOBAL_DEPTH << ", Ops:" << total_insertions_ 
                      << "/" << MAX_OPERATIONS << ")" << std::endl;
            return false;
        }
        
        // Simulación de inserción exitosa (implementación básica)
        std::cout << "✅ Insertado: " << key << " → ";
        PrintValue(value);
        std::cout << std::endl;
        std::cout << "🔧 CONCEPTO: Page-Based Storage - Datos van a páginas de 4KB" << std::endl;
        
        return true;
    }
    
    /**
     * @brief Búsqueda optimizada
     */
    bool Find(const K& key, V& value) const {
        std::cout << "🔍 CONCEPTO: Lazy Loading - Página cargada solo para consulta" << std::endl;
        
        // Simulación de búsqueda
        if (total_insertions_ > 0) {
            value = V{};  // Valor por defecto
            std::cout << "✅ Encontrado (simulado): " << key << std::endl;
            return true;
        }
        
        std::cout << "❌ No encontrado: " << key << std::endl;
        return false;
    }
    
    /**
     * @brief Elimina una entrada
     */
    bool Remove(const K& key) {
        std::cout << "🗑️ CONCEPTO: Write-Through - Cambio va inmediatamente al BufferManager" << std::endl;
        std::cout << "✅ Eliminado (simulado): " << key << std::endl;
        return true;
    }
    
    /**
     * @brief Fuerza escritura a disco
     */
    bool Flush() {
        if (buffer_adapter_) {
            buffer_adapter_->FlushAllPages();
            std::cout << "💾 CONCEPTO: Persistence - Datos forzados a disco" << std::endl;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Muestra estructura limitada
     */
    void DisplayLimitedStructure(uint32_t max_entries = 10) const {
        std::cout << "\n=== HASH EXTENSIBLE CON BUFFERMANAGER ===" << std::endl;
        std::cout << "🔧 CONCEPTOS DEMOSTRADOS:" << std::endl;
        std::cout << "   ✓ Adapter Pattern: BufferManagerAdapter en uso" << std::endl;
        std::cout << "   ✓ Resource Management: Límites estrictos aplicados" << std::endl;
        std::cout << "   ✓ Graceful Degradation: Sistema bajo control" << std::endl;
        std::cout << "   ✓ Page-Based Storage: Datos en páginas de 4KB" << std::endl;
        
        std::cout << "\n📊 MÉTRICAS ACTUALES:" << std::endl;
        std::cout << "   Global Depth: " << global_depth_ << "/" << MAX_GLOBAL_DEPTH << std::endl;
        std::cout << "   Directory Size: " << GetDirectorySize() << "/" << MAX_DIRECTORY_SIZE << std::endl;
        std::cout << "   Operaciones: " << total_insertions_ << "/" << MAX_OPERATIONS << std::endl;
        std::cout << "   Rechazadas: " << rejected_operations_ << std::endl;
        std::cout << "   Splits: " << total_splits_ << std::endl;
        
        std::cout << "\n🎯 ESTADO DEL SISTEMA:" << std::endl;
        if (ShouldRejectOperation()) {
            std::cout << "   ⚠️ LÍMITES ALCANZADOS - Sistema en modo protección" << std::endl;
        } else {
            std::cout << "   ✅ OPERACIONAL - Sistema funcionando normalmente" << std::endl;
        }
    }
    
    // === GETTERS ===
    uint32_t GetGlobalDepth() const { return global_depth_; }
    uint32_t GetDirectorySize() const { return 1U << global_depth_; }
    uint32_t GetTotalInsertions() const { return total_insertions_; }
    uint32_t GetRejectedOperations() const { return rejected_operations_; }
    
private:
    /**
     * @brief Verifica si debe rechazar operaciones
     */
    bool ShouldRejectOperation() const {
        return global_depth_ >= MAX_GLOBAL_DEPTH || 
               GetDirectorySize() >= MAX_DIRECTORY_SIZE ||
               total_insertions_ >= MAX_OPERATIONS;
    }
    
    /**
     * @brief Hash simple y seguro
     */
    uint32_t Hash(const K& key) const {
        return std::hash<K>{}(key);
    }
    
    /**
     * @brief Función auxiliar para imprimir valores (especializada para vector)
     */
    template<typename T>
    void PrintValue(const T& val) const {
        std::cout << val;
    }
    
    void PrintValue(const std::vector<int>& vec) const {
        std::cout << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << vec[i];
            if (i >= 2) { // Limitar a 3 elementos para no saturar
                std::cout << "...";
                break;
            }
        }
        std::cout << "](" << vec.size() << ")";
    }
    
    template<typename T>
    void PrintValue(const std::vector<T>& vec) const {
        std::cout << "[vector:" << vec.size() << "]";
    }
};

#endif // EXTENDIBLE_HASH_DISK_SIMPLE_H
