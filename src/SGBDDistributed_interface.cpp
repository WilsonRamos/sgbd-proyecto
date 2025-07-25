// Continuación de SGBDDistributed.cpp - Métodos de Interfaz Interactiva

#include "../include/SGBDDistributed.h"
#include <iostream>
#include <iomanip>

// ============================================================================
// INTERFAZ INTERACTIVA
// ============================================================================

void SGBDDistributed::run() {
    std::cout << "🌟 SISTEMA SGBD DISTRIBUIDO INTERACTIVO 🌟" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Versión Educativa para experimentar con:" << std::endl;
    std::cout << "✅ Hash Extensible vs B+ Tree" << std::endl;
    std::cout << "✅ Routing Automático vs Manual" << std::endl;
    std::cout << "✅ Consultas SQL personalizadas" << std::endl;
    std::cout << "✅ Análisis de rendimiento" << std::endl;
    
    // Inicializar servidores
    if (!initializeServers()) {
        std::cout << "❌ Error inicializando sistema. Saliendo..." << std::endl;
        return;
    }
    
    // Cargar datos
    std::cout << "\n📁 Cargando dataset GPS..." << std::endl;
    if (!loadGPSDataset()) {
        std::cout << "⚠️  Error cargando dataset principal, usando datos de muestra" << std::endl;
        loadSampleData();
    }
    
    // Bucle principal del menú
    std::string choice;
    do {
        displayMainMenu();
        std::cout << "\nSelecciona una opción (0-9): ";
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            executeCustomQuery();
        } else if (choice == "2") {
            std::cout << "\n🎯 Cambiando a modo manual para próxima consulta..." << std::endl;
            auto_routing_enabled = false;
            executeCustomQuery();
        } else if (choice == "3") {
            showQueryExamples();
        } else if (choice == "4") {
            toggleRoutingMode();
        } else if (choice == "5") {
            displayDetailedStatistics();
        } else if (choice == "6") {
            displayIndexStructures();
        } else if (choice == "7") {
            showQueryHistory();
        } else if (choice == "8") {
            std::cout << "\n📁 Recargando dataset completo..." << std::endl;
            loadGPSDataset();
        } else if (choice == "9") {
            showHelp();
        } else if (choice != "0") {
            std::cout << "❌ Opción inválida. Por favor selecciona 0-9." << std::endl;
        }
        
        if (choice != "0") {
            std::cout << "\nPresiona Enter para continuar...";
            std::cin.get();
        }
        
    } while (choice != "0");
    
    std::cout << "\n👋 ¡Gracias por usar el Sistema SGBD Distribuido Interactivo!" << std::endl;
    std::cout << "📊 Resumen final: " << total_queries << " consultas ejecutadas" << std::endl;
}

void SGBDDistributed::displayMainMenu() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "🚀 SISTEMA SGBD DISTRIBUIDO INTERACTIVO 🚀" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Estado del sistema
    showSystemStatus();
    
    std::cout << "\n🔀 MODO DE ROUTING: " << (auto_routing_enabled ? "AUTOMÁTICO" : "MANUAL") << std::endl;
    std::cout << "📈 Consultas ejecutadas: " << total_queries << std::endl;
    
    std::cout << "\n" << std::string(70, '-') << std::endl;
    std::cout << "OPCIONES DISPONIBLES:" << std::endl;
    std::cout << "1️⃣  Ejecutar consulta SQL personalizada" << std::endl;
    std::cout << "2️⃣  Seleccionar servidor manualmente" << std::endl;
    std::cout << "3️⃣  Ver ejemplos de consultas" << std::endl;
    std::cout << "4️⃣  Cambiar modo de routing (Auto/Manual)" << std::endl;
    std::cout << "5️⃣  Ver estadísticas detalladas" << std::endl;
    std::cout << "6️⃣  Ver estructuras de índices" << std::endl;
    std::cout << "7️⃣  Ver historial de consultas" << std::endl;
    std::cout << "8️⃣  Recargar dataset GPS completo" << std::endl;
    std::cout << "9️⃣  Ayuda - Guía de consultas" << std::endl;
    std::cout << "0️⃣  Salir" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
}

