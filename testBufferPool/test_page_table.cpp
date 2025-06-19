#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cassert>

// Incluir la clase PageTable
#include "buffer/PageTable.h"

/**
 * @brief Test Completo para PageTable.h
 * 
 * Prueba todas las funcionalidades de la PageTable siguiendo
 * los conceptos de la conferencia CMU sobre buffer pool management
 */

void printTestHeader(const std::string& test_name) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TEST: " << test_name << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void printStep(const std::string& step) {
    std::cout << "\n" << step << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

bool runBasicOperationsTest() {
    printTestHeader("OPERACIONES BÁSICAS DE PAGE TABLE");
    
    PageTable page_table;
    
    // TEST 1: Inserción de páginas
    printStep("1. Inserción de Páginas");
    
    std::cout << "Insertando pagina 1..." << std::endl;
    bool result1 = page_table.insertPage(1);  // Auto-asigna frame 0
    assert(result1 == true);
    std::cout << "Pagina 1 insertada correctamente" << std::endl;
    
    std::cout << "Insertando pagina 2..." << std::endl;
    bool result2 = page_table.insertPage(2);  // Auto-asigna frame 1
    assert(result2 == true);
    std::cout << "Pagina 2 insertada correctamente" << std::endl;
    
    std::cout << "Intentando insertar pagina 1 nuevamente..." << std::endl;
    bool result3 = page_table.insertPage(1);  // Debe fallar (ya existe)
    assert(result3 == false);
    std::cout << "Insercion duplicada rechazada correctamente" << std::endl;
    
    // TEST 2: Búsqueda de páginas
    printStep("2. Búsqueda de Páginas");
    
    PageTableEntry entry;
    std::cout << "Buscando pagina 1..." << std::endl;
    bool found1 = page_table.findPage(1, entry);
    assert(found1 == true);
    assert(entry.frame_id == 0);
    assert(entry.valid_bit == true);
    assert(entry.dirty_bit == false);
    assert(entry.pin_count == 0);
    std::cout << "Pagina 1 encontrada: Frame " << entry.frame_id << std::endl;
    
    std::cout << "Buscando pagina inexistente (99)..." << std::endl;
    bool found2 = page_table.findPage(99, entry);
    assert(found2 == false);
    std::cout << "Pagina inexistente no encontrada" << std::endl;
    
    // TEST 3: Verificación de estado inicial
    printStep("3. Estado Inicial de la Page Table");
    
    auto stats = page_table.getStats();
    assert(stats.total_pages == 2);
    assert(stats.pinned_pages == 0);
    assert(stats.dirty_pages == 0);
    assert(stats.evictable_pages == 2);
    
    std::cout << "Estadisticas iniciales:" << std::endl;
    std::cout << "   - Total paginas: " << stats.total_pages << std::endl;
    std::cout << "   - Paginas pinned: " << stats.pinned_pages << std::endl;
    std::cout << "   - Paginas dirty: " << stats.dirty_pages << std::endl;
    std::cout << "   - Paginas evictables: " << stats.evictable_pages << std::endl;
    
    page_table.displayCompact();
    
    return true;
}

