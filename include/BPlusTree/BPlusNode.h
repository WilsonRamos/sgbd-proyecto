#ifndef BPLUS_NODE_H
#define BPLUS_NODE_H

#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "KeyComparator.h"
#include "../RecordReference.h"

/**
 * @brief Nodo base para B+ Tree ACTUALIZADO
 * 
 * ✅ MEJORAS IMPLEMENTADAS:
 * - Integración completa con RecordReference
 * - Métodos virtuales optimizados para polimorfismo
 * - Validación de consistencia robusta
 * - Soporte para serialización avanzada
 * - Estadísticas detalladas por nodo
 * - Gestión mejorada de memoria
 */
template<typename KeyType>
class BPlusNode {
protected:
    std::vector<KeyType> keys;           // Claves almacenadas en el nodo
    int order;                          // Orden del B+ Tree
    bool is_leaf_node;                  // Indica si es nodo hoja
    BPlusNode* parent;                  // Puntero al nodo padre
    
    // Estadísticas del nodo (mutable para métodos const)
    mutable size_t access_count;        // Número de accesos al nodo
    mutable size_t modification_count;  // Número de modificaciones

public:
    /**
     * @brief Constructor protegido (solo para clases derivadas)
     */
    BPlusNode(int node_order, bool is_leaf) 
        : order(node_order)
        , is_leaf_node(is_leaf)
        , parent(nullptr)
        , access_count(0)
        , modification_count(0)
    {
        keys.reserve(order - 1); // Máximo order-1 claves
    }

    /**
     * @brief Destructor virtual
     */
    virtual ~BPlusNode() = default;

    // ============================================================================
    // MÉTODOS VIRTUALES PUROS (IMPLEMENTADOS EN CLASES DERIVADAS)
    // ============================================================================
    
    /**
     * @brief Identifica el tipo de nodo
     */
    virtual bool isLeaf() const = 0;
    
    /**
     * @brief Inserta una clave con su RecordReference
     */
    virtual bool insert(const KeyType& key, const RecordReference& record_ref) = 0;
    
    /**
     * @brief Busca una clave y retorna su RecordReference
     */
    virtual bool search(const KeyType& key, RecordReference& record_ref) = 0;
    
    /**
     * @brief Elimina una clave del nodo
     */
    virtual bool remove(const KeyType& key) = 0;
    
    /**
     * @brief Divide el nodo cuando está lleno
     */
    virtual BPlusNode* split() = 0;
    
    /**
     * @brief Muestra el contenido del nodo
     */
    virtual void display(int level = 0) const = 0;

    /**
     * @brief Serializa el nodo
     */
    virtual std::string serialize() const = 0;

    /**
     * @brief Deserializa el nodo
     */
    virtual bool deserialize(const std::string& data) = 0;

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
    // PROPIEDADES Y ESTADO DEL NODO
    // ============================================================================
    
    /**
     * @brief Verifica si el nodo está lleno
     */
    bool isFull() const {
        return keys.size() >= static_cast<size_t>(getMaxKeys());
    }
    
    /**
     * @brief Verifica si el nodo está vacío
     */
    bool isEmpty() const {
        return keys.empty();
    }
    
    /**
     * @brief Verifica si el nodo está por debajo del mínimo
     */
    bool isUnderflow() const {
        if (isRoot()) {
            return false; // La raíz puede tener menos claves
        }
        return keys.size() < static_cast<size_t>(getMinKeys());
    }
    
    /**
     * @brief Verifica si es nodo raíz
     */
    bool isRoot() const {
        return parent == nullptr;
    }

    /**
     * @brief Número máximo de claves que puede contener
     */
    int getMaxKeys() const {
        return order - 1;
    }

    /**
     * @brief Número mínimo de claves que debe contener
     */
    int getMinKeys() const {
        if (is_leaf_node) {
            return (order - 1) / 2; // Para hojas
        } else {
            return order / 2 - 1;   // Para nodos internos
        }
    }

    /**
     * @brief Número actual de claves
     */
    size_t getKeyCount() const {
        return keys.size();
    }

