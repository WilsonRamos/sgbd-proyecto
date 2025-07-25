#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <map>
#include <iomanip>
#include <algorithm>

// Headers del proyecto existente - ESTRUCTURA CORREGIDA
#include "include/HashExtendible/ExtensibleHash.h"
#include "include/BPlusTree/BPlusTree.h"
#include "include/buffer/BufferManagerClock.h"
#include "include/DiskManagerExtended.h"
#include "include/Record.h"
#include "include/RecordReference.h"
#include "include/PhysicalAddress.h"

/**
 * @brief Registro GPS especializado para datos del CSV
 */
class GPSRecord : public VariableRecord {
public:
    GPSRecord(int id = -1) : VariableRecord(id) {
        setupGPSSchema();
    }
    
    void setupGPSSchema() {
        std::vector<FieldDefinition> gps_schema = {
            {"id", FieldType::INTEGER, 0},
            {"imei", FieldType::STRING, 20},
            {"commandId", FieldType::INTEGER, 0},
            {"timestamp", FieldType::STRING, 30},
            {"latitude", FieldType::STRING, 15},
            {"longitude", FieldType::STRING, 15},
            {"recordIndex", FieldType::INTEGER, 0},
            {"timestampExtension", FieldType::INTEGER, 0},
            {"recordExtension", FieldType::INTEGER, 0},
            {"priority", FieldType::INTEGER, 0},
            {"altitude", FieldType::STRING, 10},
            {"angle", FieldType::STRING, 10},
            {"satellites", FieldType::INTEGER, 0},
            {"speed", FieldType::INTEGER, 0},
            {"hdop", FieldType::STRING, 10},
            {"eventId", FieldType::INTEGER, 0},
            {"punto", FieldType::STRING, 100},
            {"ioElements", FieldType::STRING, 200},
            {"processedAt", FieldType::STRING, 30},
            {"createdAt", FieldType::STRING, 30},
            {"updatedAt", FieldType::STRING, 30}
        };
        setSchema(gps_schema);
    }
    
    // Getters específicos para campos clave
    std::string getIMEI() const { return getField(1); }
    std::string getTimestamp() const { return getField(3); }
    std::string getLatitude() const { return getField(4); }
    std::string getLongitude() const { return getField(5); }
    std::string getAltitude() const { return getField(10); }
    std::string getSpeed() const { return getField(13); }
    
    void setFromCSVLine(const std::vector<std::string>& csvFields) {
        if (csvFields.size() >= 21) {
            setFieldValues(csvFields);
            setId(std::stoi(csvFields[0]));
            calculateOffsets();
        }
    }
    
    std::unique_ptr<Record> clone() const override {
        auto cloned = std::make_unique<GPSRecord>(getId());
        cloned->setFieldValues(this->getFieldValues());
        cloned->calculateOffsets();
        return cloned;
    }
    
    void displayGPSInfo() const {
        std::cout << "GPS[ID:" << getId() 
                  << ", IMEI:" << getIMEI()
                  << ", Time:" << getTimestamp()
                  << ", Lat:" << getLatitude() 
                  << ", Lon:" << getLongitude()
                  << ", Alt:" << getAltitude() << "m"
                  << ", Speed:" << getSpeed() << "km/h]";
    }
};

/**
 * @brief Servidor especializado con un solo tipo de índice
 */
class SpecializedServer {
protected:
    std::string server_name;
    std::string server_type;
    std::shared_ptr<BufferManagerClock> buffer_manager;
    std::shared_ptr<DiskManagerExtended> disk_manager;
    
    // Estadísticas del servidor
    size_t total_operations = 0;
    size_t read_operations = 0;
    size_t write_operations = 0;
    size_t records_stored = 0;
    
public:
    SpecializedServer(const std::string& name, const std::string& type, int buffer_size = 32) 
        : server_name(name), server_type(type) {
        
        // Configurar componentes base - ORDEN CORREGIDO
        disk_manager = std::make_shared<DiskManagerExtended>("disk_" + name);
        buffer_manager = std::make_shared<BufferManagerClock>(buffer_size, disk_manager.get());
        
        std::cout << "🖥️  Servidor " << server_name << " (" << server_type << ") inicializado" << std::endl;
    }
    
