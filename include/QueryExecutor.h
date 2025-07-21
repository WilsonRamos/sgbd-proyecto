#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include <chrono>
#include <cstdint>
#include "BPlusTree/BPlusTree.h"
#include "buffer/BufferManagerClock.h"
#include "DiskManagerExtended.h"
#include "RecordReference.h"
#include "Record.h"
#include "PhysicalAddress.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

/**
 * @brief Ejecutor de Consultas Integrado
 * 
 * Coordina el flujo completo desde la consulta hasta el acceso al registro:
 * 1. Consulta -> B+ Tree (localizar página)
 * 2. B+ Tree -> BufferManager (verificar si página está en memoria)
 * 3. BufferManager -> DiskManager (cargar página si es necesario)
 * 4. Retornar registro completo al usuario
 * 
 * Soporta registros de longitud fija y variable.
 */
class QueryExecutor {
private:
    std::unique_ptr<BPlusTree<std::string>> btree_index;     // Índice B+ Tree
    std::unique_ptr<BufferManagerClock> buffer_manager;      // Gestión de memoria
    std::unique_ptr<DiskManagerExtended> disk_manager;       // Gestión de disco
    
    // Estadísticas de rendimiento
    size_t total_queries;
    size_t cache_hits;
    size_t cache_misses;
    double total_query_time_ms;

public:
    /**
     * @brief Constructor - Inicializa todos los componentes
     */
    QueryExecutor(int btree_order = 4, size_t buffer_pool_size = 16) 
        : total_queries(0)
        , cache_hits(0)
        , cache_misses(0)
        , total_query_time_ms(0.0)
    {
        std::cout << "\n[*] INICIALIZANDO QUERY EXECUTOR INTEGRADO [*]" << std::endl;
        
        // 1. Inicializar DiskManager
        disk_manager = std::make_unique<DiskManagerExtended>();
        std::cout << "[+] DiskManager inicializado" << std::endl;
        
        // 2. Inicializar BufferManager
        buffer_manager = std::make_unique<BufferManagerClock>(buffer_pool_size, disk_manager.get());
        std::cout << "[+] BufferManager inicializado (Clock algorithm)" << std::endl;
        
        // 3. Inicializar B+ Tree
        btree_index = std::make_unique<BPlusTree<std::string>>(btree_order);
        std::cout << "[+] B+ Tree Index inicializado" << std::endl;
        
        std::cout << "\n[+] SISTEMA LISTO PARA CONSULTAS" << std::endl;
        std::cout << "   - Buffer Pool: " << buffer_pool_size << " frames" << std::endl;
        std::cout << "   - B+ Tree Order: " << btree_order << std::endl;
        std::cout << "   - Tipo de clave: String" << std::endl;
        std::cout << "================================================" << std::endl;
    }
    
    /**
     * @brief Operación SELECT - Buscar un registro por clave
     * 
     * Flujo completo:
     * 1. B+ Tree localiza la referencia al registro
     * 2. BufferManager verifica si la página está en memoria
     * 3. Si no está, DiskManager la carga desde disco
     * 4. Se retorna el registro completo
     */
    std::unique_ptr<Record> selectRecord(const std::string& key) {
        std::cout << "\n[*] EJECUTANDO SELECT para clave: '" << key << "'" << std::endl;
        std::cout << "================================================" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // PASO 1: Consultar B+ Tree Index
        std::cout << "[*] PASO 1: Consultando B+ Tree Index..." << std::endl;
        RecordReference record_ref;
        bool found = btree_index->search(key, record_ref);
        
        if (!found) {
            std::cout << "[X] Clave no encontrada en indice" << std::endl;
            cache_misses++;
            total_queries++;
            return nullptr;
        }
        
        // PASO 2: Obtener página del BufferManager
        std::cout << "[*] PASO 2: Consultando BufferManager..." << std::endl;
        std::cout << "     Buscando PageID: " << record_ref.toPageId() << ", Slot: " << record_ref.getSlotId() << std::endl;
        
        auto block = buffer_manager->fetchPage(record_ref.toPageId());
        if (!block) {
            std::cout << "[X] Error obteniendo pagina del buffer" << std::endl;
            cache_misses++;
            total_queries++;
            return nullptr;
        }
        
        // PASO 3: Extraer registro del bloque
        std::cout << "[*] PASO 3: Extrayendo registro del bloque..." << std::endl;
        auto record = extractRecordFromBlock(block, record_ref);
        buffer_manager->unpinPage(record_ref.toPageId(), false);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        total_query_time_ms += duration.count() / 1000.0;
        total_queries++;
        
        if (record) {
            std::cout << "[OK] Registro encontrado exitosamente" << std::endl;
            std::cout << "     Tiempo: " << duration.count() / 1000.0 << " ms" << std::endl;
            cache_hits++;
            return record;
        } else {
            std::cout << "[X] Error extrayendo registro" << std::endl;
            cache_misses++;
            return nullptr;
        }
    }
    
