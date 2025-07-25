#ifndef SGBD_DISTRIBUTED_H
#define SGBD_DISTRIBUTED_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>

#include "HashExtendible/ExtensibleHash.h"
#include "BPlusTree/BPlusTree.h"
#include "buffer/BufferManagerClock.h"
#include "DiskManagerExtended.h"
#include "Record.h"
#include "RecordReference.h"

/**
 * @brief Estados del sistema distribuido
 */
enum class DistributedSystemState {
    NOT_INITIALIZED,
    SERVERS_READY,
    DATA_LOADED,
    ERROR_STATE
};

/**
 * @brief Registro GPS especializado
 */
class GPSRecord : public VariableRecord {
public:
    GPSRecord(int id = -1);
    void setupGPSSchema();
    
    // Getters específicos para campos GPS
    std::string getIMEI() const;
    std::string getTimestamp() const;
    std::string getLatitude() const;
    std::string getLongitude() const;
    std::string getAltitude() const;
    std::string getSpeed() const;
    
    void setFromCSVLine(const std::vector<std::string>& csvFields);
    std::unique_ptr<Record> clone() const override;
    void displayGPSInfo() const;
};

/**
 * @brief Servidor especializado base
 */
class SpecializedServer {
protected:
    std::string server_name;
    std::string server_type;
    std::shared_ptr<BufferManagerClock> buffer_manager;
    std::shared_ptr<DiskManagerExtended> disk_manager;
    
    // Estadísticas
    size_t total_operations = 0;
    size_t read_operations = 0;
    size_t write_operations = 0;
    size_t records_stored = 0;

public:
    SpecializedServer(const std::string& name, const std::string& type, int buffer_size = 32);
    virtual ~SpecializedServer() = default;
    
    // Métodos virtuales puros
    virtual bool insert(const std::string& key, std::unique_ptr<GPSRecord> record) = 0;
    virtual std::vector<std::unique_ptr<GPSRecord>> search(const std::string& query) = 0;
    virtual std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) = 0;
    virtual void displayStatistics() const = 0;
    virtual void displayStructure() const = 0;
    virtual std::string getIndexType() const = 0;
    
    // Métodos públicos comunes
    const std::string& getName() const;
    const std::string& getType() const;
    size_t getRecordsStored() const;
    void incrementReadOps();
    void incrementWriteOps();
    void displayBasicStats() const;
};

/**
 * @brief Servidor S1: Hash Extensible por IMEI
 */
class TransactionalServer : public SpecializedServer {
private:
    std::unique_ptr<ExtensibleHash> imei_index;
    
public:
    TransactionalServer();
    std::string getIndexType() const override;
    
    bool insert(const std::string& imei, std::unique_ptr<GPSRecord> record) override;
    std::vector<std::unique_ptr<GPSRecord>> search(const std::string& imei_query) override;
    std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) override;
    void displayStatistics() const override;
    void displayStructure() const override;
};

/**
 * @brief Servidor S2: B+ Tree por Timestamp
 */
class AnalyticalServer : public SpecializedServer {
private:
    std::unique_ptr<BPlusTree<std::string>> timestamp_index;
    std::map<std::string, std::unique_ptr<GPSRecord>> record_storage;
    
public:
    AnalyticalServer();
    std::string getIndexType() const override;
    
    bool insert(const std::string& timestamp, std::unique_ptr<GPSRecord> record) override;
    std::vector<std::unique_ptr<GPSRecord>> search(const std::string& timestamp_range) override;
    std::vector<std::unique_ptr<GPSRecord>> executeCustomQuery(const std::string& sql) override;
    std::vector<std::unique_ptr<GPSRecord>> searchAll();
    void displayStatistics() const override;
    void displayStructure() const override;
};

/**
 * @brief Sistema distribuido interactivo completo
 */
class SGBDDistributed {
private:
    std::unique_ptr<TransactionalServer> server_s1;
    std::unique_ptr<AnalyticalServer> server_s2;
    
    // Estado del sistema
    DistributedSystemState current_state;
    
    // Estadísticas globales
    size_t total_queries = 0;
    size_t auto_routed_queries = 0;
    size_t manual_routed_queries = 0;
    std::vector<std::string> query_history;
    
    bool auto_routing_enabled = true;
    
    // Datos del dataset
    std::string dataset_path;
    size_t total_loaded_records = 0;

public:
    SGBDDistributed(const std::string& data_path = "data/data-GPS.csv");
    
    // === INICIALIZACIÓN ===
    bool initializeServers();
    bool loadGPSDataset();
    bool loadCompleteGPSDataset(const std::string& csv_file);
    
    // === INTERFAZ INTERACTIVA ===
    void run();
    void displayMainMenu();
    void showSystemStatus();
    
    // === CONSULTAS ===
    void executeCustomQuery();
    void showQueryExamples();
    void showHelp();
    
    // === ROUTING ===
    std::vector<std::unique_ptr<GPSRecord>> executeWithAutoRouting(const std::string& query);
    std::vector<std::unique_ptr<GPSRecord>> executeWithManualRouting(const std::string& query);
    void toggleRoutingMode();
    
    // === ESTADÍSTICAS Y ANÁLISIS ===
    void displayDetailedStatistics();
    void displayIndexStructures();
    void showQueryHistory();
    void displayQueryResults(const std::vector<std::unique_ptr<GPSRecord>>& results, 
                           const std::string& query, long long execution_time_us);
    
    // === UTILIDADES ===
    DistributedSystemState getState() const;
    size_t getTotalRecords() const;
    
private:
    // Métodos auxiliares
    std::vector<std::string> parseCSVLine(const std::string& line, char delimiter = ',');
    int countRecordsInFile(const std::string& filename);
    void loadSampleData(); // Datos de respaldo si no hay CSV
};

/**
 * @brief Esquema GPS para integración con SGBDSystemExtended
 */
struct GPSDatasetSchema {
    static std::string getTableName() { return "gps_tracks"; }
    static std::vector<FieldDefinition> getSchema();
    static char getDelimiter() { return ','; }
    static std::string getDescription() { return "Dataset GPS con tracking de vehículos"; }
    static int getExpectedFields() { return 21; }
};

#endif // SGBD_DISTRIBUTED_H