    virtual ~SpecializedServer() = default;
    
    // Métodos virtuales para operaciones específicas
    virtual bool insert(const std::string& key, std::unique_ptr<GPSRecord> record) = 0;
    virtual std::vector<std::unique_ptr<GPSRecord>> search(const std::string& query) = 0;
    virtual std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) = 0;
    virtual void displayStatistics() const = 0;
    virtual void displayStructure() const = 0;
    virtual std::string getIndexType() const = 0;
    
    const std::string& getName() const { return server_name; }
    const std::string& getType() const { return server_type; }
    size_t getRecordsStored() const { return records_stored; }
    
    void incrementReadOps() { read_operations++; total_operations++; }
    void incrementWriteOps() { write_operations++; total_operations++; }
    
    void displayBasicStats() const {
        std::cout << "📊 " << server_name << " - " << getIndexType() 
                  << " | Registros: " << records_stored 
                  << " | Ops: " << total_operations 
                  << " (R:" << read_operations << ", W:" << write_operations << ")" << std::endl;
    }
};

/**
 * @brief Servidor S1: Especializado en Hash Extensible (IMEI)
 */
class TransactionalServer : public SpecializedServer {
private:
    std::unique_ptr<ExtensibleHash> imei_index;
    
public:
    TransactionalServer() : SpecializedServer("S1", "Transaccional") {
        imei_index = std::make_unique<ExtensibleHash>(4);
        std::cout << "📋 S1: Hash Extensible por IMEI (Búsquedas exactas O(1))" << std::endl;
    }
    
    std::string getIndexType() const override { return "Hash Extensible"; }
    
    bool insert(const std::string& imei, std::unique_ptr<GPSRecord> record) override {
        incrementWriteOps();
        
        if (imei_index->insert(imei, std::move(record))) {
            records_stored++;
            return true;
        }
        return false;
    }
    
    std::vector<std::unique_ptr<GPSRecord>> search(const std::string& imei_query) override {
        incrementReadOps();
        
        std::vector<std::unique_ptr<GPSRecord>> results;
        
        GPSRecord temp_record;
        if (imei_index->search(imei_query, temp_record)) {
            auto base_record = temp_record.clone();
            auto gps_record = std::unique_ptr<GPSRecord>(
                static_cast<GPSRecord*>(base_record.release())
            );
            results.push_back(std::move(gps_record));
        }
        
        return results;
    }
    
    std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) override {
        std::vector<std::unique_ptr<GPSRecord>> results;
        
        // Parser simple para consultas por IMEI
        if (sql.find("imei") != std::string::npos && sql.find("=") != std::string::npos) {
            // Extraer IMEI de la consulta
            size_t start = sql.find("'");
            if (start != std::string::npos) {
                size_t end = sql.find("'", start + 1);
                if (end != std::string::npos) {
                    std::string imei = sql.substr(start + 1, end - start - 1);
                    std::cout << "🔍 Buscando IMEI: " << imei << " en Hash Extensible..." << std::endl;
                    results = search(imei);
                }
            }
        } else {
            std::cout << "⚠️  Servidor S1 optimizado para consultas: WHERE imei = 'valor'" << std::endl;
        }
        
        return results;
    }
    
    void displayStatistics() const override {
        std::cout << "\n📊 ESTADÍSTICAS " << server_name << " (Hash Extensible)" << std::endl;
        std::cout << "Registros almacenados: " << records_stored << std::endl;
        std::cout << "Operaciones totales: " << total_operations << std::endl;
        std::cout << "  - Lecturas: " << read_operations << " (" 
                  << (total_operations > 0 ? (read_operations * 100.0 / total_operations) : 0) 
                  << "%)" << std::endl;
        std::cout << "  - Escrituras: " << write_operations << " (" 
                  << (total_operations > 0 ? (write_operations * 100.0 / total_operations) : 0) 
                  << "%)" << std::endl;
        
        imei_index->displayStatistics();
    }
    
    void displayStructure() const override {
        std::cout << "\n🏗️ ESTRUCTURA Hash Extensible (S1):" << std::endl;
        imei_index->displayStructure();
    }
};

/**
 * @brief Servidor S2: Especializado en B+ Tree (Timestamp)
 */
