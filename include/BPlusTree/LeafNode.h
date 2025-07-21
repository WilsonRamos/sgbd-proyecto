#ifndef LEAF_NODE_H
#define LEAF_NODE_H

#include "BPlusNode.h"
#include "../RecordReference.h"
#include <iostream>

template<typename KeyType>
class LeafNode : public BPlusNode<KeyType> {
private:
    std::vector<RecordReference> record_refs; // Referencias a registros en disco
    LeafNode* next;                          // Puntero al siguiente nodo hoja
    LeafNode* prev;                          // Puntero al nodo hoja anterior

public:
    LeafNode(int order) : BPlusNode<KeyType>(order), next(nullptr), prev(nullptr) {}
    
    // Implementar método virtual
    bool isLeaf() const override { return true; }
    
    // Getters para navegación
    LeafNode* getNext() const { return next; }
    LeafNode* getPrev() const { return prev; }
    void setNext(LeafNode* n) { next = n; }
    void setPrev(LeafNode* p) { prev = p; }
    
    // Implementación de métodos virtuales
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
    
    bool search(const KeyType& key, RecordReference& record_ref) override {
        int index = this->findKey(key);
        if (index != -1) {
            record_ref = record_refs[index];
            return true;
        }
        return false;
    }
    
    bool remove(const KeyType& key) override {
        int index = this->findKey(key);
        if (index != -1) {
            this->keys.erase(this->keys.begin() + index);
            record_refs.erase(record_refs.begin() + index);
            return true;
        }
        return false;
    }
    
    // Dividir nodo hoja cuando está lleno
    LeafNode* split() {
        int mid = this->order / 2;
        auto new_leaf = new LeafNode<KeyType>(this->order);
        
        // Mover la mitad derecha al nuevo nodo
        new_leaf->keys.assign(this->keys.begin() + mid, this->keys.end());
        new_leaf->record_refs.assign(record_refs.begin() + mid, record_refs.end());
        
        // Mantener la mitad izquierda
        this->keys.resize(mid);
        record_refs.resize(mid);
        
        // Actualizar enlaces de lista
        new_leaf->next = this->next;
        new_leaf->prev = this;
        if (this->next) {
            this->next->prev = new_leaf;
        }
        this->next = new_leaf;
        
        return new_leaf;
    }
    
    // Búsqueda por rango en nodos hoja
    void rangeSearch(const KeyType& start_key, const KeyType& end_key, 
                    std::vector<RecordReference>& results) const {
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (KeyComparator<KeyType>::compare(this->keys[i], start_key) >= 0 &&
                KeyComparator<KeyType>::compare(this->keys[i], end_key) <= 0) {
                results.push_back(record_refs[i]);
            }
        }
    }
    
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "HOJA [";
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << this->keys[i] << "*";
        }
        std::cout << "]";
        if (next) std::cout << " -> próximo";
        std::cout << std::endl;
    }
    
    const std::vector<RecordReference>& getRecordReferences() const { return record_refs; }
};

#endif
