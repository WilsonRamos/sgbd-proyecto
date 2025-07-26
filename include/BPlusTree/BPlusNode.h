#ifndef BPLUS_NODE_H
#define BPLUS_NODE_H

#include <vector>
#include <memory>
#include <iostream>
#include "../RecordReference.h"
#include "KeyComparator.h"

/**
 * @brief Clase base abstracta para nodos del B+ Tree
 * 
 * Define la interfaz común para nodos internos y hojas:
 * - Gestión de claves
 * - Operaciones básicas (insertar, buscar, eliminar)
 * - Información de estado del nodo
 * - Métodos virtuales puros para especialización
 */
template<typename KeyType>
class BPlusNode {
protected:
    std::vector<KeyType> keys;               // Claves almacenadas en el nodo
    int order;                               // Orden del árbol (max claves = order-1)
    bool is_leaf_node;                       // Flag para identificar tipo de nodo
    BPlusNode* parent;                       // Puntero al nodo padre

public:
    /**
     * @brief Constructor
     */
    BPlusNode(int tree_order, bool leaf = false) 
        : order(tree_order), is_leaf_node(leaf), parent(nullptr) {
        keys.reserve(order - 1);
    }
    
    /**
     * @brief Destructor virtual
     */
    virtual ~BPlusNode() = default;

    // ============================================================================
    // MÉTODOS VIRTUALES PUROS (DEBEN SER IMPLEMENTADOS POR SUBCLASES)
    // ============================================================================
    
    /**
     * @brief Inserta una clave con su referencia
     */
    virtual bool insert(const KeyType& key, const RecordReference& record_ref) = 0;
    
    /**
     * @brief Busca una clave y retorna su referencia
     */
    virtual bool search(const KeyType& key, RecordReference& record_ref) = 0;
    
    /**
     * @brief Elimina una clave del nodo
     */
    virtual bool remove(const KeyType& key) = 0;
    
    /**
     * @brief Verifica si es un nodo hoja
     */
    virtual bool isLeaf() const = 0;
    
    /**
     * @brief Divide el nodo cuando está lleno
     */
    virtual BPlusNode* split() = 0;

    // ============================================================================
    // MÉTODOS COMUNES PARA GESTIÓN DE CLAVES
    // ============================================================================
    
    /**
     * @brief Verifica si el nodo está lleno
     */
    bool isFull() const {
        return keys.size() >= (order - 1);
    }
    
    /**
     * @brief Verifica si el nodo está vacío
     */
    bool isEmpty() const {
        return keys.empty();
    }
    
    /**
     * @brief Obtiene el número de claves
     */
    size_t getKeyCount() const {
        return keys.size();
    }
    
    /**
     * @brief Obtiene la capacidad máxima de claves
     */
    size_t getMaxKeys() const {
        return order - 1;
    }
    
    /**
     * @brief Obtiene el orden del árbol
     */
    int getOrder() const {
        return order;
    }

    // ============================================================================
    // ACCESO A CLAVES
    // ============================================================================
    
    /**
     * @brief Obtiene las claves del nodo (solo lectura)
     */
    const std::vector<KeyType>& getKeys() const {
        return keys;
    }
    
    /**
     * @brief Obtiene las claves del nodo (modificable)
     */
    std::vector<KeyType>& getKeys() {
        return keys;
    }
    
    /**
     * @brief Obtiene una clave específica por índice
     */
    const KeyType& getKey(size_t index) const {
        if (index < keys.size()) {
            return keys[index];
        }
        throw std::out_of_range("Índice de clave fuera de rango");
    }
    
    /**
     * @brief Establece una clave en una posición específica
     */
    void setKey(size_t index, const KeyType& key) {
        if (index < keys.size()) {
            keys[index] = key;
        } else if (index == keys.size()) {
            keys.push_back(key);
        } else {
            throw std::out_of_range("Índice de clave fuera de rango");
        }
    }

    // ============================================================================
    // BÚSQUEDA DE POSICIONES
    // ============================================================================
    
    /**
     * @brief Encuentra la posición donde insertar una clave
     */
    int findInsertPosition(const KeyType& key) const {
        int pos = 0;
        while (pos < static_cast<int>(keys.size()) && 
               KeyComparator<KeyType>::compare(keys[pos], key) < 0) {
            pos++;
        }
        return pos;
    }
    