    /**
     * @brief Operación INSERT - Insertar nuevo registro
     * 
     * Flujo:
     * 1. Almacenar registro en disco (DiskManager)
     * 2. Obtener referencia del registro almacenado
     * 3. Insertar referencia en B+ Tree
     */
    bool insertRecord(const std::string& key, std::unique_ptr<Record> record) {
        std::cout << "\n[*] EJECUTANDO INSERT para clave: '" << key << "'" << std::endl;
        std::cout << "================================================" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // PASO 1: Almacenar registro en disco
        std::cout << "[*] PASO 1: Almacenando registro en disco..." << std::endl;
        
        // Obtener nueva página
        auto page_id = disk_manager->allocateNewPageId();
        if (page_id <= 0) {
            std::cout << "[X] Error obteniendo direccion para nueva pagina " << page_id << std::endl;
            return false;
        }
        
        // Crear dirección física (platter=0, surface=0, track=0, sector=page_id)
        PhysicalAddress addr(0, 0, 0, page_id);
        
        // Crear bloque con la dirección física
        auto block = std::make_shared<Block>(addr);
        
        // Añadir el registro al bloque usando addRecord()
        std::shared_ptr<Record> shared_record(record.release());
        bool added = block->addRecord(shared_record);
        if (!added) {
            std::cout << "[X] Error agregando registro al bloque" << std::endl;
            return false;
        }
        
        // Almacenar en disco
        bool stored = disk_manager->writeBlock(addr, *block);
        if (!stored) {
            std::cout << "[X] Error almacenando bloque en disco" << std::endl;
            return false;
        }
        
        // PASO 2: Indexar en B+ Tree
        std::cout << "[*] PASO 2: Indexando en B+ Tree..." << std::endl;
        RecordReference record_ref(addr, 0); // slot 0
        bool index_success = btree_index->insert(key, record_ref);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        total_query_time_ms += duration.count() / 1000.0;
        total_queries++;
        
        if (index_success) {
            std::cout << "[OK] Registro insertado exitosamente" << std::endl;
            std::cout << "     PageID: " << page_id << ", Slot: 0" << std::endl;
            std::cout << "     Tiempo: " << duration.count() / 1000.0 << " ms" << std::endl;
            return true;
        } else {
            std::cout << "[X] Error indexando en B+ Tree" << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Operación RANGE SELECT - Búsqueda por rango
     */
    std::vector<std::unique_ptr<Record>> selectRange(const std::string& start_key, const std::string& end_key) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "\n[*] EJECUTANDO RANGE SELECT [" << start_key << ", " << end_key << "]" << std::endl;
        std::cout << "================================================" << std::endl;
        
        std::vector<std::unique_ptr<Record>> results;
        
        // PASO 1: Obtener referencias del B+ Tree
        std::cout << "[*] PASO 1: Consultando rango en B+ Tree..." << std::endl;
        auto record_refs = btree_index->rangeSearch(start_key, end_key);
        
        if (record_refs.empty()) {
            std::cout << "[X] No se encontraron registros en el rango" << std::endl;
            return results;
        }
        
        std::cout << "[OK] Encontradas " << record_refs.size() << " referencias" << std::endl;
        
        // PASO 2: Cargar registros usando BufferManager
        std::cout << "\n[*] PASO 2: Cargando registros..." << std::endl;
        
        for (const auto& ref : record_refs) {
            int page_id = ref.toPageId();
            auto block = buffer_manager->fetchPage(page_id);
            
            if (block) {
                auto record = extractRecordFromBlock(block, ref);
                if (record) {
                    results.push_back(std::move(record));
                }
                buffer_manager->unpinPage(page_id, false);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double query_time_ms = duration.count() / 1000.0;
        
        std::cout << "\n[OK] RANGE SELECT COMPLETADO" << std::endl;
        std::cout << "   Registros encontrados: " << results.size() << std::endl;
        std::cout << "   Tiempo: " << query_time_ms << " ms" << std::endl;
        std::cout << "================================================" << std::endl;
        
        return results;
    }
    
    /**
     * @brief Mostrar estadísticas del sistema
     */
    void displaySystemStatistics() {
        std::cout << "\n[*] ESTADISTICAS DEL SISTEMA [*]" << std::endl;
        std::cout << "================================================" << std::endl;
        
        // Estadísticas generales
        std::cout << "[*] CONSULTAS:" << std::endl;
        std::cout << "   Total de consultas: " << total_queries << std::endl;
        std::cout << "   Cache hits: " << cache_hits << std::endl;
        std::cout << "   Cache misses: " << cache_misses << std::endl;
        
        if (total_queries > 0) {
            double avg_time = total_query_time_ms / total_queries;
            double hit_ratio = (double)cache_hits / total_queries * 100.0;
            std::cout << "   Tiempo promedio: " << avg_time << " ms" << std::endl;
            std::cout << "   Hit ratio: " << hit_ratio << "%" << std::endl;
        }
        
        std::cout << "\n" << std::endl;
        
        // Estadísticas del B+ Tree
        btree_index->displayStatistics();
        
        std::cout << "\n" << std::endl;
        
        // Estadísticas del BufferManager
        buffer_manager->displayStatistics();
        
        std::cout << "\n" << std::endl;
        
        // Estadísticas del DiskManager
        disk_manager->displayStatistics();
        
        std::cout << "================================================" << std::endl;
    }
    
    /**
     * @brief Mostrar estado actual del sistema
     */
    void displaySystemState() {
        std::cout << "\n[*] ESTADO ACTUAL DEL SISTEMA [*]" << std::endl;
        std::cout << "================================================" << std::endl;
        
        std::cout << "[*] ESTRUCTURA B+ TREE [*]" << std::endl;
        if (btree_index) {
            std::cout << "Orden: " << btree_index->getOrder() << ", Altura: " << btree_index->getHeight() << std::endl;
            std::cout << "Registros: " << btree_index->getTotalRecords() << std::endl;
            std::cout << "Root: " << (!btree_index->isEmpty() ? "Existe" : "Vacio") << std::endl;
            std::cout << "================================" << std::endl;
            if (!btree_index->isEmpty()) {
                btree_index->display();
            }
        }
        
        std::cout << "\n[*] CLOCK BUFFER STATE (PIN-AWARE MEJORADO):" << std::endl;
        if (buffer_manager) {
            buffer_manager->displayClockState();
        }
        
        std::cout << "\n[*] Proteccion PIN-AWARE MEJORADA:" << std::endl;
        std::cout << "[+] NUNCA evicta paginas con pin_count > 0" << std::endl;
        std::cout << "[*] CADA pasada decrementa pin_count automaticamente" << std::endl;
        std::cout << "[!] Algoritmo Clock con reference bits activo" << std::endl;
        std::cout << "[+] GARANTIA: Eventualmente encuentra victimas SIEMPRE" << std::endl;
        std::cout << "================================================" << std::endl;
    }

private:
    /**
     * @brief Extrae un registro específico de un bloque
     */
    std::unique_ptr<Record> extractRecordFromBlock(std::shared_ptr<Block> block, const RecordReference& ref) {
        // Buscar registro por slot_id en el bloque
        const auto& records = block->getAllRecords();
        
        if (ref.getSlotId() >= 0 && ref.getSlotId() < static_cast<int>(records.size())) {
            auto record_ptr = records[ref.getSlotId()];
            
            if (record_ptr && !record_ptr->isDeleted()) {
                // Crear copia del registro para retornar
                return record_ptr->clone();
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Actualiza estadísticas de cache
     */
    void updateCacheStatistics(bool was_hit) {
        if (was_hit) {
            cache_hits++;
        } else {
            cache_misses++;
        }
    }
};

#endif // QUERY_EXECUTOR_H
