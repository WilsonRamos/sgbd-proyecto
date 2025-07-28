#ifndef INTERNAL_NODE_H
#define INTERNAL_NODE_H

#include "BPlusNode.h"
#include "KeyComparator.h"
#include <memory>
#include <iostream>
#include <algorithm>

/**
 * @brief Nodo interno del B+ Tree
 * 
 * Características:
 * - No almacena RecordReference (solo en hojas)
 * - Tiene punteros a nodos hijos
 * - Las claves son separadores/guías
 * - Número de hijos = número de claves + 1
 * - Facilita navegación hacia hojas correctas
 */
template<typename KeyType>
class InternalNode : public BPlusNode<KeyType> {
private:
    std::vector<std::shared_ptr<BPlusNode<KeyType>>> children; // Punteros a hijos

public:
    /**
     * @brief Constructor
     */
    InternalNode(int order) : BPlusNode<KeyType>(order, false) {
        children.reserve(order);
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
     * @brief Inserta una clave en el nodo interno
     * NOTA: Los nodos internos no insertan directamente, sino que redirigen
     */
    bool insert(const KeyType& key, const RecordReference& record_ref) override {
        // Los nodos internos no insertan directamente
        // Esta función es principalmente para compatibilidad
        auto child = findChildForKey(key);
        if (child) {
            return child->insert(key, record_ref);
        }
        return false;
    }

    /**
     * @brief Busca una clave navegando hacia el hijo apropiado
     */
    bool search(const KeyType& key, RecordReference& record_ref) override {
        auto child = findChildForKey(key);
        if (child) {
            return child->search(key, record_ref);
        }
        return false;
    }

    /**
     * @brief Elimina una clave (redirige al hijo apropiado)
     */
    bool remove(const KeyType& key) override {
        auto child = findChildForKey(key);
        if (child) {
            return child->remove(key);
        }
        return false;
    }

    /**
     * @brief División de nodo interno cuando está lleno
     */
    BPlusNode<KeyType>* split() override {
        if (!this->isFull()) {
            return nullptr;
        }

        int mid = this->order / 2;
        auto new_internal = new InternalNode<KeyType>(this->order);

        // La clave del medio sube al padre
        KeyType middle_key = this->keys[mid];

        // Mover claves de la mitad derecha al nuevo nodo
        new_internal->keys.assign(this->keys.begin() + mid + 1, this->keys.end());
        
        // Mover hijos correspondientes
        new_internal->children.assign(children.begin() + mid + 1, children.end());

        // Actualizar padres de los hijos movidos
        for (auto& child : new_internal->children) {
            child->setParent(new_internal);
        }

        // Mantener la mitad izquierda
        this->keys.resize(mid);
        children.resize(mid + 1);

        // Establecer padre
        new_internal->setParent(this->getParent());

        std::cout << "🌳 Nodo interno dividido: " << this->keys.size() 
                  << " + " << new_internal->keys.size() << " claves" << std::endl;
        std::cout << "   Clave promovida: " << middle_key << std::endl;

        return new_internal;
    }

    // ============================================================================
    // GESTIÓN DE HIJOS
    // ============================================================================

    /**
     * @brief Añade un hijo al nodo
     */
    void addChild(std::shared_ptr<BPlusNode<KeyType>> child) {
        if (children.size() < static_cast<size_t>(this->order)) {
            children.push_back(child);
            child->setParent(this);
        }
    }

    /**
     * @brief Inserta un hijo en una posición específica
     */
    void insertChild(size_t index, std::shared_ptr<BPlusNode<KeyType>> child) {
        if (index <= children.size()) {
            children.insert(children.begin() + index, child);
            child->setParent(this);
        }
    }

    /**
     * @brief Obtiene un hijo por índice
     */
    std::shared_ptr<BPlusNode<KeyType>> getChild(size_t index) const {
        if (index < children.size()) {
            return children[index];
        }
        return nullptr;
    }

    /**
     * @brief Encuentra el hijo apropiado para una clave
     */
    std::shared_ptr<BPlusNode<KeyType>> findChildForKey(const KeyType& key) const {
        size_t i = 0;
        
        // Encontrar la posición donde la clave debería ir
        while (i < this->keys.size() && KeyComparator<KeyType>::lessEqual(key, this->keys[i])) {
            if (KeyComparator<KeyType>::equal(key, this->keys[i])) {
                // Si la clave es igual, ir al hijo derecho
                i++;
                break;
            }
            i++;
        }

        return getChild(i);
    }

    /**
     * @brief Encuentra el índice del hijo que contiene una clave
     */
    int findChildIndex(std::shared_ptr<BPlusNode<KeyType>> child) const {
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i] == child) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Elimina un hijo
     */
    bool removeChild(std::shared_ptr<BPlusNode<KeyType>> child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Número de hijos
     */
    size_t getChildCount() const {
        return children.size();
    }

    /**
     * @brief Obtiene todos los hijos
     */
    const std::vector<std::shared_ptr<BPlusNode<KeyType>>>& getChildren() const {
        return children;
    }

    // ============================================================================
    // OPERACIONES DE RANGO
    // ============================================================================

    /**
     * @brief Búsqueda por rango (redirige a hojas)
     */
    void rangeSearch(const KeyType& start_key, const KeyType& end_key,
                     std::vector<RecordReference>& results, int& found_count) override {
        found_count = 0;

        // Encontrar el hijo que podría contener start_key
        auto start_child = findChildForKey(start_key);
        if (!start_child) return;

        // Realizar búsqueda por rango en el subárbol
        start_child->rangeSearch(start_key, end_key, results, found_count);

        // Si abarca múltiples hijos, continuar con los siguientes
        // (Esta lógica se puede optimizar más)
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i] == start_child) {
                // Continuar con hijos siguientes si es necesario
                for (size_t j = i + 1; j < children.size(); j++) {
                    // Verificar si este hijo podría contener claves en el rango
                    if (j > 0 && KeyComparator<KeyType>::greater(this->keys[j-1], end_key)) {
                        break; // No más hijos relevantes
                    }
                    
                    int additional_found = 0;
                    children[j]->rangeSearch(start_key, end_key, results, additional_found);
                    found_count += additional_found;
                }
                break;
            }
        }
    }

    // ============================================================================
    // INSERCIÓN DE CLAVES EN NODO INTERNO
    // ============================================================================

    /**
     * @brief Inserta una clave separadora en el nodo interno
     */
    bool insertSeparatorKey(const KeyType& key, std::shared_ptr<BPlusNode<KeyType>> left_child,
                           std::shared_ptr<BPlusNode<KeyType>> right_child) {
        if (this->isFull()) {
            return false; // Necesita split primero
        }

        // Encontrar posición de inserción
        int pos = this->findInsertPosition(key);

        // Insertar clave
        this->keys.insert(this->keys.begin() + pos, key);

        // Actualizar hijos
        if (pos < static_cast<int>(children.size())) {
            children[pos] = left_child;
            children.insert(children.begin() + pos + 1, right_child);
        } else {
            children.push_back(right_child);
        }

        // Establecer padres
        left_child->setParent(this);
        right_child->setParent(this);

        return true;
    }

    // ============================================================================
    // SERIALIZACIÓN
    // ============================================================================

    /**
     * @brief Serializa el nodo interno
     */
    std::string serialize() const {
        std::ostringstream oss;
        
        oss << "INTERNAL_NODE|" << this->order << "|" << this->keys.size() << std::endl;
        
        // Serializar claves
        for (const auto& key : this->keys) {
            oss << "KEY|" << key << std::endl;
        }
        
        // Serializar hijos (recursivamente)
        for (const auto& child : children) {
            oss << "CHILD_START" << std::endl;
            oss << child->serialize();
            oss << "CHILD_END" << std::endl;
        }
        
        return oss.str();
    }

    /**
     * @brief Deserializa el nodo interno
     */
    bool deserialize(const std::string& data) {
        // Implementación básica - puede expandirse según necesidades
        std::istringstream iss(data);
        std::string line;
        
        this->keys.clear();
        children.clear();
        
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            if (line.find("KEY|") == 0) {
                KeyType key;
                std::istringstream key_stream(line.substr(4));
                key_stream >> key;
                this->keys.push_back(key);
            }
            // Deserialización de hijos sería más compleja
            // Se implementaría según necesidades específicas
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================

    /**
     * @brief Muestra el nodo interno
     */
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        
        std::cout << indent << "InternalNode (Level " << level << "): [";
        for (size_t i = 0; i < this->keys.size(); i++) {
            std::cout << this->keys[i];
            if (i < this->keys.size() - 1) std::cout << ", ";
        }
        std::cout << "] (" << children.size() << " children)" << std::endl;

        // Mostrar hijos
        for (size_t i = 0; i < children.size(); i++) {
            std::cout << indent << "Child[" << i << "]:" << std::endl;
            if (children[i]) {
                children[i]->display(level + 1);
            } else {
                std::cout << indent << "  (null)" << std::endl;
            }
        }
    }

    /**
     * @brief Información detallada del nodo interno
     */
    void displayDetailed() const {
        std::cout << "\n🌳 NODO INTERNO DETALLADO:" << std::endl;
        this->displayBasicInfo();
        std::cout << "  Número de hijos: " << children.size() << std::endl;
        
        // Verificar consistencia
        bool consistent = (children.size() == this->keys.size() + 1);
        std::cout << "  Consistencia hijos/claves: " << (consistent ? "✓" : "✗") << std::endl;

        if (!this->keys.empty()) {
            std::cout << "  Rango de claves: [" << this->keys.front() 
                      << " ... " << this->keys.back() << "]" << std::endl;
        }
    }

    /**
     * @brief Validación de consistencia del nodo interno
     */
    bool validateConsistency() const {
        // Verificar que número de hijos = número de claves + 1
        if (children.size() != this->keys.size() + 1) {
            std::cout << "❌ Inconsistencia: " << children.size() << " hijos, " 
                      << this->keys.size() << " claves" << std::endl;
            return false;
        }

        // Verificar que las claves están ordenadas
        for (size_t i = 1; i < this->keys.size(); i++) {
            if (KeyComparator<KeyType>::greater(this->keys[i-1], this->keys[i])) {
                std::cout << "❌ Claves desordenadas: " << this->keys[i-1] 
                          << " > " << this->keys[i] << std::endl;
                return false;
            }
        }

        // Verificar que todos los hijos tienen este nodo como padre
        for (const auto& child : children) {
            if (child && child->getParent() != this) {
                std::cout << "❌ Hijo con padre incorrecto" << std::endl;
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Encuentra la hoja más a la izquierda en el subárbol
     */
    std::shared_ptr<BPlusNode<KeyType>> getLeftmostLeaf() const {
        if (children.empty()) return nullptr;
        
        auto current = children[0];
        while (current && !current->isLeaf()) {
            auto internal = std::dynamic_pointer_cast<InternalNode<KeyType>>(current);
            if (internal && !internal->children.empty()) {
                current = internal->children[0];
            } else {
                break;
            }
        }
        
        return current;
    }

    /**
     * @brief Encuentra la hoja más a la derecha en el subárbol
     */
    std::shared_ptr<BPlusNode<KeyType>> getRightmostLeaf() const {
        if (children.empty()) return nullptr;
        
        auto current = children.back();
        while (current && !current->isLeaf()) {
            auto internal = std::dynamic_pointer_cast<InternalNode<KeyType>>(current);
            if (internal && !internal->children.empty()) {
                current = internal->children.back();
            } else {
                break;
            }
        }
        
        return current;
    }
};

#endif // INTERNAL_NODE_H