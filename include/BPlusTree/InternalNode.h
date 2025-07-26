#ifndef INTERNAL_NODE_H
#define INTERNAL_NODE_H

#include "BPlusNode.h"
#include "../RecordReference.h"
#include <iostream>

/**
 * @brief Nodo interno del B+ Tree
 * 
 * Características especiales:
 * - No almacena datos, solo claves de navegación
 * - Mantiene punteros a nodos hijo
 * - Siempre tiene un hijo más que claves
 * - Responsable de la navegación en el árbol
 */
template<typename KeyType>
class InternalNode : public BPlusNode<KeyType> {
private:
    std::vector<BPlusNode<KeyType>*> children; // Punteros a nodos hijo

public:
    /**
     * @brief Constructor
     */
    InternalNode(int order) : BPlusNode<KeyType>(order, false) {
        children.reserve(order); // Un nodo interno puede tener hasta 'order' hijos
    }
    
    /**
     * @brief Destructor
     */
    ~InternalNode() override = default;

    // ============================================================================
    // IMPLEMENTACIÓN DE MÉTODOS VIRTUALES
    // ============================================================================
    
    /**
     * @brief Identifica que es un nodo interno
     */
    bool isLeaf() const override { 
        return false; 
    }
    
    /**
     * @brief Inserta una clave (delega a los hijos apropiados)
     */
    bool insert(const KeyType& key, const RecordReference& record_ref) override {
        // Los nodos internos no insertan directamente, delegan a sus hijos
        BPlusNode<KeyType>* child = findChild(key);
        
        if (child) {
            return child->insert(key, record_ref);
        }
        
        return false;
    }
    
    /**
     * @brief Busca una clave (navega hacia los hijos apropiados)
     */
    bool search(const KeyType& key, RecordReference& record_ref) override {
        BPlusNode<KeyType>* child = findChild(key);
        
        if (child) {
            return child->search(key, record_ref);
        }
        
        return false;
    }
    
    /**
     * @brief Elimina una clave (delega a los hijos apropiados)
     */
    bool remove(const KeyType& key) override {
        BPlusNode<KeyType>* child = findChild(key);
        
        if (child) {
            return child->remove(key);
        }
        
        return false;
    }
    
    /**
     * @brief Divide el nodo interno cuando está lleno
     */
    BPlusNode<KeyType>* split() override {
        if (!this->isFull()) {
            return nullptr;
        }
        
        int mid = this->order / 2;
        auto new_internal = new InternalNode<KeyType>(this->order);
        
        // La clave del medio sube al padre, no se incluye en ningún nodo
        KeyType middle_key = this->keys[mid];
        
        // Mover claves de la derecha al nuevo nodo (excluyendo la del medio)
        new_internal->keys.assign(this->keys.begin() + mid + 1, this->keys.end());
        
        // Mover hijos correspondientes
        new_internal->children.assign(children.begin() + mid + 1, children.end());
        
        // Actualizar padres de los hijos movidos
        for (auto* child : new_internal->children) {
            child->setParent(new_internal);
        }
        
        // Mantener la mitad izquierda (sin la clave del medio)
        this->keys.resize(mid);
        children.resize(mid + 1);
        
        // Establecer padre
        new_internal->setParent(this->getParent());
        
        std::cout << "🌳 Nodo interno dividido: " << this->keys.size() 
                  << " + " << new_internal->keys.size() << " claves" << std::endl;
        std::cout << "   Clave promocionada: " << middle_key << std::endl;
        
        return new_internal;
    }

    // ============================================================================
    // GESTIÓN DE HIJOS
    // ============================================================================
    
    /**
     * @brief Encuentra el hijo apropiado para una clave
     */
    BPlusNode<KeyType>* findChild(const KeyType& key) {
        if (children.empty()) {
            return nullptr;
        }
        
        // Encontrar el índice del primer hijo apropiado
        int index = 0;
        while (index < static_cast<int>(this->keys.size()) && 
               KeyComparator<KeyType>::compare(key, this->keys[index]) >= 0) {
            index++;
        }
        
        // El hijo está en la posición 'index'
        if (index < static_cast<int>(children.size())) {
            return children[index];
        }
        
        return nullptr;
    }
    
    /**
     * @brief Añade un hijo al nodo
     */
    void addChild(BPlusNode<KeyType>* child) {
        if (child) {
            children.push_back(child);
            child->setParent(this);
        }
    }
    
    /**
     * @brief Inserta un hijo en una posición específica
     */
    void insertChild(int index, BPlusNode<KeyType>* child) {
        if (child && index >= 0 && index <= static_cast<int>(children.size())) {
            children.insert(children.begin() + index, child);
            child->setParent(this);
        }
    }
    
