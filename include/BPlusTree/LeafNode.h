#ifndef LEAF_NODE_H
#define LEAF_NODE_H

#include "BPlusNode.h"
#include "../RecordReference.h"
#include <iostream>

/**
 * @brief Nodo hoja del B+ Tree
 * 
 * Características especiales:
 * - Almacena RecordReference junto con claves
 * - Enlaces horizontales a nodos hermanos
 * - Soporte para búsquedas por rango
 * - Todas las claves están en las hojas
 */
template<typename KeyType>
class LeafNode : public BPlusNode<KeyType> {
private:
    std::vector<RecordReference> record_refs; // Referencias a registros en disco
    LeafNode* next;                          // Puntero al siguiente nodo hoja
    LeafNode* prev;                          // Puntero al nodo hoja anterior

public:
    /**
     * @brief Constructor
     */
    LeafNode(int order) : BPlusNode<KeyType>(order, true), next(nullptr), prev(nullptr) {
        record_refs.reserve(order - 1);
    }
    
    /**
     * @brief Destructor
     */
    ~LeafNode() override = default;

    // ============================================================================
    // IMPLEMENTACIÓN DE MÉTODOS VIRTUALES
    // ============================================================================
    
    /**
     * @brief Identifica que es un nodo hoja
     */
    bool isLeaf() const override { 
        return true; 
    }
    
    /**
     * @brief Inserta una clave con su RecordReference
     */
    bool insert(const KeyType& key, const RecordReference& record_ref) override {
        if (this->isFull()) {
            return false; // El nodo está lleno
        }
        
        int pos = this->findInsertPosition(key);
        
        // Verificar si la clave ya existe
        if (pos < static_cast<int>(this->keys.size()) && 
            KeyComparator<KeyType>::compare(this->keys[pos], key) == 0) {
            // Actualizar referencia existente
            record_refs[pos] = record_ref;
            return true;
        }
        
        // Insertar nueva clave y referencia
        this->keys.insert(this->keys.begin() + pos, key);
        record_refs.insert(record_refs.begin() + pos, record_ref);
        
        return true;
    }
    
    /**
     * @brief Busca una clave y retorna su RecordReference
     */
    bool search(const KeyType& key, RecordReference& record_ref) override {
        int index = this->findKey(key);
        if (index != -1) {
            record_ref = record_refs[index];
            return true;
        }
        return false;
    }
    
