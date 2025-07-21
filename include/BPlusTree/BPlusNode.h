#ifndef BPLUS_NODE_H
#define BPLUS_NODE_H

#include <vector>
#include <memory>
#include "../Record.h"
#include "../RecordReference.h"
#include "KeyComparator.h"

template<typename KeyType>
class BPlusNode {
protected:
    int order;                                    // Orden del árbol (max keys = order-1)
    std::vector<KeyType> keys;
    BPlusNode* parent;

public:
    BPlusNode(int order) : order(order), parent(nullptr) {}
    virtual ~BPlusNode() = default;
    
    // Getters básicos
    virtual bool isLeaf() const = 0;
    int getOrder() const { return order; }
    int getKeyCount() const { return keys.size(); }
    bool isFull() const { return keys.size() >= static_cast<size_t>(order - 1); }
    bool isUnderflow() const { return keys.size() < static_cast<size_t>((order - 1) / 2); }
    
    // Acceso a claves
    const std::vector<KeyType>& getKeys() const { return keys; }
    std::vector<KeyType>& getKeys() { return keys; }
    const KeyType& getKey(int index) const { return keys[index]; }
    
    // Manejo del padre
    BPlusNode* getParent() const { return parent; }
    void setParent(BPlusNode* p) { parent = p; }
    
    // Métodos virtuales puros
    virtual bool insert(const KeyType& key, const RecordReference& record_ref) = 0;
    virtual bool search(const KeyType& key, RecordReference& record_ref) = 0;
    virtual bool remove(const KeyType& key) = 0;
    virtual void display(int level = 0) const = 0;
    
    // Búsqueda de posición para inserción
    int findInsertPosition(const KeyType& key) const {
        int pos = 0;
        while (pos < static_cast<int>(keys.size()) && KeyComparator<KeyType>::compare(keys[pos], key) < 0) {
            pos++;
        }
        return pos;
    }
    
    // Búsqueda de clave
    int findKey(const KeyType& key) const {
        for (size_t i = 0; i < keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(keys[i], key) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};

#endif