    /**
     * @brief Remueve un hijo
     */
    bool removeChild(BPlusNode<KeyType>* child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Obtiene todos los hijos
     */
    const std::vector<BPlusNode<KeyType>*>& getChildren() const {
        return children;
    }
    
    /**
     * @brief Obtiene un hijo específico por índice
     */
    BPlusNode<KeyType>* getChild(size_t index) const {
        if (index < children.size()) {
            return children[index];
        }
        return nullptr;
    }
    
    /**
     * @brief Obtiene el número de hijos
     */
    size_t getChildCount() const {
        return children.size();
    }

    // ============================================================================
    // INSERCIÓN DE CLAVES CON MANEJO DE HIJOS
    // ============================================================================
    
    /**
     * @brief Inserta una clave de separación y reorganiza hijos
     */
    bool insertSeparatorKey(const KeyType& key, BPlusNode<KeyType>* left_child, 
                            BPlusNode<KeyType>* right_child) {
        
        if (this->isFull()) {
            return false; // Necesita split primero
        }
        
        // Encontrar posición donde insertar la clave
        int pos = this->findInsertPosition(key);
        
        // Insertar la clave
        this->keys.insert(this->keys.begin() + pos, key);
        
        // El hijo izquierdo ya debe estar en la posición correcta
        // Insertar el hijo derecho en pos + 1
        if (pos + 1 <= static_cast<int>(children.size())) {
            children.insert(children.begin() + pos + 1, right_child);
            right_child->setParent(this);
        }
        
        return true;
    }
    
    /**
     * @brief Actualiza una clave de separación
     */
    bool updateSeparatorKey(const KeyType& old_key, const KeyType& new_key) {
        int index = this->findKey(old_key);
        if (index != -1) {
            this->keys[index] = new_key;
            return true;
        }
        return false;
    }

    // ============================================================================
    // NAVEGACIÓN Y BÚSQUEDA
    // ============================================================================
    
    /**
     * @brief Encuentra el hijo más a la izquierda (para encontrar el mínimo)
     */
    BPlusNode<KeyType>* getLeftmostChild() const {
        if (!children.empty()) {
            return children.front();
        }
        return nullptr;
    }
    
    /**
     * @brief Encuentra el hijo más a la derecha (para encontrar el máximo)
     */
    BPlusNode<KeyType>* getRightmostChild() const {
        if (!children.empty()) {
            return children.back();
        }
        return nullptr;
    }
    
    /**
     * @brief Obtiene el rango de claves que maneja este nodo
     */
    std::pair<KeyType, KeyType> getKeyRange() const {
        if (this->keys.empty()) {
            throw std::runtime_error("Nodo interno sin claves");
        }
        
        return std::make_pair(this->keys.front(), this->keys.back());
    }

    // ============================================================================
    // VALIDACIÓN ESPECÍFICA DE NODOS INTERNOS
    // ============================================================================
    
    /**
     * @brief Valida la integridad del nodo interno
     */
    bool validateNode() const override {
        // Validación de la clase base
        if (!BPlusNode<KeyType>::validateNode()) {
            return false;
        }
        
        // Verificar que tenga un hijo más que claves
        if (children.size() != this->keys.size() + 1) {
            std::cout << "❌ Error: Número de hijos (" << children.size() 
                      << ") debe ser claves + 1 (" << this->keys.size() + 1 << ")" << std::endl;
            return false;
        }
        
        // Verificar que todos los hijos tengan este nodo como padre
        for (const auto* child : children) {
            if (child && child->getParent() != this) {
                std::cout << "❌ Error: Hijo no tiene este nodo como padre" << std::endl;
                return false;
            }
        }
        
        // Verificar que no haya hijos nulos
        for (const auto* child : children) {
            if (!child) {
                std::cout << "❌ Error: Hijo nulo encontrado" << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief Valida la estructura del subárbol
     */
    bool validateSubtree() const {
        if (!validateNode()) {
            return false;
        }
        
        // Validar recursivamente todos los hijos
        for (const auto* child : children) {
            if (child) {
                if (!child->validateNode()) {
                    return false;
                }
                
                // Si el hijo es interno, validar su subárbol también
                if (!child->isLeaf()) {
                    const auto* internal_child = static_cast<const InternalNode<KeyType>*>(child);
                    if (!internal_child->validateSubtree()) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra información detallada del nodo interno
     */
    void displayInfo() const override {
        std::cout << "🌳 NODO INTERNO:" << std::endl;
        std::cout << "  Orden: " << this->order << std::endl;
        std::cout << "  Claves: " << this->keys.size() << "/" << this->getMaxKeys() << std::endl;
        std::cout << "  Hijos: " << children.size() << "/" << this->order << std::endl;
        std::cout << "  Ocupación: " << std::fixed << std::setprecision(1) 
                  << this->getOccupancyFactor() * 100 << "%" << std::endl;
        
        // Mostrar claves de separación
        if (!this->keys.empty()) {
            std::cout << "  Claves de separación: ";
            for (size_t i = 0; i < this->keys.size(); i++) {
                std::cout << this->keys[i];
                if (i < this->keys.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        // Información de hijos
        std::cout << "  Hijos (" << children.size() << "):" << std::endl;
        for (size_t i = 0; i < children.size() && i < 3; i++) {
            std::cout << "    [" << i << "] " << (children[i]->isLeaf() ? "HOJA" : "INTERNO") 
                      << " con " << children[i]->getKeys().size() << " claves" << std::endl;
        }
        
        if (children.size() > 3) {
            std::cout << "    ... (" << (children.size() - 3) << " hijos más)" << std::endl;
        }
        
        // Estado
        std::cout << "  Estado: " << (this->isFull() ? "LLENO" : 
                                    (this->isEmpty() ? "VACÍO" : "PARCIAL")) << std::endl;
    }
    
    /**
     * @brief Representación en string del nodo interno
     */
    std::string toString() const override {
        std::stringstream ss;
        ss << "INTERNAL[";
        for (size_t i = 0; i < this->keys.size(); i++) {
            ss << this->keys[i];
            if (i < this->keys.size() - 1) ss << ",";
        }
        ss << "](" << children.size() << " hijos)";
        return ss.str();
    }
    
    /**
     * @brief Muestra la estructura del subárbol
     */
    void displaySubtree(int depth = 0) const {
        std::string indent(depth * 2, ' ');
        std::cout << indent << toString() << std::endl;
        
        // Mostrar hijos recursivamente
        for (const auto* child : children) {
            if (child) {
                if (child->isLeaf()) {
                    std::cout << indent << "  " << child->toString() << std::endl;
                } else {
                    const auto* internal_child = static_cast<const InternalNode<KeyType>*>(child);
                    internal_child->displaySubtree(depth + 1);
                }
            }
        }
    }

    // ============================================================================
    // ESTADÍSTICAS ESPECÍFICAS DE NODOS INTERNOS
    // ============================================================================
    
    /**
     * @brief Obtiene estadísticas del nodo interno
     */
    struct InternalStats {
        size_t key_count;
        size_t child_count;
        size_t leaf_children;
        size_t internal_children;
        int subtree_height;
        double occupancy;
        bool all_children_valid;
    };
    
    InternalStats getInternalStats() const {
        InternalStats stats;
        stats.key_count = this->keys.size();
        stats.child_count = children.size();
        stats.leaf_children = 0;
        stats.internal_children = 0;
        stats.subtree_height = 1;
        stats.occupancy = this->getOccupancyFactor();
        stats.all_children_valid = true;
        
        int max_child_height = 0;
        
        for (const auto* child : children) {
            if (child) {
                if (child->isLeaf()) {
                    stats.leaf_children++;
                } else {
                    stats.internal_children++;
                    const auto* internal_child = static_cast<const InternalNode<KeyType>*>(child);
                    int child_height = internal_child->getInternalStats().subtree_height;
                    max_child_height = std::max(max_child_height, child_height);
                }
            } else {
                stats.all_children_valid = false;
            }
        }
        
        stats.subtree_height = 1 + max_child_height;
        
        return stats;
    }
    
    /**
     * @brief Cuenta el número total de nodos en el subárbol
     */
    size_t countNodesInSubtree() const {
        size_t count = 1; // Este nodo
        
        for (const auto* child : children) {
            if (child) {
                if (child->isLeaf()) {
                    count += 1;
                } else {
                    const auto* internal_child = static_cast<const InternalNode<KeyType>*>(child);
                    count += internal_child->countNodesInSubtree();
                }
            }
        }
        
        return count;
    }
    
    /**
     * @brief Calcula la altura del subárbol
     */
    int getSubtreeHeight() const {
        if (children.empty()) {
            return 1;
        }
        
        int max_height = 0;
        for (const auto* child : children) {
            if (child) {
                int child_height = 1;
                if (!child->isLeaf()) {
                    const auto* internal_child = static_cast<const InternalNode<KeyType>*>(child);
                    child_height = internal_child->getSubtreeHeight();
                }
                max_height = std::max(max_height, child_height);
            }
        }
        
        return 1 + max_height;
    }
};

#endif // INTERNAL_NODE_H