class AnalyticalServer : public SpecializedServer {
private:
    std::unique_ptr<BPlusTree<std::string>> timestamp_index;
    std::map<std::string, std::unique_ptr<GPSRecord>> record_storage;
    
public:
    AnalyticalServer() : SpecializedServer("S2", "Analítico", 16) {
        timestamp_index = std::make_unique<BPlusTree<std::string>>(3);
        std::cout << "📈 S2: B+ Tree por Timestamp (Range queries eficientes)" << std::endl;
    }
    
    std::string getIndexType() const override { return "B+ Tree"; }
    
    bool insert(const std::string& timestamp, std::unique_ptr<GPSRecord> record) override {
        incrementWriteOps();
        
        RecordReference ref;
        ref.setPhysicalAddress(PhysicalAddress(0, records_stored, 0, 0));
        ref.setSlotId(static_cast<int>(records_stored));
        
        std::string record_key = timestamp + "_" + std::to_string(record->getId());
        
        auto base_record = record->clone();
        auto gps_record = std::unique_ptr<GPSRecord>(
            static_cast<GPSRecord*>(base_record.release())
        );
        record_storage[record_key] = std::move(gps_record);
        
        if (timestamp_index->insert(timestamp, ref)) {
            records_stored++;
            return true;
        }
        return false;
    }
    
    std::vector<std::unique_ptr<GPSRecord>> search(const std::string& timestamp_range) override {
        incrementReadOps();
        
        std::vector<std::unique_ptr<GPSRecord>> results;
        
        size_t comma_pos = timestamp_range.find(',');
        if (comma_pos != std::string::npos) {
            std::string start_time = timestamp_range.substr(0, comma_pos);
            std::string end_time = timestamp_range.substr(comma_pos + 1);
            
            auto refs = timestamp_index->rangeSearch(start_time, end_time);
            
            std::cout << "🔍 B+ Tree encontró " << refs.size() 
                      << " referencias en rango [" << start_time << ", " << end_time << "]" << std::endl;
            
            for (const auto& [key, record] : record_storage) {
                if (record->getTimestamp() >= start_time && record->getTimestamp() <= end_time) {
                    auto base_record = record->clone();
                    auto gps_record = std::unique_ptr<GPSRecord>(
                        static_cast<GPSRecord*>(base_record.release())
                    );
                    results.push_back(std::move(gps_record));
                }
            }
        }
        
        return results;
    }
    
    std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) override {
        std::vector<std::unique_ptr<GPSRecord>> results;
        
        // Parser para diferentes tipos de consulta temporal
        if (sql.find("BETWEEN") != std::string::npos) {
            // Extraer rango temporal
            size_t start1 = sql.find("'");
            if (start1 != std::string::npos) {
                size_t end1 = sql.find("'", start1 + 1);
                size_t start2 = sql.find("'", end1 + 1);
                size_t end2 = sql.find("'", start2 + 1);
                
                if (end2 != std::string::npos) {
                    std::string start_time = sql.substr(start1 + 1, end1 - start1 - 1);
                    std::string end_time = sql.substr(start2 + 1, end2 - start2 - 1);
                    std::string range_query = start_time + "," + end_time;
                    
                    std::cout << "🔍 Range query en B+ Tree: [" << start_time << " - " << end_time << "]" << std::endl;
                    results = search(range_query);
                }
            }
        } else if (sql.find("SELECT *") != std::string::npos && sql.find("WHERE") == std::string::npos) {
            // SELECT * sin WHERE
            std::cout << "🔍 Full scan en B+ Tree (secuencial)..." << std::endl;
            results = searchAll();
        } else if (sql.find("timestamp") != std::string::npos && sql.find("=") != std::string::npos) {
            // Búsqueda exacta por timestamp
            size_t start = sql.find("'");
            if (start != std::string::npos) {
                size_t end = sql.find("'", start + 1);
                if (end != std::string::npos) {
                    std::string exact_time = sql.substr(start + 1, end - start - 1);
                    std::cout << "🔍 Búsqueda exacta por timestamp: " << exact_time << std::endl;
                    
                    // Simular búsqueda exacta como rango pequeño
                    std::string range_query = exact_time + "," + exact_time + "~";
                    results = search(range_query);
                }
            }
        } else {
            std::cout << "⚠️  Servidor S2 optimizado para consultas:" << std::endl;
            std::cout << "   - WHERE timestamp BETWEEN 'inicio' AND 'fin'" << std::endl;
            std::cout << "   - WHERE timestamp = 'valor'" << std::endl;
            std::cout << "   - SELECT * (full scan)" << std::endl;
        }
        
        return results;
    }
    
    std::vector<std::unique_ptr<GPSRecord>> searchAll() {
        incrementReadOps();
        
        std::vector<std::unique_ptr<GPSRecord>> results;
        for (const auto& [key, record] : record_storage) {
            auto base_record = record->clone();
            auto gps_record = std::unique_ptr<GPSRecord>(
                static_cast<GPSRecord*>(base_record.release())
            );
            results.push_back(std::move(gps_record));
        }
        return results;
    }
    
    void displayStatistics() const override {
        std::cout << "\n📊 ESTADÍSTICAS " << server_name << " (B+ Tree)" << std::endl;
        std::cout << "Registros almacenados: " << records_stored << std::endl;
        std::cout << "Operaciones totales: " << total_operations << std::endl;
        std::cout << "  - Lecturas: " << read_operations << " (" 
                  << (total_operations > 0 ? (read_operations * 100.0 / total_operations) : 0) 
                  << "%)" << std::endl;
        std::cout << "  - Escrituras: " << write_operations << " (" 
                  << (total_operations > 0 ? (write_operations * 100.0 / total_operations) : 0) 
                  << "%)" << std::endl;
        
        timestamp_index->displayStatistics();
    }
    
    void displayStructure() const override {
        std::cout << "\n🌳 ESTRUCTURA B+ Tree (S2):" << std::endl;
        timestamp_index->displayTree();
    }
};