    // ============================================================================
    // GESTIÓN DE PADRE
    // ============================================================================
    
    BPlusNode* getParent() const { return parent; }
    void setParent(BPlusNode* p) { parent = p; }

    // ============================================================================
    // OPERACIONES CON CLAVES
    // ============================================================================
    
    /**
     * @brief Busca el índice de una clave
     */
    int findKey(const KeyType& key) const {
        for (size_t i = 0; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::equal(keys[i], key)) {
                return static_cast<int>(i);
            }
        }
        return -1; // No encontrada
    }

    /**
     * @brief Encuentra la posición de inserción para una clave
     */
    int findInsertPosition(const KeyType& key) const {
        int pos = 0;
        while (pos < static_cast<int>(keys.size()) && 
               KeyComparator<KeyType>::less(keys[pos], key)) {
            pos++;
        }
        return pos;
    }

    /**
     * @brief Verifica si una clave existe en el nodo
     */
    bool hasKey(const KeyType& key) const {
        return findKey(key) != -1;
    }

    /**
     * @brief Obtiene la clave mínima
     */
    KeyType getMinKey() const {
        return keys.empty() ? KeyType{} : keys.front();
    }

    /**
     * @brief Obtiene la clave máxima
     */
    KeyType getMaxKey() const {
        return keys.empty() ? KeyType{} : keys.back();
    }

    /**
     * @brief Verifica si las claves están ordenadas
     */
    bool areKeysSorted() const {
        for (size_t i = 1; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::greater(keys[i-1], keys[i])) {
                return false;
            }
        }
        return true;
    }

    // ============================================================================
    // ESTADÍSTICAS Y ANÁLISIS
    // ============================================================================
    
    /**
     * @brief Incrementa contador de accesos
     */
    void recordAccess() const {
        access_count++;
    }

    /**
     * @brief Incrementa contador de modificaciones
     */
    void recordModification() {
        modification_count++;
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
        size_t access_count;
        size_t modification_count;
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
        stats.access_count = access_count;
        stats.modification_count = modification_count;
        return stats;
    }

    /**
     * @brief Calcula el factor de ocupación del nodo
     */
    double getOccupancyFactor() const {
        return (double)keys.size() / getMaxKeys();
    }

    // ============================================================================
    // INFORMACIÓN BÁSICA DEL NODO
    // ============================================================================
    
    /**
     * @brief Muestra información básica del nodo
     */
    void displayBasicInfo() const {
        std::cout << "  Tipo: " << (isLeaf() ? "HOJA" : "INTERNO") << std::endl;
        std::cout << "  Claves: " << keys.size() << "/" << getMaxKeys() << std::endl;
        std::cout << "  Ocupación: " << std::fixed << std::setprecision(1) 
                  << (getOccupancyFactor() * 100) << "%" << std::endl;
        std::cout << "  Estado: " << (isFull() ? "FULL" : (isEmpty() ? "EMPTY" : "PARTIAL")) << std::endl;
        std::cout << "  Padre: " << (parent ? "Sí" : "No (RAÍZ)") << std::endl;
        std::cout << "  Accesos: " << access_count << std::endl;
        std::cout << "  Modificaciones: " << modification_count << std::endl;
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
    // VALIDACIÓN DE CONSISTENCIA
    // ============================================================================
    
    /**
     * @brief Valida la consistencia del nodo
     */
    virtual bool validateConsistency() const {
        // Verificar que las claves estén ordenadas
        if (!areKeysSorted()) {
            std::cout << "❌ Claves desordenadas en nodo" << std::endl;
            return false;
        }

        // Verificar límites de claves
        if (keys.size() > static_cast<size_t>(getMaxKeys())) {
            std::cout << "❌ Demasiadas claves: " << keys.size() 
                      << " > " << getMaxKeys() << std::endl;
            return false;
        }

        // Verificar underflow (excepto para raíz)
        if (!isRoot() && isUnderflow()) {
            std::cout << "❌ Underflow en nodo: " << keys.size() 
                      << " < " << getMinKeys() << std::endl;
            return false;
        }

        return true;
    }

    // ============================================================================
    // UTILIDADES PARA CLASES DERIVADAS
    // ============================================================================

protected:
    /**
     * @brief Inserta una clave en la posición correcta
     */
    bool insertKeyAtPosition(const KeyType& key, int position) {
        if (isFull()) {
            return false;
        }

        if (position < 0 || position > static_cast<int>(keys.size())) {
            return false;
        }

        keys.insert(keys.begin() + position, key);
        recordModification();
        return true;
    }

    /**
     * @brief Elimina una clave por índice
     */
    bool removeKeyAtIndex(int index) {
        if (index < 0 || index >= static_cast<int>(keys.size())) {
            return false;
        }

        keys.erase(keys.begin() + index);
        recordModification();
        return true;
    }

    /**
     * @brief Reorganiza las claves para mantener el orden
     */
    void sortKeys() {
        std::sort(keys.begin(), keys.end(), 
            [](const KeyType& a, const KeyType& b) {
                return KeyComparator<KeyType>::less(a, b);
            });
    }

public:
    // ============================================================================
    // ACCESO CONTROLADO A CLAVES (PARA DEBUGGING)
    // ============================================================================
    
    /**
     * @brief Obtiene todas las claves (solo lectura)
     */
    const std::vector<KeyType>& getKeys() const {
        recordAccess();
        return keys;
    }

    /**
     * @brief Obtiene una clave por índice
     */
    KeyType getKeyAt(size_t index) const {
        recordAccess();
        if (index < keys.size()) {
            return keys[index];
        }
        return KeyType{}; // Valor por defecto
    }

    // ============================================================================
    // MÉTODOS PARA ANÁLISIS EDUCATIVO
    // ============================================================================
    
    /**
     * @brief Analiza la distribución de claves en el nodo
     */
    void analyzeKeyDistribution() const {
        if (keys.empty()) {
            std::cout << "📊 Nodo vacío - sin análisis" << std::endl;
            return;
        }

        std::cout << "\n📊 ANÁLISIS DE DISTRIBUCIÓN DE CLAVES:" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Número de claves: " << keys.size() << std::endl;
        std::cout << "Capacidad máxima: " << getMaxKeys() << std::endl;
        std::cout << "Factor de ocupación: " << std::fixed << std::setprecision(1) 
                  << (getOccupancyFactor() * 100) << "%" << std::endl;
        
        std::cout << "Rango de claves: [" << getMinKey() << " - " << getMaxKey() << "]" << std::endl;
        std::cout << "Claves ordenadas: " << (areKeysSorted() ? "✓" : "✗") << std::endl;

        // Mostrar algunas claves como muestra
        std::cout << "Muestra de claves: ";
        size_t sample_size = std::min(static_cast<size_t>(5), keys.size());
        for (size_t i = 0; i < sample_size; i++) {
            std::cout << keys[i];
            if (i < sample_size - 1) std::cout << ", ";
        }
        if (keys.size() > sample_size) {
            std::cout << "...";
        }
        std::cout << std::endl;
    }

    /**
     * @brief Información de rendimiento del nodo
     */
    void displayPerformanceInfo() const {
        std::cout << "\n⚡ RENDIMIENTO DEL NODO:" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "Accesos totales: " << access_count << std::endl;
        std::cout << "Modificaciones: " << modification_count << std::endl;
        
        if (access_count > 0) {
            double modification_rate = static_cast<double>(modification_count) / access_count;
            std::cout << "Tasa de modificación: " << std::fixed << std::setprecision(2) 
                      << (modification_rate * 100) << "%" << std::endl;
        }

        std::cout << "Complejidad búsqueda: O(" << keys.size() << ") - búsqueda lineal" << std::endl;
    }

    // ============================================================================
    // FRIEND CLASSES Y TEMPLATE FRIENDS
    // ============================================================================
    
    // Permitir acceso a clases derivadas y BPlusTree
    template<typename T> friend class BPlusTree;
    template<typename T> friend class LeafNode;
    template<typename T> friend class InternalNode;
};

#endif // BPLUS_NODE_H