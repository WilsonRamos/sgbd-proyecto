#ifndef INTERNAL_NODE_H
#define INTERNAL_NODE_H

#include "BPlusNode.h"
#include "KeyComparator.h"
#include "../RecordReference.h"
#include <iostream>
#include <vector>

template<typename KeyType>
class InternalNode : public BPlusNode<KeyType> {
private:
    std::vector<BPlusNode<KeyType>*> children;

public:
    InternalNode(int order) : BPlusNode<KeyType>(order) {}
    
    ~InternalNode() {
        for (auto child : children) {
            delete child;
        }
    }
    
    bool isLeaf() const override {
        return false;
    }
    
    bool insert(const KeyType& key, const RecordReference& record_ref) override {
        int child_index = findChildIndex(key);
        
        // Asegurar que tenemos suficientes hijos
        if (child_index >= static_cast<int>(children.size())) {
            return false;
        }
        
        return children[child_index]->insert(key, record_ref);
    }
    
    bool search(const KeyType& key, RecordReference& record_ref) override {
        int child_index = findChildIndex(key);
        
        if (child_index >= static_cast<int>(children.size())) {
            return false;
        }
        
        return children[child_index]->search(key, record_ref);
    }
    
    bool remove(const KeyType& key) override {
        int child_index = findChildIndex(key);
        
        if (child_index >= static_cast<int>(children.size())) {
            return false;
        }
        
        return children[child_index]->remove(key);
    }
    
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "INTERNO [";
        for (size_t i = 0; i < this->keys.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << this->keys[i];
        }
        std::cout << "] (" << children.size() << " hijos)" << std::endl;
        
        // Mostrar hijos recursivamente
        for (auto child : children) {
            if (child) {
                child->display(level + 1);
            }
        }
    }
    
    void addChild(BPlusNode<KeyType>* child) {
        children.push_back(child);
    }
    
    BPlusNode<KeyType>* getChild(int index) {
        if (index >= 0 && index < static_cast<int>(children.size())) {
            return children[index];
        }
        return nullptr;
    }
    
    const std::vector<BPlusNode<KeyType>*>& getChildren() const {
        return children;
    }
    
    std::vector<BPlusNode<KeyType>*>& getChildren() {
        return children;
    }
    
private:
    int findChildIndex(const KeyType& key) {
        int pos = 0;
        while (pos < static_cast<int>(this->keys.size()) && 
               KeyComparator<KeyType>::compare(key, this->keys[pos]) >= 0) {
            pos++;
        }
        return pos;
    }
};

#endif // INTERNAL_NODE_H