    /**
     * @brief Elimina una clave del nodo hoja
     */
    bool remove(const KeyType& key) override {
        int index = this->findKey(key);
        if (index != -1) {
            this->keys.erase(this->keys.begin() + index);
            record_refs.erase(record_refs.begin() + index);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Divide el nodo hoja cuando está lleno
     */
    BPlusNode<KeyType>* split() override {
        if (!this->isFull()) {
            return nullptr;
        }
        
        int mid = this->order / 2;
        auto new_leaf = new LeafNode<KeyType>(this->order);
        
        // Mover la mitad derecha al nuevo nodo
        new_leaf->keys.assign(this->keys.begin() + mid, this->keys.end());
        new_leaf->record_refs.assign(record_refs.begin() + mid, record_refs.end());
        
        // Mantener la mitad izquierda
        this->keys.resize(mid);
        record_refs.resize(mid);
        
        // Actualizar enlaces horizontales
        new_leaf->next = this->next;
        new_leaf->prev = this;
        
        if (this->next) {
            this->next->prev = new_leaf;
        }
        this->next = new_leaf;
        
        // Establecer padre
        new_leaf->setParent(this->getParent());
        
        std::cout << "🌿 Nodo hoja dividido: " << this->keys.size() 
                  << " + " << new_leaf->keys.size() << " claves" << std::endl;
        
        return new_leaf;
    }

    // ============================================================================
    // GESTIÓN DE ENLACES HORIZONTALES
    // ============================================================================
    
    /**
     * @brief Obtiene el siguiente nodo hoja
     */
    LeafNode* getNext() const { 
        return next; 
    }
    
    /**
     * @brief Obtiene el nodo hoja anterior
     */
    LeafNode* getPrev() const { 
        return prev; 
    }
    
    /**
     * @brief Establece el siguiente nodo hoja
     */
    void setNext(LeafNode* n) { 
        next = n; 
    }
    
    /**
     * @brief Establece el nodo hoja anterior
     */
    void setPrev(LeafNode* p) { 
        prev = p; 
    }

    // ============================================================================
    // BÚSQUEDAS POR RANGO
    // ============================================================================
    
    /**
     * @brief Búsqueda por rango en el nodo hoja
     */
    void rangeSearch(const KeyType& start_key, const KeyType& end_key,
                    std::vector<RecordReference>& results, int& found_count) override {
        
        found_count = 0;
        
        // Encontrar la primera clave en el rango
        int start_index = this->findFirstGreaterOrEqual(start_key);
        
        // Recorrer las claves en el rango
        for (int i = start_index; i < static_cast<int>(this->keys.size()); i++) {
            if (KeyComparator<KeyType>::compare(this->keys[i], end_key) > 0) {
                break; // Fuera del rango
            }
            
            results.push_back(record_refs[i]);
            found_count++;
        }
    }
    
    /**
     * @brief Búsqueda por rango simplificada (solo conteo)
     */
    int countInRange(const KeyType& start_key, const KeyType& end_key) const {
        int count = 0;
        int start_index = this->findFirstGreaterOrEqual(start_key);
        
        for (int i = start_index; i < static_cast<int>(this->keys.size()); i++) {
            if (KeyComparator<KeyType>::compare(this->keys[i], end_key) > 0) {
                break;
            }
            count++;
        }
        
        return count;
    }
    
    /**
     * @brief Obtiene todas las claves en un rango específico
     */
    std::vector<KeyType> getKeysInRange(const KeyType& start_key, const KeyType& end_key) const {
        std::vector<KeyType> range_keys;
        int start_index = this->findFirstGreaterOrEqual(start_key);
        
        for (int i = start_index; i < static_cast<int>(this->keys.size()); i++) {
            if (KeyComparator<KeyType>::compare(this->keys[i], end_key) > 0) {
                break;
            }
            range_keys.push_back(this->keys[i]);
        }
        
        return range_keys;
    }

    // ============================================================================
    // ACCESO A RECORD REFERENCES
    // ============================================================================
    
    /**
     * @brief Obtiene todas las referencias de registros
     */
    const std::vector<RecordReference>& getRecordReferences() const {
        return record_refs;
    }
    
    /**
     * @brief Obtiene una referencia específica por índice
     */
    const RecordReference& getRecordReference(size_t index) const {
        if (index < record_refs.size()) {
            return record_refs[index];
        }
        throw std::out_of_range("Índice de RecordReference fuera de rango");
    }
    
    /**
     * @brief Establece una referencia en una posición específica
     */
    void setRecordReference(size_t index, const RecordReference& ref) {
        if (index < record_refs.size()) {
            record_refs[index] = ref;
        } else {
            throw std::out_of_range("Índice de RecordReference fuera de rango");
        }
    }

    // ============================================================================
    // VALIDACIÓN ESPECÍFICA DE NODOS HOJA
    // ============================================================================
    
    /**
     * @brief Valida la integridad del nodo hoja
     */
    bool validateNode() const override {
        // Validación de la clase base
        if (!BPlusNode<KeyType>::validateNode()) {
            return false;
        }
        
        // Verificar que el número de claves coincida con el de referencias
        if (this->keys.size() != record_refs.size()) {
            std::cout << "❌ Error: Número de claves (" << this->keys.size() 
                      << ") no coincide con referencias (" << record_refs.size() << ")" << std::endl;
            return false;
        }
        
        // Verificar enlaces horizontales
        if (next && next->prev != this) {
            std::cout << "❌ Error: Enlace horizontal inconsistente (next->prev)" << std::endl;
            return false;
        }
        
        if (prev && prev->next != this) {
            std::cout << "❌ Error: Enlace horizontal inconsistente (prev->next)" << std::endl;
            return false;
        }
        
        // Verificar que todas las referencias sean válidas
        for (const auto& ref : record_refs) {
            if (!ref.isValid()) {
                std::cout << "⚠️ Warning: RecordReference inválida encontrada" << std::endl;
            }
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra información detallada del nodo hoja
     */
    void displayInfo() const override {
        std::cout << "🌿 NODO HOJA:" << std::endl;
        std::cout << "  Orden: " << this->order << std::endl;
        std::cout << "  Claves: " << this->keys.size() << "/" << this->getMaxKeys() << std::endl;
        std::cout << "  Referencias: " << record_refs.size() << std::endl;
        std::cout << "  Ocupación: " << std::fixed << std::setprecision(1) 
                  << this->getOccupancyFactor() * 100 << "%" << std::endl;
        
        // Mostrar claves y referencias
        if (!this->keys.empty()) {
            std::cout << "  Contenido:" << std::endl;
            for (size_t i = 0; i < this->keys.size() && i < 5; i++) {
                std::cout << "    [" << i << "] " << this->keys[i] 
                          << " → " << record_refs[i] << std::endl;
            }
            
            if (this->keys.size() > 5) {
                std::cout << "    ... (" << (this->keys.size() - 5) << " más)" << std::endl;
            }
        }
        
        // Información de enlaces
        std::cout << "  Enlaces: ";
        std::cout << "Prev=" << (prev ? "Sí" : "No") << ", ";
        std::cout << "Next=" << (next ? "Sí" : "No") << std::endl;
        
        // Estado
        std::cout << "  Estado: " << (this->isFull() ? "LLENO" : 
                                    (this->isEmpty() ? "VACÍO" : "PARCIAL")) << std::endl;
    }
    
    /**
     * @brief Representación en string del nodo hoja
     */
    std::string toString() const override {
        std::stringstream ss;
        ss << "LEAF[";
        for (size_t i = 0; i < this->keys.size(); i++) {
            ss << this->keys[i];
            if (i < this->keys.size() - 1) ss << ",";
        }
        ss << "](" << this->keys.size() << "/" << this->getMaxKeys() << ")";
        return ss.str();
    }
    
    /**
     * @brief Muestra la cadena de nodos hoja (para debug)
     */
    void displayLeafChain() const {
        std::cout << "🔗 CADENA DE NODOS HOJA:" << std::endl;
        
        const LeafNode* current = this;
        
        // Ir al primer nodo
        while (current->prev) {
            current = current->prev;
        }
        
        int node_count = 0;
        while (current && node_count < 10) { // Limitar a 10 nodos para evitar spam
            std::cout << "  Nodo " << node_count << ": " << current->toString() << std::endl;
            current = current->next;
            node_count++;
        }
        
        if (current) {
            std::cout << "  ... (más nodos)" << std::endl;
        }
    }

    // ============================================================================
    // ESTADÍSTICAS ESPECÍFICAS DE HOJAS
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas del nodo hoja
     */
    struct LeafStats {
        size_t key_count;
        size_t ref_count;
        bool has_next;
        bool has_prev;
        double occupancy;
        KeyType min_key;
        KeyType max_key;
        bool valid_refs;
    };
    
    LeafStats getLeafStats() const {
        LeafStats stats;
        stats.key_count = this->keys.size();
        stats.ref_count = record_refs.size();
        stats.has_next = (next != nullptr);
        stats.has_prev = (prev != nullptr);
        stats.occupancy = this->getOccupancyFactor();
        
        if (!this->keys.empty()) {
            stats.min_key = this->keys.front();
            stats.max_key = this->keys.back();
        }
        
        // Verificar validez de referencias
        stats.valid_refs = true;
        for (const auto& ref : record_refs) {
            if (!ref.isValid()) {
                stats.valid_refs = false;
                break;
            }
        }
        
        return stats;
    }
    
    /**
     * @brief Calcula la densidad de datos en el nodo
     */
    double getDataDensity() const {
        if (this->getMaxKeys() == 0) return 0.0;
        return (double)this->keys.size() / this->getMaxKeys();
    }
    
    /**
     * @brief Predice si necesitará división pronto
     */
    bool needsSplitSoon() const {
        return getDataDensity() > 0.85; // Más del 85% lleno
    }
};

#endif // LEAF_NODE_H