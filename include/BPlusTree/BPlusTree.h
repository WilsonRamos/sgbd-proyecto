#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include "../RecordReference.h"

/**
 * @brief Nodo base para B+ Tree
 */
template<typename KeyType>
class BPlusNode {
protected:
    std::vector<KeyType> keys;
    bool is_leaf;
    int order;

public:
    BPlusNode(int order, bool leaf) : is_leaf(leaf), order(order) {
        keys.reserve(order - 1);
    }
    
    virtual ~BPlusNode() = default;
    
    bool isLeaf() const { return is_leaf; }
    size_t getKeyCount() const { return keys.size(); }
    const std::vector<KeyType>& getKeys() const { return keys; }
    
    virtual bool isFull() const { return keys.size() >= order - 1; }
    virtual void display(int level = 0) const = 0;
};

/**
 * @brief Nodo hoja del B+ Tree
 */
template<typename KeyType>
class LeafNode : public BPlusNode<KeyType> {
private:
    std::vector<RecordReference> record_refs;
    std::shared_ptr<LeafNode<KeyType>> next; // Enlace a siguiente hoja

public:
    LeafNode(int order) : BPlusNode<KeyType>(order, true) {
        record_refs.reserve(order - 1);
    }
    
    bool insert(const KeyType& key, const RecordReference& record_ref) {
        if (this->isFull()) {
            return false;
        }
        
        // Encontrar posición de inserción
        auto it = std::lower_bound(this->keys.begin(), this->keys.end(), key);
        size_t pos = it - this->keys.begin();
        
        // Insertar clave y referencia
        this->keys.insert(it, key);
        record_refs.insert(record_refs.begin() + pos, record_ref);
        
        return true;
    }
    
    bool search(const KeyType& key, RecordReference& record_ref) const {
        auto it = std::lower_bound(this->keys.begin(), this->keys.end(), key);
        if (it != this->keys.end() && *it == key) {
            size_t pos = it - this->keys.begin();
            record_ref = record_refs[pos];
            return true;
        }
        return false;
    }
    
    std::vector<RecordReference> rangeSearch(const KeyType& start, const KeyType& end) const {
        std::vector<RecordReference> results;
        
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (this->keys[i] >= start && this->keys[i] <= end) {
                results.push_back(record_refs[i]);
            }
        }
        