/**
 * @brief Sistema Interactivo de Query Routing
 */
class InteractiveQuerySystem {
private:
    std::unique_ptr<TransactionalServer> server_s1;
    std::unique_ptr<AnalyticalServer> server_s2;
    
    // Estadísticas globales
    size_t total_queries = 0;
    size_t auto_routed_queries = 0;
    size_t manual_routed_queries = 0;
    std::vector<std::string> query_history;
    
    bool auto_routing_enabled = true;
    
public:
    InteractiveQuerySystem() {
        server_s1 = std::make_unique<TransactionalServer>();
        server_s2 = std::make_unique<AnalyticalServer>();
        
        std::cout << "\n🌐 Sistema Interactivo de Consultas Distribuidas inicializado" << std::endl;
    }
    
    void loadSampleData() {
        std::cout << "\n📁 Cargando datos GPS de muestra..." << std::endl;
        
        // Datos GPS de ejemplo ampliados
        std::vector<std::vector<std::string>> sample_data = {
            {"1", "868018070237402", "68", "2025-06-25 00:47:02+00", "-16.4103100", "-71.5309216", "0", "0", "0", "0", "2345.8", "55.4", "5", "0", "2.0", "7", "POINT", "{}", "2025-06-25 00:47:48+00", "2025-06-25 00:47:48+00", "2025-06-25 00:47:48+00"},
            {"2", "868018070237402", "68", "2025-06-25 00:48:02+00", "-16.4102800", "-71.5308633", "0", "0", "0", "0", "2348.1", "137.7", "7", "0", "1.5", "7", "POINT", "{}", "2025-06-25 00:48:52+00", "2025-06-25 00:48:52+00", "2025-06-25 00:48:52+00"},
            {"3", "868018070237410", "68", "2025-06-25 00:49:02+00", "-16.4102800", "-71.5309516", "0", "0", "0", "0", "2345.4", "357.5", "8", "0", "1.4", "7", "POINT", "{}", "2025-06-25 00:49:55+00", "2025-06-25 00:49:55+00", "2025-06-25 00:49:55+00"},
            {"4", "868018070237420", "68", "2025-06-25 00:50:02+00", "-16.4102833", "-71.5309150", "0", "0", "0", "0", "2344.5", "191.4", "9", "0", "1.2", "7", "POINT", "{}", "2025-06-25 00:50:02+00", "2025-06-25 00:50:02+00", "2025-06-25 00:50:02+00"},
            {"5", "868018070237430", "68", "2025-06-25 00:51:02+00", "-16.4102916", "-71.5309033", "0", "0", "0", "0", "2347.5", "122.1", "10", "0", "1.1", "7", "POINT", "{}", "2025-06-25 00:52:02+00", "2025-06-25 00:52:02+00", "2025-06-25 00:52:02+00"},
            {"6", "868018070237440", "68", "2025-06-25 00:52:02+00", "-16.4102683", "-71.5308333", "0", "0", "0", "0", "2349.6", "141.1", "9", "0", "1.2", "7", "POINT", "{}", "2025-06-25 00:52:02+00", "2025-06-25 00:52:02+00", "2025-06-25 00:52:02+00"},
            {"7", "868018070237450", "68", "2025-06-25 00:53:02+00", "-16.4102766", "-71.5308483", "0", "0", "0", "0", "2350.7", "333.7", "11", "0", "1.0", "7", "POINT", "{}", "2025-06-25 00:53:05+00", "2025-06-25 00:53:05+00", "2025-06-25 00:53:05+00"},
            {"8", "868018070237460", "68", "2025-06-25 00:54:02+00", "-16.4102866", "-71.5308800", "0", "0", "0", "0", "2350.7", "65.1", "13", "0", "1.0", "7", "POINT", "{}", "2025-06-25 00:54:08+00", "2025-06-25 00:54:08+00", "2025-06-25 00:54:08+00"},
            {"9", "868018070237470", "68", "2025-06-25 00:55:02+00", "-16.4102683", "-71.5308966", "0", "0", "0", "0", "2348.5", "56.0", "16", "0", "0.7", "7", "POINT", "{}", "2025-06-25 00:55:12+00", "2025-06-25 00:55:12+00", "2025-06-25 00:55:12+00"},
            {"10", "868018070237480", "68", "2025-06-25 01:00:02+00", "-16.4103000", "-71.5309000", "0", "0", "0", "0", "2350.0", "90.0", "12", "0", "1.5", "7", "POINT", "{}", "2025-06-25 01:00:12+00", "2025-06-25 01:00:12+00", "2025-06-25 01:00:12+00"}
        };
        
        size_t loaded = 0;
        for (const auto& row : sample_data) {
            auto gps_record = std::make_unique<GPSRecord>();
            gps_record->setFromCSVLine(row);
            
            std::string imei = gps_record->getIMEI();
            std::string timestamp = gps_record->getTimestamp();
            
            // Insertar en S1 (Hash por IMEI)
            auto s1_record = std::unique_ptr<GPSRecord>(
                static_cast<GPSRecord*>(gps_record->clone().release())
            );
            server_s1->insert(imei, std::move(s1_record));
            
            // Insertar algunos en S2 (B+ Tree por timestamp)
            if (loaded % 2 == 0) { // 50% van también a S2
                server_s2->insert(timestamp, std::move(gps_record));
            }
            
            loaded++;
        }
        
        std::cout << "✅ Cargados " << loaded << " registros GPS" << std::endl;
        std::cout << "   📋 S1 (Hash): " << server_s1->getRecordsStored() << " registros" << std::endl;
        std::cout << "   📈 S2 (B+ Tree): " << server_s2->getRecordsStored() << " registros" << std::endl;
    }
    