void SGBDDistributed::showSystemStatus() {
    std::cout << "\n📊 ESTADO DEL SISTEMA:" << std::endl;
    
    // Estado general
    std::string state_str;
    switch (current_state) {
        case DistributedSystemState::NOT_INITIALIZED:
            state_str = "❌ NO INICIALIZADO";
            break;
        case DistributedSystemState::SERVERS_READY:
            state_str = "🟡 SERVIDORES LISTOS";
            break;
        case DistributedSystemState::DATA_LOADED:
            state_str = "✅ DATOS CARGADOS";
            break;
        case DistributedSystemState::ERROR_STATE:
            state_str = "🔴 ERROR";
            break;
    }
    std::cout << "Estado: " << state_str << std::endl;
    
    // Estado de servidores
    if (server_s1 && server_s2) {
        server_s1->displayBasicStats();
        server_s2->displayBasicStats();
    }
    
    std::cout << "📁 Dataset: " << dataset_path << std::endl;
    std::cout << "📊 Registros totales: " << total_loaded_records << std::endl;
}

void SGBDDistributed::executeCustomQuery() {
    std::cout << "\n💻 EJECUTOR DE CONSULTAS SQL PERSONALIZADO" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    if (current_state != DistributedSystemState::DATA_LOADED) {
        std::cout << "❌ Error: Sistema no está listo para consultas" << std::endl;
        return;
    }
    
    if (auto_routing_enabled) {
        std::cout << "🔀 Modo: ROUTING AUTOMÁTICO (el sistema elige el mejor servidor)" << std::endl;
    } else {
        std::cout << "🎯 Modo: SELECCIÓN MANUAL de servidor" << std::endl;
    }
    
    std::cout << "\nEscribe tu consulta SQL (o 'help' para ejemplos, 'back' para volver):" << std::endl;
    std::cout << "SQL> ";
    
    std::string query;
    std::getline(std::cin, query);
    
    if (query == "back" || query.empty()) return;
    if (query == "help") {
        showQueryExamples();
        return;
    }
    
    // Agregar al historial
    query_history.push_back(query);
    total_queries++;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::unique_ptr<GPSRecord>> results;
    
    if (auto_routing_enabled) {
        results = executeWithAutoRouting(query);
        auto_routed_queries++;
    } else {
        results = executeWithManualRouting(query);
        manual_routed_queries++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Mostrar resultados
    displayQueryResults(results, query, duration.count());
}

std::vector<std::unique_ptr<GPSRecord>> SGBDDistributed::executeWithAutoRouting(const std::string& query) {
    std::cout << "\n🤖 ROUTING AUTOMÁTICO:" << std::endl;
    
    std::vector<std::unique_ptr<GPSRecord>> results;
    
    if (query.find("imei") != std::string::npos && query.find("=") != std::string::npos) {
        std::cout << "🔀 Detectado: Consulta por IMEI → Enviando a S1 (Hash)" << std::endl;
        results = server_s1->executeCustomQuery(query);
    } else if (query.find("timestamp") != std::string::npos || 
               query.find("BETWEEN") != std::string::npos ||
               (query.find("SELECT *") != std::string::npos && query.find("WHERE") == std::string::npos)) {
        std::cout << "🔀 Detectado: Consulta temporal/scan → Enviando a S2 (B+ Tree)" << std::endl;
        results = server_s2->executeCustomQuery(query);
    } else {
        std::cout << "🔀 Consulta no reconocida → Probando ambos servidores" << std::endl;
        
        std::cout << "\n🔍 Intentando en S1 (Hash):" << std::endl;
        auto results_s1 = server_s1->executeCustomQuery(query);
        
        std::cout << "\n🔍 Intentando en S2 (B+ Tree):" << std::endl;
        auto results_s2 = server_s2->executeCustomQuery(query);
        
        // Combinar resultados
        results = std::move(results_s1);
        for (auto& r : results_s2) {
            results.push_back(std::move(r));
        }
    }
    
    return results;
}

std::vector<std::unique_ptr<GPSRecord>> SGBDDistributed::executeWithManualRouting(const std::string& query) {
    std::cout << "\n🎯 SELECCIÓN MANUAL DE SERVIDOR:" << std::endl;
    std::cout << "1. S1 - Hash Extensible (IMEI)" << std::endl;
    std::cout << "2. S2 - B+ Tree (Timestamp)" << std::endl;
    std::cout << "3. Ambos servidores" << std::endl;
    std::cout << "Selecciona servidor (1-3): ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    std::vector<std::unique_ptr<GPSRecord>> results;
    
    if (choice == "1") {
        std::cout << "\n📋 Ejecutando en S1 (Hash Extensible)..." << std::endl;
        results = server_s1->executeCustomQuery(query);
    } else if (choice == "2") {
        std::cout << "\n📈 Ejecutando en S2 (B+ Tree)..." << std::endl;
        results = server_s2->executeCustomQuery(query);
    } else if (choice == "3") {
        std::cout << "\n🔄 Ejecutando en ambos servidores..." << std::endl;
        
        std::cout << "\n📋 Resultados de S1:" << std::endl;
        auto results_s1 = server_s1->executeCustomQuery(query);
        
        std::cout << "\n📈 Resultados de S2:" << std::endl;
        auto results_s2 = server_s2->executeCustomQuery(query);
        
        // Combinar resultados
        results = std::move(results_s1);
        for (auto& r : results_s2) {
            results.push_back(std::move(r));
        }
    }
    
    return results;
}

void SGBDDistributed::displayQueryResults(const std::vector<std::unique_ptr<GPSRecord>>& results, 
                                        const std::string& query, long long execution_time_us) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "📊 RESULTADOS DE LA CONSULTA" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "SQL: " << query << std::endl;
    std::cout << "Tiempo de ejecución: " << execution_time_us << " μs" << std::endl;
    std::cout << "Registros encontrados: " << results.size() << std::endl;
    
    if (!results.empty()) {
        std::cout << "\n📋 REGISTROS:" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        for (size_t i = 0; i < results.size() && i < 10; ++i) { // Mostrar máximo 10
            std::cout << (i + 1) << ". ";
            results[i]->displayGPSInfo();
            std::cout << std::endl;
        }
        
        if (results.size() > 10) {
            std::cout << "... (" << (results.size() - 10) << " registros más)" << std::endl;
        }
    } else {
        std::cout << "\n⚠️  No se encontraron registros que coincidan con la consulta." << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
}

void SGBDDistributed::showQueryExamples() {
    std::cout << "\n📚 EJEMPLOS DE CONSULTAS SQL:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::cout << "\n🔍 PARA SERVIDOR S1 (Hash Extensible - IMEI):" << std::endl;
    std::cout << "  SELECT * FROM gps WHERE imei = '868018070237402';" << std::endl;
    std::cout << "  SELECT * FROM gps WHERE imei = '868018070237410';" << std::endl;
    std::cout << "  → Búsquedas exactas O(1)" << std::endl;
    
    std::cout << "\n🌳 PARA SERVIDOR S2 (B+ Tree - Timestamp):" << std::endl;
    std::cout << "  SELECT * FROM gps WHERE timestamp BETWEEN '2025-06-25 00:47:00' AND '2025-06-25 00:52:00';" << std::endl;
    std::cout << "  SELECT * FROM gps WHERE timestamp = '2025-06-25 00:50:02+00';" << std::endl;
    std::cout << "  SELECT * FROM gps;" << std::endl;
    std::cout << "  → Range queries y full scans eficientes" << std::endl;
    
    std::cout << "\n💡 CONSEJOS:" << std::endl;
    std::cout << "  • Usa IMEI para búsquedas exactas rápidas" << std::endl;
    std::cout << "  • Usa timestamp para análisis temporales" << std::endl;
    std::cout << "  • Compara rendimiento entre servidores" << std::endl;
}

void SGBDDistributed::toggleRoutingMode() {
    auto_routing_enabled = !auto_routing_enabled;
    std::cout << "\n🔀 Modo de routing cambiado a: " 
              << (auto_routing_enabled ? "AUTOMÁTICO" : "MANUAL") << std::endl;
    
    if (auto_routing_enabled) {
        std::cout << "   → El sistema elegirá automáticamente el mejor servidor" << std::endl;
    } else {
        std::cout << "   → Podrás seleccionar manualmente el servidor para cada consulta" << std::endl;
    }
}

void SGBDDistributed::displayDetailedStatistics() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "📊 ESTADÍSTICAS DETALLADAS DEL SISTEMA DISTRIBUIDO" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Estadísticas globales
    std::cout << "\n🌐 ESTADÍSTICAS GLOBALES:" << std::endl;
    std::cout << "Total de consultas ejecutadas: " << total_queries << std::endl;
    std::cout << "Consultas con routing automático: " << auto_routed_queries << std::endl;
    std::cout << "Consultas con routing manual: " << manual_routed_queries << std::endl;
    std::cout << "Dataset cargado: " << dataset_path << std::endl;
    std::cout << "Registros totales: " << total_loaded_records << std::endl;
    
    // Estadísticas por servidor
    if (server_s1 && server_s2) {
        server_s1->displayStatistics();
        server_s2->displayStatistics();
    }
    
    // Análisis de eficiencia
    std::cout << "\n💡 ANÁLISIS DE EFICIENCIA:" << std::endl;
    if (server_s1 && server_s2 && server_s1->getRecordsStored() > 0 && server_s2->getRecordsStored() > 0) {
        std::cout << "✅ Distribución de datos balanceada" << std::endl;
        std::cout << "✅ Hash para IMEI: O(1) búsquedas exactas" << std::endl;
        std::cout << "✅ B+ Tree para timestamp: Eficiente en range queries" << std::endl;
        
        double s1_load = static_cast<double>(server_s1->getRecordsStored()) / total_loaded_records * 100;
        double s2_load = static_cast<double>(server_s2->getRecordsStored()) / total_loaded_records * 100;
        
        std::cout << "📊 Distribución de carga:" << std::endl;
        std::cout << "   S1: " << std::fixed << std::setprecision(1) << s1_load << "%" << std::endl;
        std::cout << "   S2: " << std::fixed << std::setprecision(1) << s2_load << "%" << std::endl;
    }
}