        return results;
    }
    
    void setNext(std::shared_ptr<LeafNode<KeyType>> next_node) {
        next = next_node;
    }
    
    std::shared_ptr<LeafNode<KeyType>> getNext() const {
        return next;
    }
    
    const std::vector<RecordReference>& getRecordRefs() const {
        return record_refs;
    }
    
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "LeafNode: ";
        for (size_t i = 0; i < this->keys.size(); i++) {
            std::cout << this->keys[i];
            if (i < this->keys.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
};

/**
 * @brief Nodo interno del B+ Tree
 */
template<typename KeyType>
class InternalNode : public BPlusNode<KeyType> {
private:
    std::vector<std::shared_ptr<BPlusNode<KeyType>>> children;

public:
    InternalNode(int order) : BPlusNode<KeyType>(order, false) {
        children.reserve(order);
    }
    
    void addChild(std::shared_ptr<BPlusNode<KeyType>> child) {
        children.push_back(child);
    }
    
    std::shared_ptr<BPlusNode<KeyType>> getChild(size_t index) const {
        if (index < children.size()) {
            return children[index];
        }
        return nullptr;
    }
    
    size_t getChildCount() const {
        return children.size();
    }
    
    std::shared_ptr<BPlusNode<KeyType>> findChild(const KeyType& key) const {
        size_t i = 0;
        while (i < this->keys.size() && key >= this->keys[i]) {
            i++;
        }
        return getChild(i);
    }
    
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "InternalNode: ";
        for (size_t i = 0; i < this->keys.size(); i++) {
            std::cout << this->keys[i];
            if (i < this->keys.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        for (const auto& child : children) {
            if (child) {
                child->display(level + 1);
            }
        }
    }
};

/**
 * @brief B+ Tree implementación educativa
 */
template<typename KeyType>
class BPlusTree {
private:
    std::shared_ptr<BPlusNode<KeyType>> root;
    int order;
    size_t total_keys;
    size_t search_operations;
    size_t insert_operations;
    
    std::shared_ptr<LeafNode<KeyType>> first_leaf; // Para range queries

public:
    /**
     * @brief Constructor
     */
    BPlusTree(int tree_order = 4) 
        : order(tree_order), total_keys(0), search_operations(0), insert_operations(0) {
        
        // Crear nodo raíz como hoja inicial
        auto leaf = std::make_shared<LeafNode<KeyType>>(order);
        root = leaf;
        first_leaf = leaf;
        
        std::cout << "🌳 B+ Tree inicializado (orden: " << order << ")" << std::endl;
    }
    
    // ============================================================================
    // OPERACIONES BÁSICAS
    // ============================================================================
    
    /**
     * @brief Insertar clave con referencia a registro
     */
    bool insert(const KeyType& key, const RecordReference& record_ref) {
        insert_operations++;
        
        // Si la raíz es una hoja y no está llena
        if (root->isLeaf()) {
            auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
            if (leaf->insert(key, record_ref)) {
                total_keys++;
                return true;
            }
        }
        
        // Implementación simplificada para educación
        // En una implementación completa, aquí iría la lógica de split
        std::cout << "⚠️ Inserción en árbol complejo no implementada (educativo)" << std::endl;
        return false;
    }
    
    /**
     * @brief Buscar clave
     */
    bool search(const KeyType& key, RecordReference& record_ref) {
        search_operations++;
        
        if (!root) {
            return false;
        }
        
        // Navegación simplificada
        auto current = root;
        while (!current->isLeaf()) {
            auto internal = std::static_pointer_cast<InternalNode<KeyType>>(current);
            current = internal->findChild(key);
            if (!current) {
                return false;
            }
        }
        
        // Buscar en hoja
        auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(current);
        return leaf->search(key, record_ref);
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Búsqueda por rango
     */
    std::vector<KeyType> rangeSearch(const KeyType& start, const KeyType& end) {
        std::vector<KeyType> results;
        
        if (!root || !root->isLeaf()) {
            return results;
        }
        
        // Implementación simplificada para hoja única
        auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
        const auto& keys = leaf->getKeys();
        
        for (const auto& key : keys) {
            if (key >= start && key <= end) {
                results.push_back(key);
            }
        }
        
        std::cout << "🔍 Range search [" << start << ", " << end << "] encontró " 
                  << results.size() << " resultados" << std::endl;
        
        return results;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Búsqueda con acceso a disco
     */
    bool searchWithDiskAccess(const KeyType& key, RecordReference& record_ref,
                              std::function<bool(const RecordReference&)> disk_loader = nullptr) {
        
        bool found = search(key, record_ref);
        
        if (found && disk_loader) {
            std::cout << "🔍 B+ Tree: Acceso a disco para clave " << key << std::endl;
            std::cout << "   Page ID: " << record_ref.getPhysicalAddress().toString() << std::endl;
            
            bool loaded = disk_loader(record_ref);
            std::cout << "📀 Carga desde disco: " << (loaded ? "✅ Exitosa" : "❌ Falló") << std::endl;
        }
        
        return found;
    }
    
    // ============================================================================
    // ✅ FUNCIONES AGREGADAS - ESTADÍSTICAS E INFORMACIÓN
    // ============================================================================
    
    /**
     * @brief Obtiene número total de claves
     */
    size_t size() const {
        return total_keys;
    }
    
    /**
     * @brief Obtiene orden del árbol
     */
    int getOrder() const {
        return order;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Calcula altura del árbol
     */
    int getHeight() const {
        if (!root) {
            return 0;
        }
        
        int height = 1;
        auto current = root;
        
        while (!current->isLeaf()) {
            auto internal = std::static_pointer_cast<InternalNode<KeyType>>(current);
            current = internal->getChild(0);
            if (current) {
                height++;
            } else {
                break;
            }
        }
        
        return height;
    }
    
    /**
     * @brief ✅ FUNCIÓN AGREGADA - Obtiene estadísticas completas
     */
    std::string getStatistics() const {
        std::ostringstream ss;
        
        ss << "=== ESTADÍSTICAS B+ TREE ===\n";
        ss << "Orden: " << order << "\n";
        ss << "Total de claves: " << total_keys << "\n";
        ss << "Altura: " << getHeight() << "\n";
        ss << "Operaciones de búsqueda: " << search_operations << "\n";
        ss << "Operaciones de inserción: " << insert_operations << "\n";
        
        if (search_operations > 0) {
            double avg_search = (double)search_operations / std::max(size_t(1), total_keys);
            ss << "Promedio búsquedas por clave: " << std::fixed << std::setprecision(2) << avg_search << "\n";
        }
        
        // Estadísticas de la raíz
        if (root) {
            ss << "Claves en raíz: " << root->getKeyCount() << "\n";
            ss << "Tipo de raíz: " << (root->isLeaf() ? "Hoja" : "Interno") << "\n";
            
            if (root->isLeaf()) {
                auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
                double fill_factor = (double)leaf->getKeyCount() / (order - 1);
                ss << "Factor de llenado raíz: " << std::fixed << std::setprecision(2) << (fill_factor * 100) << "%\n";
            }
        }
        
        return ss.str();
    }
    
    /**
     * @brief Muestra estadísticas
     */
    void displayStatistics() const {
        std::cout << getStatistics() << std::endl;
    }
    
    /**
     * @brief Muestra estructura del árbol
     */
    void display() const {
        std::cout << "\n🌳 ESTRUCTURA DEL B+ TREE:" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;
        
        if (root) {
            root->display();
        } else {
            std::cout << "Árbol vacío" << std::endl;
        }
        
        std::cout << "\n📊 " << getStatistics() << std::endl;
    }
    
    // ============================================================================
    // ✅ FUNCIONES AGREGADAS - ANÁLISIS Y NAVEGACIÓN
    // ============================================================================
    
    /**
     * @brief Obtiene todas las claves en orden
     */
    std::vector<KeyType> getAllKeys() const {
        std::vector<KeyType> keys;
        
        if (root && root->isLeaf()) {
            auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
            keys = leaf->getKeys();
        }
        
        return keys;
    }
    
    /**
     * @brief Verifica si el árbol contiene una clave
     */
    bool contains(const KeyType& key) const {
        RecordReference dummy;
        return const_cast<BPlusTree*>(this)->search(key, dummy);
    }
    
    /**
     * @brief Obtiene el número de nodos en el árbol
     */
    size_t getNodeCount() const {
        // Implementación simplificada
        return 1; // Solo raíz en esta implementación educativa
    }
    
    /**
     * @brief Obtiene información de distribución de claves
     */
    std::string getKeyDistribution() const {
        std::ostringstream ss;
        
        if (root && root->isLeaf()) {
            auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
            const auto& keys = leaf->getKeys();
            
            ss << "Distribución de claves:\n";
            ss << "  Total: " << keys.size() << "\n";
            
            if (!keys.empty()) {
                ss << "  Mínima: " << keys.front() << "\n";
                ss << "  Máxima: " << keys.back() << "\n";
                
                if (keys.size() > 1) {
                    ss << "  Rango: [" << keys.front() << " - " << keys.back() << "]\n";
                }
            }
        }
        
        return ss.str();
    }
    
    /**
     * @brief Valida la integridad del árbol
     */
    bool validate() const {
        if (!root) {
            return true;
        }
        
        // Verificaciones básicas
        if (root->isLeaf()) {
            auto leaf = std::static_pointer_cast<LeafNode<KeyType>>(root);
            const auto& keys = leaf->getKeys();
            
            // Verificar que las claves están ordenadas
            for (size_t i = 1; i < keys.size(); i++) {
                if (keys[i-1] >= keys[i]) {
                    std::cout << "❌ Claves no ordenadas en posición " << i << std::endl;
                    return false;
                }
            }
        }
        
        std::cout << "✅ Estructura del B+ Tree validada" << std::endl;
        return true;
    }
    
    // ============================================================================
    // GETTERS ADICIONALES
    // ============================================================================
    
    bool isEmpty() const { return total_keys == 0; }
    size_t getSearchOperations() const { return search_operations; }
    size_t getInsertOperations() const { return insert_operations; }
    
    /**
     * @brief Obtiene referencia a la primera hoja (para iteración)
     */
    std::shared_ptr<LeafNode<KeyType>> getFirstLeaf() const {
        return first_leaf;
    }
};

#endif // BPLUS_TREE_H