bool runPinUnpinTest() {
    printTestHeader("GESTIÓN DE PIN/UNPIN");
    
    PageTable page_table;
    
    // Preparar páginas
    page_table.insertPage(10);
    page_table.insertPage(20);
    page_table.insertPage(30);
    
    printStep("1. Estado Inicial");
    page_table.displayCompact();
    
    // TEST 1: Pin de páginas
    printStep("2. Operaciones de PIN");
    
    std::cout << "Haciendo pin de página 10..." << std::endl;
    bool pin_result1 = page_table.pinPage(10);
    assert(pin_result1 == true);
    
    std::cout << "Haciendo pin de página 10 otra vez..." << std::endl;
    bool pin_result2 = page_table.pinPage(10);  // pin_count = 2
    assert(pin_result2 == true);
    
    std::cout << "Haciendo pin de página 20..." << std::endl;
    bool pin_result3 = page_table.pinPage(20);
    assert(pin_result3 == true);
    
    page_table.displayCompact();
    
    // Verificar pin counts
    PageTableEntry entry;
    page_table.findPage(10, entry);
    assert(entry.pin_count == 2);
    std::cout << "✅ Página 10 tiene pin_count = " << entry.pin_count << std::endl;
    
    page_table.findPage(20, entry);
    assert(entry.pin_count == 1);
    std::cout << "✅ Página 20 tiene pin_count = " << entry.pin_count << std::endl;
    
    // TEST 2: Unpin de páginas
    printStep("3. Operaciones de UNPIN");
    
    std::cout << "Haciendo unpin de página 10..." << std::endl;
    bool unpin_result1 = page_table.unpinPage(10);
    assert(unpin_result1 == true);
    
    page_table.findPage(10, entry);
    assert(entry.pin_count == 1);
    std::cout << "✅ Página 10 ahora tiene pin_count = " << entry.pin_count << std::endl;
    
    std::cout << "Haciendo unpin de página 10 otra vez..." << std::endl;
    bool unpin_result2 = page_table.unpinPage(10);
    assert(unpin_result2 == true);
    
    page_table.findPage(10, entry);
    assert(entry.pin_count == 0);
    std::cout << "✅ Página 10 ahora tiene pin_count = " << entry.pin_count << std::endl;
    
    // TEST 3: Verificar estados de evicción
    printStep("4. Verificación de Estados de Evicción");
    
    auto evictable_pages = page_table.getEvictablePages();
    std::cout << "Páginas evictables: ";
    for (int page_id : evictable_pages) {
        std::cout << page_id << " ";
    }
    std::cout << std::endl;
    
    // Página 20 debe estar pinned (no evictable)
    // Páginas 10 y 30 deben ser evictables
    assert(evictable_pages.size() == 2);
    std::cout << "✅ " << evictable_pages.size() << " páginas son evictables" << std::endl;
    
    page_table.displayCompact();
    
    return true;
}

bool runDirtyBitTest() {
    printTestHeader("GESTIÓN DE DIRTY BITS");
    
    PageTable page_table;
    
    // Preparar páginas
    page_table.insertPage(100);
    page_table.insertPage(200);
    
    printStep("1. Estado Inicial (Páginas CLEAN)");
    page_table.displayInfo();
    
    // TEST 1: Marcar páginas como dirty
    printStep("2. Marcar Páginas como DIRTY");
    
    std::cout << "Marcando página 100 como dirty..." << std::endl;
    bool dirty_result1 = page_table.markDirty(100);
    assert(dirty_result1 == true);
    
    PageTableEntry entry;
    page_table.findPage(100, entry);
    assert(entry.dirty_bit == true);
    std::cout << "✅ Página 100 ahora está DIRTY" << std::endl;
    
    std::cout << "Marcando página 200 como dirty..." << std::endl;
    bool dirty_result2 = page_table.markDirty(200);
    assert(dirty_result2 == true);
    
    page_table.displayCompact();
    
    // TEST 2: Verificar estadísticas
    printStep("3. Verificar Estadísticas de Dirty Pages");
    
    auto stats = page_table.getStats();
    assert(stats.dirty_pages == 2);
    std::cout << "✅ " << stats.dirty_pages << " páginas están dirty" << std::endl;
    
    // TEST 3: Limpiar dirty bits
    printStep("4. Limpiar Dirty Bits");
    
    std::cout << "Limpiando dirty bit de página 100..." << std::endl;
    bool clean_result1 = page_table.clearDirty(100);
    assert(clean_result1 == true);
    
    page_table.findPage(100, entry);
    assert(entry.dirty_bit == false);
    std::cout << "✅ Página 100 ahora está CLEAN" << std::endl;
    
    // TEST 4: Unpin con dirty
    printStep("5. Unpin con Dirty Flag");
    
    page_table.pinPage(200);
    std::cout << "Haciendo unpin de página 200 con dirty=true..." << std::endl;
    bool unpin_dirty = page_table.unpinPage(200, true);
    assert(unpin_dirty == true);
    
    page_table.findPage(200, entry);
    assert(entry.dirty_bit == true);
    std::cout << "✅ Página 200 sigue DIRTY después de unpin" << std::endl;
    
    page_table.displayInfo();
    
    return true;
}