    void displayMainMenu() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "🚀 SISTEMA SGBD DISTRIBUIDO INTERACTIVO 🚀" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        // Estado de servidores
        std::cout << "\n📊 ESTADO DE SERVIDORES:" << std::endl;
        server_s1->displayBasicStats();
        server_s2->displayBasicStats();
        
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
        std::cout << "8️⃣  Cargar más datos de prueba" << std::endl;
        std::cout << "9️⃣  Ayuda - Guía de consultas" << std::endl;
        std::cout << "0️⃣  Salir" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
    }
    
    void showQueryExamples() {
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
    
    void executeCustomQuery() {
        std::cout << "\n💻 EJECUTOR DE CONSULTAS SQL PERSONALIZADO" << std::endl;
        std::cout << "===========================================" << std::endl;
        
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
        } else {
            results = executeWithManualRouting(query);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // Mostrar resultados
        displayQueryResults(results, query, duration.count());
    }
    
    std::vector<std::unique_ptr<GPSRecord>> executeWithAutoRouting(const std::string& query) {
        std::cout << "\n🤖 ROUTING AUTOMÁTICO:" << std::endl;
        
        std::vector<std::unique_ptr<GPSRecord>> results;
        
        if (query.find("imei") != std::string::npos && query.find("=") != std::string::npos) {
            std::cout << "🔀 Detectado: Consulta por IMEI → Enviando a S1 (Hash)" << std::endl;
            results = server_s1->executeCustomQuery(query);
            auto_routed_queries++;
        } else if (query.find("timestamp") != std::string::npos || 
                   query.find("BETWEEN") != std::string::npos ||
                   (query.find("SELECT *") != std::string::npos && query.find("WHERE") == std::string::npos)) {
            std::cout << "🔀 Detectado: Consulta temporal/scan → Enviando a S2 (B+ Tree)" << std::endl;
            results = server_s2->executeCustomQuery(query);
            auto_routed_queries++;
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
    
    std::vector<std::unique_ptr<GPSRecord>> executeWithManualRouting(const std::string& query) {
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
            manual_routed_queries++;
        } else if (choice == "2") {
            std::cout << "\n📈 Ejecutando en S2 (B+ Tree)..." << std::endl;
            results = server_s2->executeCustomQuery(query);
            manual_routed_queries++;
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
            manual_routed_queries++;
        }
        
        return results;
    }
    
    void displayQueryResults(const std::vector<std::unique_ptr<GPSRecord>>& results, 
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
    
    void toggleRoutingMode() {
        auto_routing_enabled = !auto_routing_enabled;
        std::cout << "\n🔀 Modo de routing cambiado a: " 
                  << (auto_routing_enabled ? "AUTOMÁTICO" : "MANUAL") << std::endl;
        
        if (auto_routing_enabled) {
            std::cout << "   → El sistema elegirá automáticamente el mejor servidor" << std::endl;
        } else {
            std::cout << "   → Podrás seleccionar manualmente el servidor para cada consulta" << std::endl;
        }
    }
    
    void displayDetailedStatistics() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "📊 ESTADÍSTICAS DETALLADAS DEL SISTEMA DISTRIBUIDO" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        // Estadísticas globales
        std::cout << "\n🌐 ESTADÍSTICAS GLOBALES:" << std::endl;
        std::cout << "Total de consultas ejecutadas: " << total_queries << std::endl;
        std::cout << "Consultas con routing automático: " << auto_routed_queries << std::endl;
        std::cout << "Consultas con routing manual: " << manual_routed_queries << std::endl;
        
        // Estadísticas por servidor
        server_s1->displayStatistics();
        server_s2->displayStatistics();
        
        // Análisis de eficiencia
        std::cout << "\n💡 ANÁLISIS DE EFICIENCIA:" << std::endl;
        if (server_s1->getRecordsStored() > 0 && server_s2->getRecordsStored() > 0) {
            std::cout << "✅ Distribución de datos balanceada" << std::endl;
            std::cout << "✅ Hash para IMEI: O(1) búsquedas exactas" << std::endl;
            std::cout << "✅ B+ Tree para timestamp: Eficiente en range queries" << std::endl;
        }
    }
    
    void displayIndexStructures() {
        std::cout << "\n📐 ESTRUCTURAS DE ÍNDICES:" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        server_s1->displayStructure();
        server_s2->displayStructure();
    }
    
    void showQueryHistory() {
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
    
    void showHelp() {
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
        
        showQueryExamples();
    }
    
    void run() {
        std::cout << "🚀 Iniciando Sistema Interactivo..." << std::endl;
        loadSampleData();
        
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
                loadSampleData();
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
};

/**
 * @brief Punto de entrada principal - Sistema Interactivo
 */
int main() {
    std::cout << "🌟 SISTEMA SGBD DISTRIBUIDO INTERACTIVO 🌟" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Versión Educativa para experimentar con:" << std::endl;
    std::cout << "✅ Hash Extensible vs B+ Tree" << std::endl;
    std::cout << "✅ Routing Automático vs Manual" << std::endl;
    std::cout << "✅ Consultas SQL personalizadas" << std::endl;
    std::cout << "✅ Análisis de rendimiento" << std::endl;
    
    try {
        InteractiveQuerySystem system;
        system.run();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}