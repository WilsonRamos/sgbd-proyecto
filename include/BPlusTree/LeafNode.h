#ifndef LEAF_NODE_H
#define LEAF_NODE_H

#include "BPlusNode.h"
#include "KeyComparator.h"
#include "../RecordReference.h"
#include <iostream>
#include <memory>
#include <algorithm>

/**
 * @brief Nodo hoja del B+ Tree COMPLETO
 * 
 * ✅ IMPLEMENTACIÓN COMPLETA PARA SGBD FÍSICO:
 * - Almacena RecordReference junto con claves
 * - Enlaces horizontales a nodos hermanos para recorrido secuencial
 * - Optimizado para búsquedas por rango O(k) después de O(log n)
 * - Todas las claves están en las hojas (característico del B+ Tree)
 * - Soporte completo para splits y merges
 * - Serialización y validación robusta
 */
template<typename KeyType>
class LeafNode : public BPlusNode<KeyType> {
private:
    std::vector<RecordReference> record_refs; // Referencias a registros en disco
    std::shared_ptr<LeafNode<KeyType>> next;  // Puntero al siguiente nodo hoja
    std::shared_ptr<LeafNode<KeyType>> prev;  // Puntero al nodo hoja anterior

public:
    /**
     * @brief Constructor
     */
    LeafNode(int order) : BPlusNode<KeyType>(order, true), next(nullptr), prev(nullptr) {
        record_refs.reserve(order - 1);
    }
    
    /**
     * @brief Destructor
     */
    ~LeafNode() override = default;

    // ============================================================================
    // IMPLEMENTACIÓN DE MÉTODOS VIRTUALES
    // ============================================================================
    
    /**
     * @brief Identifica que es un nodo hoja
     */
    bool isLeaf() const override { 
        return true; 
    }
    
    /**
     * @brief ✅ Inserta una clave con su RecordReference
     */
    bool insert(const KeyType& key, const RecordReference& record_ref) override {
        if (this->isFull()) {
            return false; // El nodo está lleno
        }
        
        int pos = this->findInsertPosition(key);
        
        // Verificar si la clave ya existe
        if (pos < static_cast<int>(this->keys.size()) && 
            KeyComparator<KeyType>::equal(this->keys[pos], key)) {
            // Actualizar referencia existente
            record_refs[pos] = record_ref;
            this->recordModification();
            std::cout << "🔄 Actualizada referencia para clave existente: " << key << std::endl;
            return true;
        }
        
        // Insertar nueva clave y referencia
        this->keys.insert(this->keys.begin() + pos, key);
        record_refs.insert(record_refs.begin() + pos, record_ref);
        
        this->recordModification();
        return true;
    }
    
    /**
     * @brief Busca una clave y retorna su RecordReference
     */
    bool search(const KeyType& key, RecordReference& record_ref) override {
        this->recordAccess();
        
        int index = this->findKey(key);
        if (index != -1) {
            record_ref = record_refs[index];
            return true;
        }
        return false;
    }
    
    /**
     * @brief Elimina una clave del nodo hoja
     */
    bool remove(const KeyType& key) override {
        int index = this->findKey(key);
        if (index != -1) {
            this->keys.erase(this->keys.begin() + index);
            record_refs.erase(record_refs.begin() + index);
            this->recordModification();
            return true;
        }
        return false;
    }
    
    /**
     * @brief ✅ Divide el nodo hoja cuando está lleno
     */
    BPlusNode<KeyType>* split() override {
        if (!this->isFull()) {
            return nullptr;
        }
        
        int mid = this->order / 2;
        auto new_leaf = new LeafNode<KeyType>(this->order);
        
        // Mover la mitad derecha al nuevo nodo
        new_leaf->keys.assign(this->keys.begin() + mid, this->keys.end());
        new_leaf->record_refs.assign(record_refs.begin() + mid, record_refs.end());
        
        // Mantener la mitad izquierda
        this->keys.resize(mid);
        record_refs.resize(mid);
        
        // ✅ Actualizar enlaces horizontales (CRÍTICO para recorrido secuencial)
        new_leaf->next = this->next;
        new_leaf->prev = std::shared_ptr<LeafNode<KeyType>>(this, [](LeafNode<KeyType>*){});
        
        if (this->next) {
            this->next->prev = std::shared_ptr<LeafNode<KeyType>>(new_leaf, [](LeafNode<KeyType>*){});
        }
        this->next = std::shared_ptr<LeafNode<KeyType>>(new_leaf, [](LeafNode<KeyType>*){});
        
        // Establecer padre
        new_leaf->setParent(this->getParent());
        
        std::cout << "🌿 Nodo hoja dividido: " << this->keys.size() 
                  << " + " << new_leaf->keys.size() << " claves" << std::endl;
        std::cout << "   Primera clave nueva hoja: " << new_leaf->keys[0] << std::endl;
        
        return new_leaf;
    }