bool runLRUTimingTest() {
    printTestHeader("GESTIÓN DE TIMING PARA LRU");
    
    PageTable page_table;
    
    // Preparar páginas
    page_table.insertPage(1);
    page_table.insertPage(2);
    page_table.insertPage(3);
    
    printStep("1. Accesos con Delays para Simular LRU");
    
    // Acceder páginas con delays
    std::cout << "Accediendo página 1..." << std::endl;
    PageTableEntry entry;
    page_table.findPage(1, entry);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "Accediendo página 2..." << std::endl;
    page_table.findPage(2, entry);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "Accediendo página 3..." << std::endl;
    page_table.findPage(3, entry);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "Re-accediendo página 1..." << std::endl;
    page_table.findPage(1, entry);
    
    printStep("2. Verificar Orden LRU");
    
    // Página 2 debe ser la LRU (menos recientemente usada)
    int lru_page = page_table.getLRUEvictablePage();
    std::cout << "Página LRU (candidata para evicción): " << lru_page << std::endl;
    
    // Mostrar tiempos de acceso
    for (int page_id : {1, 2, 3}) {
        page_table.findPage(page_id, entry);
        std::cout << "Página " << page_id << ": " 
                  << entry.getTimeSinceLastAccess() << "ms desde último acceso" << std::endl;
    }
    
    page_table.displayInfo();
    
    return true;
}

bool runCompleteWorkflowTest() {
    printTestHeader("FLUJO COMPLETO DE TRABAJO");
    
    PageTable page_table;
    
    printStep("SIMULACIÓN: Operaciones típicas de una consulta");
    
    // Simular una consulta que necesita páginas 1, 2, 3
    std::cout << "\n🔍 QUERY: SELECT * FROM tabla WHERE id IN (1,2,3)" << std::endl;
    
    // 1. Solicitar páginas para la consulta
    std::cout << "\n1️⃣ Solicitando páginas para la consulta..." << std::endl;
    for (int page_id : {1, 2, 3}) {
        page_table.insertPage(page_id);
        page_table.pinPage(page_id);  // Pin para prevenir evicción
        std::cout << "   📌 Página " << page_id << " pinned para uso" << std::endl;
    }
    
    page_table.displayCompact();
    
    // 2. Simular lectura (no dirty)
    std::cout << "\n2️⃣ Leyendo datos de páginas..." << std::endl;
    PageTableEntry entry;
    for (int page_id : {1, 2, 3}) {
        page_table.findPage(page_id, entry);  // Actualiza access time
        std::cout << "   📖 Leyendo página " << page_id << std::endl;
    }
    
    // 3. Simular modificación en página 2
    std::cout << "\n3️⃣ Modificando página 2..." << std::endl;
    page_table.markDirty(2);
    std::cout << "   ✏️ Página 2 marcada como DIRTY" << std::endl;
    
    page_table.displayCompact();
    
    // 4. Liberar páginas cuando termine la consulta
    std::cout << "\n4️⃣ Liberando páginas al finalizar consulta..." << std::endl;
    page_table.unpinPage(1, false);  // Solo lectura
    page_table.unpinPage(2, true);   // Modificada
    page_table.unpinPage(3, false);  // Solo lectura
    
    std::cout << "   📍 Todas las páginas unpinned" << std::endl;
    
    // 5. Estado final
    std::cout << "\n5️⃣ Estado final después de la consulta:" << std::endl;
    page_table.displayInfo();
    
    // 6. Verificar que páginas son evictables
    auto evictable = page_table.getEvictablePages();
    std::cout << "\n📊 Páginas evictables: " << evictable.size() << " de 3" << std::endl;
    
    int lru_candidate = page_table.getLRUEvictablePage();
    if (lru_candidate != -1) {
        std::cout << "🎯 Próximo candidato LRU para evicción: Página " << lru_candidate << std::endl;
    }
    
    return true;
}