    /**
     * @brief Busca una clave específica y retorna su índice
     */
    int findKey(const KeyType& key) const {
        for (size_t i = 0; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(keys[i], key) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1; // No encontrada
    }
    
    /**
     * @brief Busca la primera clave mayor o igual a la dada
     */
    int findFirstGreaterOrEqual(const KeyType& key) const {
        for (size_t i = 0; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(keys[i], key) >= 0) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(keys.size()); // Todas las claves son menores
    }

    // ============================================================================
    // GESTIÓN DE PADRE/HIJO
    // ============================================================================
    
    /**
     * @brief Obtiene el nodo padre
     */
    BPlusNode* getParent() const {
        return parent;
    }
    
    /**
     * @brief Establece el nodo padre
     */
    void setParent(BPlusNode* new_parent) {
        parent = new_parent;
    }
    
    /**
     * @brief Verifica si es la raíz
     */
    bool isRoot() const {
        return parent == nullptr;
    }

    // ============================================================================
    // UTILIDADES DE VALIDACIÓN
    // ============================================================================
    
    /**
     * @brief Valida que las claves estén ordenadas
     */
    bool validateKeyOrder() const {
        for (size_t i = 1; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(keys[i-1], keys[i]) >= 0) {
                std::cout << "❌ Error: Claves no están ordenadas en posición " << i << std::endl;
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Valida el estado general del nodo
     */
    virtual bool validateNode() const {
        // Verificar orden de claves
        if (!validateKeyOrder()) {
            return false;
        }
        
        // Verificar que no exceda la capacidad
        if (keys.size() > getMaxKeys()) {
            std::cout << "❌ Error: Nodo excede capacidad máxima" << std::endl;
            return false;
        }
        
        // Para nodos no-raíz, verificar que tengan al menos order/2 claves
        if (!isRoot() && keys.size() < (order / 2)) {
            std::cout << "⚠️ Warning: Nodo tiene menos claves del mínimo requerido" << std::endl;
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra información básica del nodo
     */
    virtual void displayInfo() const {
        std::cout << (isLeaf() ? "LEAF" : "INTERNAL") << " Node:" << std::endl;
        std::cout << "  Orden: " << order << std::endl;
        std::cout << "  Claves (" << keys.size() << "/" << getMaxKeys() << "): ";
        
        for (size_t i = 0; i < keys.size(); i++) {
            std::cout << keys[i];
            if (i < keys.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        std::cout << "  Estado: " << (isFull() ? "FULL" : (isEmpty() ? "EMPTY" : "PARTIAL")) << std::endl;
        std::cout << "  Padre: " << (parent ? "Sí" : "No (RAÍZ)") << std::endl;
    }
    
    /**
     * @brief Obtiene representación en string del nodo
     */
    virtual std::string toString() const {
        std::stringstream ss;
        ss << (isLeaf() ? "LEAF" : "INTERNAL") << "[";
        for (size_t i = 0; i < keys.size(); i++) {
            ss << keys[i];
            if (i < keys.size() - 1) ss << ",";
        }
        ss << "]";
        return ss.str();
    }

    // ============================================================================
    // OPERACIONES DE RANGO (PARA HOJAS)
    // ============================================================================
    
    /**
     * @brief Búsqueda por rango (implementación base, especializada en hojas)
     */
    virtual void rangeSearch(const KeyType& start_key, const KeyType& end_key,
                            std::vector<RecordReference>& results, int& found_count) {
        // Implementación base vacía - las hojas la sobrescriben
        found_count = 0;
    }

    // ============================================================================
    // INFORMACIÓN ESTADÍSTICA
    // ============================================================================
    
    /**
     * @brief Calcula el factor de ocupación del nodo
     */
    double getOccupancyFactor() const {
        return (double)keys.size() / getMaxKeys();
    }
    
    /**
     * @brief Obtiene estadísticas del nodo
     */
    struct NodeStats {
        size_t key_count;
        size_t max_keys;
        double occupancy;
        bool is_full;
        bool is_empty;
        bool is_root;
        bool is_leaf;
    };
    
    NodeStats getStats() const {
        NodeStats stats;
        stats.key_count = keys.size();
        stats.max_keys = getMaxKeys();
        stats.occupancy = getOccupancyFactor();
        stats.is_full = isFull();
        stats.is_empty = isEmpty();
        stats.is_root = isRoot();
        stats.is_leaf = isLeaf();
        return stats;
    }
};

#endif // BPLUS_NODE_H