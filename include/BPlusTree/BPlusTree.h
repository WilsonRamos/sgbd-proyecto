#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <memory>
#include <iostream>
#include <vector>
#include "../RecordReference.h"
#include "KeyComparator.h"
#include "BPlusNode.h"
#include "LeafNode.h"
#include "InternalNode.h"

template<typename KeyType>
class BPlusTree {
private:
    BPlusNode<KeyType>* root;
    int order;                    // Orden del árbol
    int height;                   // Altura del árbol
    size_t total_records;         // Número total de registros
    
    // Estadísticas
    size_t insert_operations;
    size_t search_operations;
    size_t split_operations;

public:
    BPlusTree(int tree_order = 4) 
        : root(nullptr)
        , order(tree_order)
        , height(0)
        , total_records(0)
        , insert_operations(0)
        , search_operations(0)
        , split_operations(0) {
        
        // Crear nodo hoja inicial como raíz
        root = new LeafNode<KeyType>(order);
        height = 1;
        
        std::cout << "[*] B+ Tree inicializado (orden: " << order << ")" << std::endl;
    }
    
    ~BPlusTree() {
        // Implementar limpieza recursiva si es necesario
        clearTree(root);
    }
    
    // Insertar registro
    bool insert(const KeyType& key, const RecordReference& record_ref) {
        insert_operations++;
        
        if (!root) {
            root = new LeafNode<KeyType>(order);
            height = 1;
        }
        
        bool success = insertHelper(root, key, record_ref);
        if (success) {
            total_records++;
        }
        
        return success;
    }
    
    // Buscar registro
    bool search(const KeyType& key, RecordReference& record_ref) {
        search_operations++;
        
        if (!root) return false;
        
        return searchHelper(root, key, record_ref);
    }
    