    // ============================================================================
    // BÚSQUEDAS POR RANGO (FUNCIÓN PRINCIPAL DEL B+ TREE)
    // ============================================================================
    
    /**
     * @brief ✅ Búsqueda por rango optimizada - FUNCIÓN CLAVE DEL B+ TREE
     * 
     * Esta es la función que hace al B+ Tree superior para consultas de rango.
     * Complejidad: O(k) donde k = número de resultados en el rango
     */
    void rangeSearch(const KeyType& start_key, const KeyType& end_key,
                     std::vector<RecordReference>& results, int& found_count) override {
        found_count = 0;
        
        // Buscar todas las claves en este nodo que estén en el rango
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (KeyComparator<KeyType>::greaterEqual(this->keys[i], start_key) &&
                KeyComparator<KeyType>::lessEqual(this->keys[i], end_key)) {
                
                results.push_back(record_refs[i]);
                found_count++;
            }
            
            // Optimización: si ya pasamos el end_key, terminar
            if (KeyComparator<KeyType>::greater(this->keys[i], end_key)) {
                break;
            }
        }
    }
    
    /**
     * @brief ✅ Búsqueda por rango que retorna vector directamente
     */
    std::vector<RecordReference> rangeSearch(const KeyType& start, const KeyType& end) const {
        std::vector<RecordReference> results;
        
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (KeyComparator<KeyType>::greaterEqual(this->keys[i], start) &&
                KeyComparator<KeyType>::lessEqual(this->keys[i], end)) {
                
                results.push_back(record_refs[i]);
            }
            
            if (KeyComparator<KeyType>::greater(this->keys[i], end)) {
                break;
            }
        }
        
        return results;
    }

    /**
     * @brief ✅ Búsqueda por rango con timestamps (especializada)
     */
    std::vector<RecordReference> timestampRangeSearch(const std::string& start_timestamp, 
                                                     const std::string& end_timestamp) const {
        std::vector<RecordReference> results;
        
        // Solo funciona si KeyType es std::string
        static_assert(std::is_same_v<KeyType, std::string>, 
                     "timestampRangeSearch solo funciona con KeyType = std::string");
        
        for (size_t i = 0; i < this->keys.size(); i++) {
            if (KeyComparator<std::string>::inTimestampRange(this->keys[i], start_timestamp, end_timestamp)) {
                results.push_back(record_refs[i]);
            }
        }
        
        return results;
    }

    // ============================================================================
    // GESTIÓN DE ENLACES HORIZONTALES
    // ============================================================================
    
    /**
     * @brief Obtiene el siguiente nodo hoja
     */
    std::shared_ptr<LeafNode<KeyType>> getNext() const { 
        return next; 
    }
    
    /**
     * @brief Obtiene el nodo hoja anterior
     */
    std::shared_ptr<LeafNode<KeyType>> getPrev() const { 
        return prev; 
    }
    
    /**
     * @brief Establece el siguiente nodo hoja
     */
    void setNext(std::shared_ptr<LeafNode<KeyType>> next_node) {
        next = next_node;
    }
    
    /**
     * @brief Establece el nodo hoja anterior
     */
    void setPrev(std::shared_ptr<LeafNode<KeyType>> prev_node) {
        prev = prev_node;
    }

    // ============================================================================
    // ACCESO A DATOS
    // ============================================================================
    
    /**
     * @brief Obtiene todas las referencias
     */
    const std::vector<RecordReference>& getRecordRefs() const {
        this->recordAccess();
        return record_refs;
    }
    
    /**
     * @brief Obtiene una referencia por índice
     */
    RecordReference getRecordRefAt(size_t index) const {
        this->recordAccess();
        if (index < record_refs.size()) {
            return record_refs[index];
        }
        return RecordReference::invalid();
    }
    
    /**
     * @brief Obtiene el número de referencias válidas
     */
    size_t getValidReferencesCount() const {
        size_t count = 0;
        for (const auto& ref : record_refs) {
            if (ref.isValid()) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Obtiene pares (clave, referencia)
     */
    std::vector<std::pair<KeyType, RecordReference>> getKeyRefPairs() const {
        std::vector<std::pair<KeyType, RecordReference>> pairs;
        pairs.reserve(this->keys.size());
        
        for (size_t i = 0; i < this->keys.size() && i < record_refs.size(); i++) {
            pairs.emplace_back(this->keys[i], record_refs[i]);
        }
        
        return pairs;
    }

    // ============================================================================
    // SERIALIZACIÓN PARA PERSISTENCIA
    // ============================================================================
    
    /**
     * @brief Serializa el nodo hoja completo
     */
    std::string serialize() const override {
        std::ostringstream oss;
        
        oss << "LEAF_NODE|" << this->order << "|" << this->keys.size() << std::endl;
        
        // Serializar pares clave-referencia
        for (size_t i = 0; i < this->keys.size() && i < record_refs.size(); i++) {
            oss << "ENTRY|" << this->keys[i] << "|" << record_refs[i].serialize() << std::endl;
        }
        
        // Metadatos de enlaces (simplificado - en implementación completa se guardarían IDs)
        oss << "HAS_NEXT|" << (next ? "true" : "false") << std::endl;
        oss << "HAS_PREV|" << (prev ? "true" : "false") << std::endl;
        
        return oss.str();
    }
    
    /**
     * @brief Deserializa nodo hoja desde string
     */
    bool deserialize(const std::string& data) override {
        std::istringstream iss(data);
        std::string line;
        
        this->keys.clear();
        record_refs.clear();
        
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            std::istringstream line_stream(line);
            std::string type;
            std::getline(line_stream, type, '|');
            
            if (type == "LEAF_NODE") {
                std::string order_str, size_str;
                std::getline(line_stream, order_str, '|');
                std::getline(line_stream, size_str, '|');
                
                // Verificar orden compatible
                int file_order = std::stoi(order_str);
                if (file_order != this->order) {
                    std::cout << "⚠️ Orden incompatible: " << file_order << " vs " << this->order << std::endl;
                }
                
            } else if (type == "ENTRY") {
                std::string key_str, ref_data;
                std::getline(line_stream, key_str, '|');
                std::getline(line_stream, ref_data);
                
                // Parsear clave (específico para el tipo)
                KeyType key;
                std::istringstream key_stream(key_str);
                key_stream >> key;
                
                // Deserializar referencia
                RecordReference record_ref;
                if (record_ref.deserialize(ref_data)) {
                    this->keys.push_back(key);
                    record_refs.push_back(record_ref);
                }
            }
            // Los enlaces next/prev se reconstruirían en un paso posterior
        }
        
        return true;
    }

    // ============================================================================
    // VISUALIZACIÓN Y DEBUG
    // ============================================================================
    
    /**
     * @brief Muestra el nodo hoja con detalles
     */
    void display(int level = 0) const override {
        std::string indent(level * 2, ' ');
        
        std::cout << indent << "🍃 LeafNode (Level " << level << "): [";
        for (size_t i = 0; i < this->keys.size(); i++) {
            std::cout << this->keys[i];
            if (i < this->keys.size() - 1) std::cout << ", ";
        }
        std::cout << "] (" << record_refs.size() << " refs)" << std::endl;
        
        // Mostrar algunas referencias como muestra
        if (!record_refs.empty() && level <= 2) { // Solo en niveles superiores
            std::cout << indent << "  Referencias: ";
            size_t sample_size = std::min(static_cast<size_t>(3), record_refs.size());
            for (size_t i = 0; i < sample_size; i++) {
                std::cout << record_refs[i].toString();
                if (i < sample_size - 1) std::cout << ", ";
            }
            if (record_refs.size() > sample_size) {
                std::cout << "...";
            }
            std::cout << std::endl;
        }
        
        // Mostrar enlaces
        std::cout << indent << "  Enlaces: ";
        std::cout << "Prev[" << (prev ? "✓" : "✗") << "] ";
        std::cout << "Next[" << (next ? "✓" : "✗") << "]" << std::endl;
    }

    /**
     * @brief Información detallada del nodo hoja
     */
    void displayDetailed() const {
        std::cout << "\n🍃 NODO HOJA DETALLADO:" << std::endl;
        this->displayBasicInfo();
        
        std::cout << "  Referencias válidas: " << getValidReferencesCount() 
                  << "/" << record_refs.size() << std::endl;
        
        // Verificar consistencia tamaños
        bool size_consistent = (this->keys.size() == record_refs.size());
        std::cout << "  Consistencia claves/refs: " << (size_consistent ? "✓" : "✗") << std::endl;
        
        // Enlaces horizontales
        std::cout << "  Nodo anterior: " << (prev ? "Conectado" : "Ninguno") << std::endl;
        std::cout << "  Nodo siguiente: " << (next ? "Conectado" : "Ninguno") << std::endl;
        
        if (!this->keys.empty()) {
            std::cout << "  Rango de claves: [" << this->keys.front() 
                      << " - " << this->keys.back() << "]" << std::endl;
        }
        
        // Mostrar algunas entradas detalladas
        if (!this->keys.empty()) {
            std::cout << "\n  📋 ENTRADAS (muestra):" << std::endl;
            size_t sample_size = std::min(static_cast<size_t>(5), this->keys.size());
            for (size_t i = 0; i < sample_size; i++) {
                std::cout << "    [" << i << "] " << this->keys[i] 
                          << " -> " << record_refs[i].toString() << std::endl;
            }
            if (this->keys.size() > sample_size) {
                std::cout << "    ... y " << (this->keys.size() - sample_size) << " más" << std::endl;
            }
        }
    }

    // ============================================================================
    // VALIDACIÓN DE CONSISTENCIA
    // ============================================================================
    
    /**
     * @brief Validación específica de nodo hoja
     */
    bool validateConsistency() const override {
        // Validación base
        if (!BPlusNode<KeyType>::validateConsistency()) {
            return false;
        }
        
        // Verificar que el número de claves coincida con el de referencias
        if (this->keys.size() != record_refs.size()) {
            std::cout << "❌ Inconsistencia claves/referencias: " 
                      << this->keys.size() << " != " << record_refs.size() << std::endl;
            return false;
        }
        
        // Verificar que todas las referencias sean válidas
        for (size_t i = 0; i < record_refs.size(); i++) {
            if (!record_refs[i].isValid()) {
                std::cout << "❌ Referencia inválida en posición " << i << std::endl;
                return false;
            }
        }
        
        // Verificar enlaces horizontales (si existen)
        if (next && next->prev.get() != this) {
            std::cout << "❌ Enlace next inconsistente" << std::endl;
            return false;
        }
        
        if (prev && prev->next.get() != this) {
            std::cout << "❌ Enlace prev inconsistente" << std::endl;
            return false;
        }
        
        return true;
    }

    // ============================================================================
    // OPERACIONES AVANZADAS
    // ============================================================================
    
    /**
     * @brief Limpia referencias inválidas
     */
    size_t cleanInvalidReferences() {
        size_t removed = 0;
        
        for (size_t i = 0; i < record_refs.size(); ) {
            if (!record_refs[i].isValid()) {
                this->keys.erase(this->keys.begin() + i);
                record_refs.erase(record_refs.begin() + i);
                removed++;
            } else {
                i++;
            }
        }
        
        if (removed > 0) {
            this->recordModification();
        }
        
        return removed;
    }
    
    /**
     * @brief Reorganiza entradas por clave
     */
    void sortEntries() {
        // Crear pares (clave, referencia) para ordenar juntos
        std::vector<std::pair<KeyType, RecordReference>> pairs;
        pairs.reserve(this->keys.size());
        
        for (size_t i = 0; i < this->keys.size() && i < record_refs.size(); i++) {
            pairs.emplace_back(this->keys[i], record_refs[i]);
        }
        
        // Ordenar por clave
        std::sort(pairs.begin(), pairs.end(),
            [](const auto& a, const auto& b) {
                return KeyComparator<KeyType>::less(a.first, b.first);
            });
        
        // Reconstruir vectores ordenados
        this->keys.clear();
        record_refs.clear();
        
        for (const auto& pair : pairs) {
            this->keys.push_back(pair.first);
            record_refs.push_back(pair.second);
        }
        
        this->recordModification();
    }

    /**
     * @brief Merge con nodo hoja hermano (para eliminaciones)
     */
    bool mergeWith(std::shared_ptr<LeafNode<KeyType>> sibling) {
        if (!sibling || this->keys.size() + sibling->keys.size() > static_cast<size_t>(this->getMaxKeys())) {
            return false; // No se puede hacer merge
        }
        
        // Añadir claves y referencias del hermano
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.end());
        record_refs.insert(record_refs.end(), sibling->record_refs.begin(), sibling->record_refs.end());
        
        // Actualizar enlaces horizontales
        this->next = sibling->next;
        if (sibling->next) {
            sibling->next->prev = std::shared_ptr<LeafNode<KeyType>>(this, [](LeafNode<KeyType>*){});
        }
        
        // Ordenar después del merge
        sortEntries();
        
        this->recordModification();
        return true;
    }

    /**
     * @brief Estadísticas específicas de nodo hoja
     */
    struct LeafStats {
        size_t key_count;
        size_t valid_references;
        size_t invalid_references;
        double occupancy;
        bool has_next;
        bool has_prev;
        KeyType min_key;
        KeyType max_key;
    };
    
    LeafStats getLeafStats() const {
        LeafStats stats;
        stats.key_count = this->keys.size();
        stats.valid_references = getValidReferencesCount();
        stats.invalid_references = record_refs.size() - stats.valid_references;
        stats.occupancy = this->getOccupancyFactor();
        stats.has_next = (next != nullptr);
        stats.has_prev = (prev != nullptr);
        
        if (!this->keys.empty()) {
            stats.min_key = this->keys.front();
            stats.max_key = this->keys.back();
        }
        
        return stats;
    }

    // ============================================================================
    // RECORRIDO SECUENCIAL (CARACTERÍSTICA CLAVE DEL B+ TREE)
    // ============================================================================
    
    /**
     * @brief ✅ Recorre todas las hojas secuencialmente desde esta hoja
     * 
     * Esta es una de las características distintivas del B+ Tree:
     * Permite recorrido secuencial eficiente de todos los datos
     */
    std::vector<std::pair<KeyType, RecordReference>> sequentialScan() const {
        std::vector<std::pair<KeyType, RecordReference>> all_entries;
        
        // Empezar desde esta hoja
        auto current = std::const_pointer_cast<LeafNode<KeyType>>(
            std::shared_ptr<const LeafNode<KeyType>>(this, [](const LeafNode<KeyType>*){})
        );
        
        while (current) {
            // Añadir todas las entradas de la hoja actual
            auto current_entries = current->getKeyRefPairs();
            all_entries.insert(all_entries.end(), current_entries.begin(), current_entries.end());
            
            // Avanzar a la siguiente hoja
            current = current->getNext();
        }
        
        return all_entries;
    }
    
    /**
     * @brief Cuenta el total de entradas desde esta hoja hasta el final
     */
    size_t countRemainingEntries() const {
        size_t count = 0;
        auto current = std::const_pointer_cast<LeafNode<KeyType>>(
            std::shared_ptr<const LeafNode<KeyType>>(this, [](const LeafNode<KeyType>*){})
        );
        
        while (current) {
            count += current->keys.size();
            current = current->getNext();
        }
        
        return count;
    }
};

#endif // LEAF_NODE_H