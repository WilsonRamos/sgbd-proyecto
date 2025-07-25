#include "../include/SGBDDistributed.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

// ============================================================================
// IMPLEMENTACIÓN GPSRecord
// ============================================================================

GPSRecord::GPSRecord(int id) : VariableRecord(id) {
    setupGPSSchema();
}

void GPSRecord::setupGPSSchema() {
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

std::string GPSRecord::getIMEI() const { return getField(1); }
std::string GPSRecord::getTimestamp() const { return getField(3); }
std::string GPSRecord::getLatitude() const { return getField(4); }
std::string GPSRecord::getLongitude() const { return getField(5); }
std::string GPSRecord::getAltitude() const { return getField(10); }
std::string GPSRecord::getSpeed() const { return getField(13); }

void GPSRecord::setFromCSVLine(const std::vector<std::string>& csvFields) {
    if (csvFields.size() >= 21) {
        setFieldValues(csvFields);
        setId(std::stoi(csvFields[0]));
        calculateOffsets();
    }
}

std::unique_ptr<Record> GPSRecord::clone() const {
    auto cloned = std::make_unique<GPSRecord>(getId());
    cloned->setFieldValues(this->getFieldValues());
    cloned->calculateOffsets();
    return cloned;
}

void GPSRecord::displayGPSInfo() const {
    std::cout << "GPS[ID:" << getId() 
              << ", IMEI:" << getIMEI()
              << ", Time:" << getTimestamp()
              << ", Lat:" << getLatitude() 
              << ", Lon:" << getLongitude()
              << ", Alt:" << getAltitude() << "m"
              << ", Speed:" << getSpeed() << "km/h]";
}

// ============================================================================
// IMPLEMENTACIÓN SpecializedServer
// ============================================================================

SpecializedServer::SpecializedServer(const std::string& name, const std::string& type, int buffer_size) 
    : server_name(name), server_type(type) {
    
    disk_manager = std::make_shared<DiskManagerExtended>("disk_" + name);
    buffer_manager = std::make_shared<BufferManagerClock>(buffer_size, disk_manager.get());
    
    std::cout << "🖥️  Servidor " << server_name << " (" << server_type << ") inicializado" << std::endl;
}

const std::string& SpecializedServer::getName() const { return server_name; }
const std::string& SpecializedServer::getType() const { return server_type; }
size_t SpecializedServer::getRecordsStored() const { return records_stored; }

void SpecializedServer::incrementReadOps() { read_operations++; total_operations++; }
void SpecializedServer::incrementWriteOps() { write_operations++; total_operations++; }

void SpecializedServer::displayBasicStats() const {
    std::cout << "📊 " << server_name << " - " << getIndexType() 
              << " | Registros: " << records_stored 
              << " | Ops: " << total_operations 
              << " (R:" << read_operations << ", W:" << write_operations << ")" << std::endl;
}

// ============================================================================
// IMPLEMENTACIÓN TransactionalServer
// ============================================================================

TransactionalServer::TransactionalServer() : SpecializedServer("S1", "Transaccional") {
    imei_index = std::make_unique<ExtensibleHash>(4);
    std::cout << "📋 S1: Hash Extensible por IMEI (Búsquedas exactas O(1))" << std::endl;
}

std::string TransactionalServer::getIndexType() const { return "Hash Extensible"; }

bool TransactionalServer::insert(const std::string& imei, std::unique_ptr<GPSRecord> record) {
    incrementWriteOps();
    
    if (imei_index->insert(imei, std::move(record))) {
        records_stored++;
        return true;
    }
    return false;
}

std::vector<std::unique_ptr<GPSRecord>> TransactionalServer::search(const std::string& imei_query) {
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

std::vector<std::unique_ptr<GPSRecord>> TransactionalServer::executeCustomQuery(const std::string& sql) {
    std::vector<std::unique_ptr<GPSRecord>> results;
    
    if (sql.find("imei") != std::string::npos && sql.find("=") != std::string::npos) {
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

void TransactionalServer::displayStatistics() const {
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

void TransactionalServer::displayStructure() const {
    std::cout << "\n🏗️ ESTRUCTURA Hash Extensible (S1):" << std::endl;
    imei_index->displayStructure();
}

// ============================================================================
// IMPLEMENTACIÓN AnalyticalServer
// ============================================================================

AnalyticalServer::AnalyticalServer() : SpecializedServer("S2", "Analítico", 16) {
    timestamp_index = std::make_unique<BPlusTree<std::string>>(3);
    std::cout << "📈 S2: B+ Tree por Timestamp (Range queries eficientes)" << std::endl;
}

std::string AnalyticalServer::getIndexType() const { return "B+ Tree"; }

bool AnalyticalServer::insert(const std::string& timestamp, std::unique_ptr<GPSRecord> record) {
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

std::vector<std::unique_ptr<GPSRecord>> AnalyticalServer::search(const std::string& timestamp_range) {
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

std::vector<std::unique_ptr<GPSRecord>> AnalyticalServer::executeCustomQuery(const std::string& sql) {
    std::vector<std::unique_ptr<GPSRecord>> results;
    
    if (sql.find("BETWEEN") != std::string::npos) {
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
        std::cout << "🔍 Full scan en B+ Tree (secuencial)..." << std::endl;
        results = searchAll();
    } else if (sql.find("timestamp") != std::string::npos && sql.find("=") != std::string::npos) {
        size_t start = sql.find("'");
        if (start != std::string::npos) {
            size_t end = sql.find("'", start + 1);
            if (end != std::string::npos) {
                std::string exact_time = sql.substr(start + 1, end - start - 1);
                std::cout << "🔍 Búsqueda exacta por timestamp: " << exact_time << std::endl;
                
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

std::vector<std::unique_ptr<GPSRecord>> AnalyticalServer::searchAll() {
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

void AnalyticalServer::displayStatistics() const {
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

void AnalyticalServer::displayStructure() const {
    std::cout << "\n🌳 ESTRUCTURA B+ Tree (S2):" << std::endl;
    timestamp_index->displayTree();
}

// ============================================================================
// IMPLEMENTACIÓN SGBDDistributed
// ============================================================================

SGBDDistributed::SGBDDistributed(const std::string& data_path) 
    : current_state(DistributedSystemState::NOT_INITIALIZED)
    , dataset_path(data_path) {
}

bool SGBDDistributed::initializeServers() {
    std::cout << "\n🚀 Inicializando servidores distribuidos..." << std::endl;
    
    try {
        server_s1 = std::make_unique<TransactionalServer>();
        server_s2 = std::make_unique<AnalyticalServer>();
        
        current_state = DistributedSystemState::SERVERS_READY;
        
        std::cout << "\n🌐 Sistema distribuido inicializado exitosamente" << std::endl;
        std::cout << "   📋 S1: Hash Extensible para IMEI" << std::endl;
        std::cout << "   📈 S2: B+ Tree para Timestamp" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error inicializando servidores: " << e.what() << std::endl;
        current_state = DistributedSystemState::ERROR_STATE;
        return false;
    }
}

bool SGBDDistributed::loadCompleteGPSDataset(const std::string& csv_file) {
    std::cout << "\n📁 Cargando dataset GPS completo desde: " << csv_file << std::endl;
    
    std::ifstream file(csv_file);
    if (!file.is_open()) {
        std::cout << "❌ Error: No se pudo abrir " << csv_file << std::endl;
        std::cout << "   Asegúrate de que el archivo existe en la ruta correcta" << std::endl;
        return false;
    }
    
    // Contar registros total
    int total_records = countRecordsInFile(csv_file);
    std::cout << "📊 Registros detectados en dataset: " << total_records << std::endl;
    
    if (total_records == 0) {
        std::cout << "⚠️  Dataset vacío, cargando datos de muestra..." << std::endl;
        loadSampleData();
        return true;
    }
    
    // Leer archivo línea por línea
    std::string line;
    std::getline(file, line); // Saltar header
    
    size_t loaded_s1 = 0, loaded_s2 = 0;
    size_t line_number = 1;
    
    while (std::getline(file, line)) {
        line_number++;
        
        if (line.empty()) continue;
        
        std::vector<std::string> fields = parseCSVLine(line);
        
        if (fields.size() >= 21) {
            auto gps_record = std::make_unique<GPSRecord>();
            gps_record->setFromCSVLine(fields);
            
            std::string imei = gps_record->getIMEI();
            std::string timestamp = gps_record->getTimestamp();
            
            // Insertar en S1 (todos los registros)
            auto s1_record = std::unique_ptr<GPSRecord>(
                static_cast<GPSRecord*>(gps_record->clone().release())
            );
            if (server_s1->insert(imei, std::move(s1_record))) {
                loaded_s1++;
            }
            
            // Insertar en S2 (distribución estratégica: 30% para análisis temporal)
            std::hash<std::string> hasher;
            if (hasher(imei) % 10 < 3) { // 30% de los registros
                if (server_s2->insert(timestamp, std::move(gps_record))) {
                    loaded_s2++;
                }
            }
            
            // Progreso cada 1000 registros
            if ((loaded_s1 + loaded_s2) % 1000 == 0) {
                std::cout << "📈 Procesados: " << (loaded_s1 + loaded_s2) 
                          << " registros (línea " << line_number << ")" << std::endl;
            }
        } else {
            std::cout << "⚠️  Línea " << line_number << " con formato incorrecto (campos: " 
                      << fields.size() << ")" << std::endl;
        }
    }
    
    file.close();
    
    total_loaded_records = loaded_s1; // S1 tiene todos los registros
    current_state = DistributedSystemState::DATA_LOADED;
    
    std::cout << "\n✅ Carga del dataset GPS completada:" << std::endl;
    std::cout << "   📋 S1 (Hash): " << loaded_s1 << " registros" << std::endl;
    std::cout << "   📈 S2 (B+ Tree): " << loaded_s2 << " registros" << std::endl;
    std::cout << "   📊 Total único: " << total_loaded_records << " registros GPS" << std::endl;
    
    return true;
}

bool SGBDDistributed::loadGPSDataset() {
    return loadCompleteGPSDataset(dataset_path);
}

void SGBDDistributed::loadSampleData() {
    std::cout << "\n📁 Cargando datos GPS de muestra..." << std::endl;
    
    // Datos de muestra ampliados
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
        
        auto s1_record = std::unique_ptr<GPSRecord>(
            static_cast<GPSRecord*>(gps_record->clone().release())
        );
        server_s1->insert(imei, std::move(s1_record));
        
        if (loaded % 2 == 0) {
            server_s2->insert(timestamp, std::move(gps_record));
        }
        
        loaded++;
    }
    
    total_loaded_records = loaded;
    current_state = DistributedSystemState::DATA_LOADED;
    
    std::cout << "✅ Cargados " << loaded << " registros de muestra" << std::endl;
}

// Continúa en la siguiente parte...

// ============================================================================
// MÉTODOS AUXILIARES
// ============================================================================

std::vector<std::string> SGBDDistributed::parseCSVLine(const std::string& line, char delimiter) {
    std::vector<std::string> values;
    std::istringstream lineStream(line);
    std::string field;
    bool in_quotes = false;
    std::string current_field;
    
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == delimiter && !in_quotes) {
            values.push_back(current_field);
            current_field.clear();
        } else {
            current_field += c;
        }
    }
    values.push_back(current_field); // Último campo
    
    // Limpiar comillas
    for (auto& value : values) {
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }
    }
    
    return values;
}

int SGBDDistributed::countRecordsInFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return 0;
    
    int count = 0;
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (first_line) {
            first_line = false; // Saltar header
            continue;
        }
        if (!line.empty()) count++;
    }
    
    file.close();
    return count;
}

DistributedSystemState SGBDDistributed::getState() const { 
    return current_state; 
}

size_t SGBDDistributed::getTotalRecords() const { 
    return total_loaded_records; 
}

// ============================================================================
// IMPLEMENTACIÓN GPSDatasetSchema
// ============================================================================

std::vector<FieldDefinition> GPSDatasetSchema::getSchema() {
    return {
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
}