    // Búsqueda por rango
    std::vector<RecordReference> rangeSearch(const KeyType& start_key, const KeyType& end_key) {
        std::vector<RecordReference> results;
        
        if (!root) return results;
        
        // Encontrar el primer nodo hoja que puede contener start_key
        LeafNode<KeyType>* leaf = findLeafNode(start_key);
        
        // Iterar a través de nodos hoja hasta superar end_key
        while (leaf) {
            leaf->rangeSearch(start_key, end_key, results);
            
            // Verificar si el próximo nodo puede tener claves en el rango
            if (leaf->getNext() && !leaf->getNext()->getKeys().empty()) {
                if (KeyComparator<KeyType>::compare(leaf->getNext()->getKeys()[0], end_key) <= 0) {
                    leaf = leaf->getNext();
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        return results;
    }
    
    // Eliminar registro
    bool remove(const KeyType& key) {
        if (!root) return false;
        
        bool success = root->remove(key);
        if (success) {
            total_records--;
        }
        
        return success;
    }
    
    // Mostrar estructura del árbol
    void display() const {
        std::cout << "\n[*] ESTRUCTURA B+ TREE [*]" << std::endl;
        std::cout << "Orden: " << order << ", Altura: " << height << std::endl;
        std::cout << "Registros: " << total_records << std::endl;
        
        if (root) {
            root->display(0);
        } else {
            std::cout << "Árbol vacío" << std::endl;
        }
        std::cout << "========================" << std::endl;
    }
    
    // Estadísticas
    void displayStatistics() const {
        std::cout << "\n📊 ESTADÍSTICAS B+ TREE 📊" << std::endl;
        std::cout << "Registros totales: " << total_records << std::endl;
        std::cout << "Operaciones de inserción: " << insert_operations << std::endl;
        std::cout << "Operaciones de búsqueda: " << search_operations << std::endl;
        std::cout << "Divisiones de nodo: " << split_operations << std::endl;
        std::cout << "Orden del árbol: " << order << std::endl;
        std::cout << "Altura del árbol: " << height << std::endl;
        
        if (search_operations > 0) {
            std::cout << "Promedio de accesos por búsqueda: ~" << height << std::endl;
        }
    }
    
    /**
     * @brief Muestra la estructura del árbol
     */
    void displayTree() const {
        std::cout << "\n[*] ESTRUCTURA B+ TREE [*]" << std::endl;
        if (!root) {
            std::cout << "Árbol vacío" << std::endl;
            return;
        }
        
        std::cout << "Orden: " << order << ", Altura: " << height << std::endl;
        std::cout << "Registros: " << total_records << std::endl;
        std::cout << "Root: " << (root ? "Existe" : "Vacío") << std::endl;
        
        // Aquí podrías agregar más detalles de visualización si es necesario
        std::cout << "================================" << std::endl;
    }
    
    // Getters
    int getOrder() const { return order; }
    int getHeight() const { return height; }
    size_t getTotalRecords() const { return total_records; }
    
    // Verificar si está vacío
    bool isEmpty() const { return total_records == 0; }

private:
    bool insertHelper(BPlusNode<KeyType>* node, const KeyType& key, const RecordReference& record_ref) {
        if (node->insert(key, record_ref)) {
            return true;
        }
        
        // Si el nodo está lleno y es la raíz
        if (node->isFull() && node == root) {
            splitRoot(key, record_ref);
            split_operations++;
            return true;
        }
        
        return false;
    }
    
    bool searchHelper(BPlusNode<KeyType>* node, const KeyType& key, RecordReference& record_ref) {
        return node->search(key, record_ref);
    }
    
    void splitRoot(const KeyType& key, const RecordReference& record_ref) {
        auto old_root = root;
        
        if (old_root->isLeaf()) {
            // Dividir raíz hoja
            auto leaf = static_cast<LeafNode<KeyType>*>(old_root);
            auto new_leaf = leaf->split();
            
            if (new_leaf) {
                // Crear nueva raíz interna
                auto new_root = new InternalNode<KeyType>(order);
                new_root->getKeys().push_back(new_leaf->getKeys()[0]);
                new_root->addChild(leaf);
                new_root->addChild(new_leaf);
                
                root = new_root;
                height++;
                
                // Reintentar la inserción en el nodo apropiado
                if (KeyComparator<KeyType>::compare(key, new_leaf->getKeys()[0]) < 0) {
                    leaf->insert(key, record_ref);
                } else {
                    new_leaf->insert(key, record_ref);
                }
                
                std::cout << "[OK] Raiz hoja dividida exitosamente (nueva altura: " << height << ")" << std::endl;
            } else {
                // Fallback: insertar en el nodo actual si split falló
                std::cout << "[WARNING] Split fallo, insertando en nodo actual" << std::endl;
                leaf->insert(key, record_ref);
            }
        } else {
            // Dividir raíz interna
            auto internal = static_cast<InternalNode<KeyType>*>(old_root);
            std::cout << "[WARNING] Division de raiz interna - implementacion avanzada" << std::endl;
            
            // Para implementación completa, aquí iría la lógica de división de nodos internos
            // Por ahora, intentamos insertar directamente
            internal->insert(key, record_ref);
        }
    }
    
    LeafNode<KeyType>* findLeafNode(const KeyType& key) {
        BPlusNode<KeyType>* current = root;
        
        // Navegación completa del árbol
        while (current && !current->isLeaf()) {
            auto internal = static_cast<InternalNode<KeyType>*>(current);
            int child_index = 0;
            
            // Encontrar el índice del hijo correcto
            while (child_index < static_cast<int>(current->getKeys().size()) && 
                   KeyComparator<KeyType>::compare(key, current->getKeys()[child_index]) >= 0) {
                child_index++;
            }
            
            // Navegar al hijo correspondiente
            current = internal->getChild(child_index);
            if (!current) {
                std::cout << "[WARNING] Error: hijo nulo encontrado en navegacion" << std::endl;
                break;
            }
        }
        
        return static_cast<LeafNode<KeyType>*>(current);
    }
    
    void clearTree(BPlusNode<KeyType>* node) {
        if (!node) return;
        
        // Limpiar recursivamente si es nodo interno
        if (!node->isLeaf()) {
            auto internal = static_cast<InternalNode<KeyType>*>(node);
            
            // Obtener los hijos antes de eliminar
            auto children = internal->getChildren();
            for (auto child : children) {
                clearTree(child);
            }
        }
        
        // Eliminar el nodo actual
        delete node;
    }
};

#endif // BPLUS_TREE_H