void SGBDDistributed::displayIndexStructures() {
    std::cout << "\n📐 ESTRUCTURAS DE ÍNDICES:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    if (server_s1 && server_s2) {
        server_s1->displayStructure();
        server_s2->displayStructure();
    } else {
        std::cout << "❌ Servidores no inicializados" << std::endl;
    }
}

void SGBDDistributed::showQueryHistory() {
    std::cout << "\n📜 HISTORIAL DE CONSULTAS:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    if (query_history.empty()) {
        std::cout << "No hay consultas en el historial." << std::endl;
        return;
    }
    
    for (size_t i = 0; i < query_history.size(); ++i) {
        std::cout << (i + 1) << ". " << query_history[i] << std::endl;
    }
}

void SGBDDistributed::showHelp() {
    std::cout << "\n📖 GUÍA DE CONSULTAS AVANZADA:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    std::cout << "\n🎯 CÓMO ELEGIR EL SERVIDOR CORRECTO:" << std::endl;
    std::cout << "\n📋 Servidor S1 (Hash Extensible):" << std::endl;
    std::cout << "   ✅ IDEAL para: Búsquedas exactas por IMEI" << std::endl;
    std::cout << "   ✅ Complejidad: O(1) promedio" << std::endl;
    std::cout << "   ❌ NO ideal para: Range queries, ordenamiento" << std::endl;
    
    std::cout << "\n📈 Servidor S2 (B+ Tree):" << std::endl;
    std::cout << "   ✅ IDEAL para: Range queries, ordenamiento, full scans" << std::endl;
    std::cout << "   ✅ Complejidad: O(log n) búsquedas, O(k) range queries" << std::endl;
    std::cout << "   ❌ Menos eficiente para: Búsquedas exactas simples" << std::endl;
    
    std::cout << "\n🔀 ROUTING AUTOMÁTICO vs MANUAL:" << std::endl;
    std::cout << "   🤖 Automático: Sistema analiza la consulta y elige servidor" << std::endl;
    std::cout << "   🎯 Manual: Tú eliges el servidor (educativo para comparar)" << std::endl;
    
    std::cout << "\n📁 DATASET GPS:" << std::endl;
    std::cout << "   📊 " << total_loaded_records << " registros cargados" << std::endl;
    std::cout << "   📍 Datos de tracking GPS de vehículos" << std::endl;
    std::cout << "   🔑 Campos clave: IMEI, timestamp, lat/lon, speed, altitude" << std::endl;
    
    showQueryExamples();
}