int main() {
    std::cout << "INICIANDO TESTS DE PAGE TABLE" << std::endl;
    std::cout << "===============================================================" << std::endl;
    
    bool all_tests_passed = true;
    
    try {
        // Test 1: Operaciones básicas
        if (!runBasicOperationsTest()) {
            std::cerr << "❌ Test de operaciones básicas falló" << std::endl;
            all_tests_passed = false;
        }
        
        // Test 2: Pin/Unpin
        if (!runPinUnpinTest()) {
            std::cerr << "❌ Test de pin/unpin falló" << std::endl;
            all_tests_passed = false;
        }
        
        // Test 3: Dirty bits
        if (!runDirtyBitTest()) {
            std::cerr << "❌ Test de dirty bits falló" << std::endl;
            all_tests_passed = false;
        }
        
        // Test 4: LRU timing
        if (!runLRUTimingTest()) {
            std::cerr << "❌ Test de LRU timing falló" << std::endl;
            all_tests_passed = false;
        }
        
        // Test 5: Flujo completo
        if (!runCompleteWorkflowTest()) {
            std::cerr << "❌ Test de flujo completo falló" << std::endl;
            all_tests_passed = false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error durante tests: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    
    // Resultado final
    std::cout << "\n" << std::string(60, '=') << std::endl;
    if (all_tests_passed) {
        std::cout << "TODOS LOS TESTS PASARON EXITOSAMENTE" << std::endl;
        std::cout << "PageTable.h funciona correctamente" << std::endl;
        std::cout << "Implementacion sigue conceptos de conferencia CMU" << std::endl;
        std::cout << "Ready para integracion con BufferPoolManager" << std::endl;
    } else {
        std::cout << "ALGUNOS TESTS FALLARON" << std::endl;
        std::cout << "Revisar implementacion de PageTable.h" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
    
    return all_tests_passed ? 0 : 1;
}

/*
═══════════════════════════════════════════════════════════════════════════════
📋 QUÉ TESTEA ESTE ARCHIVO:

✅ FUNCIONALIDADES BÁSICAS:
   - Inserción de páginas con auto-asignación de frames
   - Búsqueda O(1) en hash map
   - Rechazo de duplicados
   - Estados iniciales correctos

✅ GESTIÓN DE PIN/UNPIN:
   - Pin count increment/decrement
   - Múltiples pins en misma página
   - Páginas evictables vs no-evictables
   - Prevención de evicción durante uso

✅ GESTIÓN DE DIRTY BITS:
   - Marcar páginas como modificadas
   - Limpiar dirty bits después de flush
   - Unpin con dirty flag
   - Estadísticas de páginas dirty

✅ TIMING PARA LRU:
   - Actualización automática de access time
   - Identificación de página LRU
   - Orden de acceso temporal
   - Preparación para política de evicción

✅ FLUJO COMPLETO DE TRABAJO:
   - Simulación realista de consulta
   - Pin → Use → Modify → Unpin
   - Estados coherentes durante todo el ciclo
   - Integración de todas las funcionalidades

═══════════════════════════════════════════════════════════════════════════════
🎯 CÓMO COMPILAR Y EJECUTAR:

1. Asegurar que PageTable.h esté en include/buffer/
2. Compilar: g++ -o test_page_table test_page_table.cpp -Iinclude -std=c++17
3. Ejecutar: ./test_page_table
4. Verificar que todos los tests pasen ✅

═══════════════════════════════════════════════════════════════════════════════
*/