#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <memory>
#include <iostream>
#include <vector>
#include <functional>
#include "../RecordReference.h"
#include "KeyComparator.h"
#include "BPlusNode.h"
#include "LeafNode.h"
#include "InternalNode.h"

/**
 * @brief B+ Tree con soporte para persistencia
 * 
 * Implementación educativa de B+ Tree con:
 * - Nodos internos y hojas separados
 * - Enlaces horizontales entre hojas
 * - Búsquedas por rango eficientes
 * - Estadísticas detalladas
 * - Flujo educativo de consultas
 */
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
    size_t range_operations;
    size_t split_operations;

public:
    /**
     * @brief Constructor
     */
    BPlusTree(int tree_order = 4) 
        : root(nullptr)
        , order(tree_order)
        , height(0)
        , total_records(0)
        , insert_operations(0)
        , search_operations(0)
        , range_operations(0)
        , split_operations(0) {
        
        // Crear nodo hoja inicial como raíz
        root = new LeafNode<KeyType>(order);
        height = 1;
        
        std::cout << "🌳 B+ Tree inicializado (orden: " << order << ")" << std::endl;
    }
    
    /**
     * @brief Destructor
     */
    ~BPlusTree() {
        clearTree(root);
    }
    
    // ============================================================================
    // OPERACIONES BÁSICAS DEL ÁRBOL
    // ============================================================================
    
    /**
     * @brief Insertar registro con RecordReference
     */
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
    
    /**
     * @brief Buscar registro único
     */
    bool search(const KeyType& key, RecordReference& record_ref) {
        search_operations++;
        
        if (!root) return false;
        
        return searchHelper(root, key, record_ref);
    }
    
    /**
     * @brief Búsqueda con flujo educativo detallado
     */
    bool searchWithFlow(const KeyType& key, RecordReference& record_ref) {
        search_operations++;
        
        std::cout << "\n🌳 FLUJO DE BÚSQUEDA B+ TREE:" << std::endl;
        std::cout << "1️⃣ Clave buscada: " << key << std::endl;
        std::cout << "2️⃣ Altura del árbol: " << height << std::endl;
        std::cout << "3️⃣ Orden del árbol: " << order << std::endl;
        
        if (!root) {
            std::cout << "❌ Árbol vacío" << std::endl;
            return false;
        }
        
        return searchHelperWithFlow(root, key, record_ref, 0);
    }
    
    /**
     * @brief Búsqueda por rango con flujo educativo
     */
    std::vector<RecordReference> rangeSearchWithFlow(const KeyType& start_key, 
                                                     const KeyType& end_key) {
        range_operations++;
        
        std::cout << "\n🌳 FLUJO DE BÚSQUEDA POR RANGO B+ TREE:" << std::endl;
        std::cout << "1️⃣ Rango: [" << start_key << ", " << end_key << "]" << std::endl;
        std::cout << "2️⃣ Localizando nodo hoja inicial..." << std::endl;
        
        std::vector<RecordReference> results;
        
        if (!root) {
            std::cout << "❌ Árbol vacío" << std::endl;
            return results;
        }
        
        // Encontrar el primer nodo hoja que puede contener start_key
        LeafNode<KeyType>* leaf = findLeafNode(start_key);
        
        if (!leaf) {
            std::cout << "❌ No se encontró nodo hoja inicial" << std::endl;
            return results;
        }
        
        std::cout << "3️⃣ Nodo hoja inicial localizado" << std::endl;
        std::cout << "4️⃣ Recorriendo hojas enlazadas..." << std::endl;
        
        int leaf_count = 0;
        int total_found = 0;
        
        // Iterar a través de nodos hoja hasta superar end_key
        while (leaf) {
            leaf_count++;
            int found_in_leaf = 0;
            
            leaf->rangeSearch(start_key, end_key, results, found_in_leaf);
            total_found += found_in_leaf;
            
            std::cout << "   Hoja #" << leaf_count << ": " << found_in_leaf << " registros encontrados" << std::endl;
            
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
        
        std::cout << "5️⃣ ✅ Búsqueda completada:" << std::endl;
        std::cout << "   • Hojas examinadas: " << leaf_count << std::endl;
        std::cout << "   • Registros encontrados: " << total_found << std::endl;
        
        return results;
    }
    
    /**
     * @brief Búsqueda por rango estándar
     */
    std::vector<RecordReference> rangeSearch(const KeyType& start_key, const KeyType& end_key) {
        range_operations++;
        
        std::vector<RecordReference> results;
        
        if (!root) return results;
        
        LeafNode<KeyType>* leaf = findLeafNode(start_key);
        
        while (leaf) {
            int found_in_leaf = 0;
            leaf->rangeSearch(start_key, end_key, results, found_in_leaf);
            
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
    
    /**
     * @brief Eliminar registro
     */
    bool remove(const KeyType& key) {
        if (!root) return false;
        
        bool success = root->remove(key);
        if (success) {
            total_records--;
        }
        
        return success;
    }
    
    // ============================================================================
    // ESTADÍSTICAS Y VISUALIZACIÓN
    // ============================================================================
    
    /**
     * @brief Muestra estadísticas detalladas
     */
    void displayStatistics() const {
        std::cout << "\n📊 ESTADÍSTICAS B+ TREE 📊" << std::endl;
        std::cout << "Registros totales: " << total_records << std::endl;
        std::cout << "Operaciones de inserción: " << insert_operations << std::endl;
        std::cout << "Operaciones de búsqueda: " << search_operations << std::endl;
        std::cout << "Operaciones de rango: " << range_operations << std::endl;
        std::cout << "Divisiones de nodo: " << split_operations << std::endl;
        std::cout << "Orden del árbol: " << order << std::endl;
        std::cout << "Altura: " << height << std::endl;
        
        if (search_operations > 0) {
            std::cout << "Promedio de comparaciones por búsqueda: " << (double)height << std::endl;
        }
    }
    
    /**
     * @brief Muestra estructura del árbol
     */
    void displayTree() const {
        std::cout << "\n🌳 ESTRUCTURA DEL B+ TREE:" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        
        if (root) {
            displayNodeRecursive(root, 0);
        } else {
            std::cout << "Árbol vacío" << std::endl;
        }
        
        std::cout << "\n📋 RESUMEN:" << std::endl;
        std::cout << "Altura: " << height << std::endl;
        std::cout << "Orden: " << order << std::endl;
        std::cout << "Nodos hoja: " << countLeafNodes() << std::endl;
        std::cout << "Nodos internos: " << countInternalNodes() << std::endl;
    }
    
    // ============================================================================
    // GETTERS PARA PERSISTENCIA
    // ============================================================================
    
    int getOrder() const { return order; }
    int getHeight() const { return height; }
    size_t getTotalRecords() const { return total_records; }
    size_t getSearchOperations() const { return search_operations; }
    size_t getRangeOperations() const { return range_operations; }
    size_t getSplitOperations() const { return split_operations; }
    bool isEmpty() const { return total_records == 0; }
    
    /**
     * @brief Obtiene todas las claves en orden (para exportación)
     */
    std::vector<KeyType> getAllKeysInOrder() const {
        std::vector<KeyType> keys;
        
        LeafNode<KeyType>* leaf = findFirstLeaf();
        while (leaf) {
            auto leaf_keys = leaf->getKeys();
            keys.insert(keys.end(), leaf_keys.begin(), leaf_keys.end());
            leaf = leaf->getNext();
        }
        
        return keys;
    }
    
    /**
     * @brief Información de distribución de nodos
     */
    std::string getNodeDistribution() const {
        std::stringstream ss;
        
        int leaf_count = countLeafNodes();
        int internal_count = countInternalNodes();
        
        ss << "Tree Order: " << order << "\n";
        ss << "Tree Height: " << height << "\n";
        ss << "Total Records: " << total_records << "\n";
        ss << "Leaf Nodes: " << leaf_count << "\n";
        ss << "Internal Nodes: " << internal_count << "\n";
        ss << "Average Records per Leaf: ";
        
        if (leaf_count > 0) {
            ss << (double)total_records / leaf_count << "\n";
        } else {
            ss << "0\n";
        }
        
        return ss.str();
    }
    
    // ============================================================================
    // BÚSQUEDAS ESPECIALES PARA INTEGRACIÓN CON SGBD
    // ============================================================================
    
    /**
     * @brief Búsqueda con acceso al disco integrado
     */
    bool searchWithDiskAccess(const KeyType& key, RecordReference& record_ref,
                              std::function<bool(const RecordReference&)> disk_accessor) {
        
        std::cout << "\n🎯 BÚSQUEDA INTEGRADA B+ TREE → DISCO:" << std::endl;
        std::cout << "=" << std::string(40, '=') << std::endl;
        
        // Paso 1: Búsqueda en índice
        if (!searchWithFlow(key, record_ref)) {
            std::cout << "❌ Clave no encontrada en B+ Tree" << std::endl;
            return false;
        }
        
        // Paso 2: Acceso al disco
        std::cout << "6️⃣ Accediendo al disco..." << std::endl;
        std::cout << "   Page ID: " << record_ref.toPageId() << std::endl;
        std::cout << "   Physical Address: " << record_ref.getPhysicalAddress() << std::endl;
        std::cout << "   Slot ID: " << record_ref.getSlotId() << std::endl;
        
        bool disk_success = disk_accessor(record_ref);
        
        if (disk_success) {
            std::cout << "7️⃣ ✅ Registro recuperado exitosamente desde disco" << std::endl;
        } else {
            std::cout << "7️⃣ ❌ Error accediendo al registro en disco" << std::endl;
        }
        
        return disk_success;
    }

private:
    // ============================================================================
    // MÉTODOS AUXILIARES PRIVADOS
    // ============================================================================
    
    bool insertHelper(BPlusNode<KeyType>* node, const KeyType& key, const RecordReference& record_ref) {
        if (node->insert(key, record_ref)) {
            return true;
        }
        
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
    
    bool searchHelperWithFlow(BPlusNode<KeyType>* node, const KeyType& key, 
                              RecordReference& record_ref, int level) {
        
        std::cout << "4️⃣ Nivel " << level << ": ";
        
        if (node->isLeaf()) {
            std::cout << "Nodo HOJA - Búsqueda final" << std::endl;
            bool found = node->search(key, record_ref);
            if (found) {
                std::cout << "5️⃣ ✅ Clave encontrada en nodo hoja!" << std::endl;
                std::cout << "   RecordReference: " << record_ref << std::endl;
            } else {
                std::cout << "5️⃣ ❌ Clave no encontrada en nodo hoja" << std::endl;
            }
            return found;
        } else {
            std::cout << "Nodo INTERNO - Navegando hacia abajo" << std::endl;
            
            // Para nodos internos, encontrar el hijo apropiado
            auto internal = static_cast<InternalNode<KeyType>*>(node);
            auto child = internal->findChild(key);
            
            if (child) {
                return searchHelperWithFlow(child, key, record_ref, level + 1);
            } else {
                std::cout << "❌ No se encontró hijo apropiado en nodo interno" << std::endl;
                return false;
            }
        }
    }
    
    void splitRoot(const KeyType& key, const RecordReference& record_ref) {
        auto old_root = root;
        
        if (old_root->isLeaf()) {
            auto leaf = static_cast<LeafNode<KeyType>*>(old_root);
            auto new_leaf = leaf->split();
            
            if (new_leaf) {
                auto new_root = new InternalNode<KeyType>(order);
                new_root->getKeys().push_back(new_leaf->getKeys()[0]);
                new_root->addChild(leaf);
                new_root->addChild(new_leaf);
                
                root = new_root;
                height++;
                
                if (KeyComparator<KeyType>::compare(key, new_leaf->getKeys()[0]) < 0) {
                    leaf->insert(key, record_ref);
                } else {
                    new_leaf->insert(key, record_ref);
                }
                
                std::cout << "🌿 Raiz hoja dividida (nueva altura: " << height << ")" << std::endl;
            }
        } else {
            auto internal = static_cast<InternalNode<KeyType>*>(old_root);
            internal->insert(key, record_ref);
        }
    }
    
    LeafNode<KeyType>* findLeafNode(const KeyType& key) const {
        if (!root) return nullptr;
        
        BPlusNode<KeyType>* current = root;
        while (!current->isLeaf()) {
            auto internal = static_cast<InternalNode<KeyType>*>(current);
            current = internal->findChild(key);
            if (!current) return nullptr;
        }
        
        return static_cast<LeafNode<KeyType>*>(current);
    }
    
    LeafNode<KeyType>* findFirstLeaf() const {
        if (!root) return nullptr;
        
        BPlusNode<KeyType>* current = root;
        while (!current->isLeaf()) {
            auto internal = static_cast<InternalNode<KeyType>*>(current);
            if (internal->getChildren().empty()) return nullptr;
            current = internal->getChildren()[0];
        }
        
        return static_cast<LeafNode<KeyType>*>(current);
    }
    
    void clearTree(BPlusNode<KeyType>* node) {
        if (node) {
            if (!node->isLeaf()) {
                auto internal = static_cast<InternalNode<KeyType>*>(node);
                for (auto child : internal->getChildren()) {
                    clearTree(child);
                }
            }
            delete node;
        }
    }
    
    void displayNodeRecursive(BPlusNode<KeyType>* node, int level) const {
        if (!node) return;
        
        std::string indent(level * 2, ' ');
        
        if (node->isLeaf()) {
            std::cout << indent << "LEAF: ";
            auto keys = node->getKeys();
            for (size_t i = 0; i < keys.size(); ++i) {
                std::cout << keys[i];
                if (i < keys.size() - 1) std::cout << ", ";
            }
            std::cout << " (" << keys.size() << " keys)" << std::endl;
        } else {
            std::cout << indent << "INTERNAL: ";
            auto keys = node->getKeys();
            for (size_t i = 0; i < keys.size(); ++i) {
                std::cout << keys[i];
                if (i < keys.size() - 1) std::cout << ", ";
            }
            std::cout << " (" << keys.size() << " keys)" << std::endl;
            
            auto internal = static_cast<InternalNode<KeyType>*>(node);
            for (auto child : internal->getChildren()) {
                displayNodeRecursive(child, level + 1);
            }
        }
    }
    
    int countLeafNodes() const {
        return countLeafNodesRecursive(root);
    }
    
    int countInternalNodes() const {
        return countInternalNodesRecursive(root);
    }
    
    int countLeafNodesRecursive(BPlusNode<KeyType>* node) const {
        if (!node) return 0;
        
        if (node->isLeaf()) {
            return 1;
        } else {
            int count = 0;
            auto internal = static_cast<InternalNode<KeyType>*>(node);
            for (auto child : internal->getChildren()) {
                count += countLeafNodesRecursive(child);
            }
            return count;
        }
    }
    
    int countInternalNodesRecursive(BPlusNode<KeyType>* node) const {
        if (!node) return 0;
        
        if (node->isLeaf()) {
            return 0;
        } else {
            int count = 1;
            auto internal = static_cast<InternalNode<KeyType>*>(node);
            for (auto child : internal->getChildren()) {
                count += countInternalNodesRecursive(child);
            }
            return count;
        }
    }
};

#endif // BPLUS_TREE_H