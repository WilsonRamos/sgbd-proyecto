#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "../include/PhysicalAddress.h"
#include "../include/DiskConfig.h"
#include "../include/Record.h"
#include "../include/Block.h"
#include "../include/FileSystemSimulator.h"
#include "../include/DiskManager.h"

namespace fs = std::filesystem;

/**
 * @brief Clase para ejecutar tests básicos del SGBD
 */
class BasicTests {
private:
    std::string test_path = "./test_disk";
    int tests_passed = 0;
    int tests_failed = 0;

public:
    /**
     * @brief Ejecuta todos los tests
     */
    void runAllTests() {
        std::cout << "=== INICIANDO TESTS BÁSICOS DEL SGBD FÍSICO ===" << std::endl;
        
        cleanup();
        
        testPhysicalAddress();
        testDiskConfig();
        testFixedRecord();
        testVariableRecord();
        testBlock();
        testFileSystemSimulator();
        testDiskManager();
        
        cleanup();
        
        std::cout << "\n=== RESUMEN DE TESTS ===" << std::endl;
        std::cout << "Tests pasados: " << tests_passed << std::endl;
        std::cout << "Tests fallidos: " << tests_failed << std::endl;
        
        if (tests_failed == 0) {
            std::cout << "🎉 ¡TODOS LOS TESTS PASARON!" << std::endl;
        } else {
            std::cout << "❌ " << tests_failed << " tests fallaron." << std::endl;
        }
    }

private:
    /**
     * @brief Test de PhysicalAddress
     */
    void testPhysicalAddress() {
        std::cout << "\n--- Test: PhysicalAddress ---" << std::endl;
        
        try {
            PhysicalAddress addr1(1, 0, 100, 50);
            PhysicalAddress addr2(1, 0, 100, 51);
            
            // Test de getters
            assert(addr1.getPlatter() == 1);
            assert(addr1.getSurface() == 0);
            assert(addr1.getTrack() == 100);
            assert(addr1.getSector() == 50);
            
            // Test de comparación
            assert(addr1 < addr2);
            assert(!(addr1 == addr2));
            
            // Test de serialización
            std::string addr_str = addr1.toString();
            assert(addr_str == "P1_S0_T100_SEC50");
            
            // Test de paths
            std::string dir_path = addr1.getDirectoryPath();
            std::string file_name = addr1.getSectorFileName();
            assert(dir_path == "platter_1/surface_0/track_100");
            assert(file_name == "sector_50.txt");
            
            passTest("PhysicalAddress");
            
        } catch (const std::exception& e) {
            failTest("PhysicalAddress", e.what());
        }
    }

    /**
     * @brief Test de DiskConfig
     */
    void testDiskConfig() {
        std::cout << "\n--- Test: DiskConfig ---" << std::endl;
        
        try {
            DiskConfig config(2, 2, 1000, 64, 4096);
            
            // Test de cálculos
            assert(config.getNumPlatters() == 2);
            assert(config.getTotalSurfaces() == 4);
            assert(config.getTotalSectors() == 2 * 2 * 1000 * 64);
            assert(config.getTotalCapacity() == 2LL * 2 * 1000 * 64 * 4096);
            
            // Test de validación
            assert(config.isValid());
            
            // Test de formato
            std::string capacity = config.getFormattedCapacity();
            assert(!capacity.empty());
            
            passTest("DiskConfig");
            
        } catch (const std::exception& e) {
            failTest("DiskConfig", e.what());
        }
    }

    /**
     * @brief Test de FixedRecord
     */
    void testFixedRecord() {
        std::cout << "\n--- Test: FixedRecord ---" << std::endl;
        
        try {
            FixedRecord record(1);
            
            // Crear esquema
            std::vector<FieldDefinition> schema = {
                FieldDefinition("id", FieldType::INTEGER, 0),
                FieldDefinition("name", FieldType::STRING, 50),
                FieldDefinition("age", FieldType::INTEGER, 0)
            };
            
            record.setSchema(schema);
            record.calculateFixedSize();
            
            // Test de datos
            std::vector<std::string> values = {"1", "Juan Perez", "30"};
            record.setFieldValues(values);
            
            assert(record.getId() == 1);
            assert(record.getField(0) == "1");
            assert(record.getField(1) == "Juan Perez");
            assert(record.getField(2) == "30");
            assert(record.getSize() > 0);
            
            // Test de serialización
            std::string serialized = record.serialize();
            assert(!serialized.empty());
            
            FixedRecord record2;
            assert(record2.deserialize(serialized));
            assert(record2.getId() == 1);
            
            passTest("FixedRecord");
            
        } catch (const std::exception& e) {
            failTest("FixedRecord", e.what());
        }
    }

    /**
     * @brief Test de VariableRecord
     */
    void testVariableRecord() {
        std::cout << "\n--- Test: VariableRecord ---" << std::endl;
        
        try {
            VariableRecord record(2);
            
            // Crear esquema
            std::vector<FieldDefinition> schema = {
                FieldDefinition("id", FieldType::INTEGER, 0),
                FieldDefinition("description", FieldType::STRING, 0), // Variable
                FieldDefinition("price", FieldType::FLOAT, 0)
            };
            
            record.setSchema(schema);
            
            // Test con diferentes longitudes
            std::vector<std::string> values = {"2", "Producto de prueba con descripción larga", "99.99"};
            record.setFieldValues(values);
            record.calculateOffsets();
            
            assert(record.getId() == 2);
            assert(record.getSize() > 0);
            
            // Test de serialización
            std::string serialized = record.serialize();
            assert(!serialized.empty());
            
            VariableRecord record2;
            assert(record2.deserialize(serialized));
            assert(record2.getId() == 2);
            
            passTest("VariableRecord");
            
        } catch (const std::exception& e) {
            failTest("VariableRecord", e.what());
        }
    }

    /**
     * @brief Test de Block
     */
    void testBlock() {
        std::cout << "\n--- Test: Block ---" << std::endl;
        
        try {
            PhysicalAddress addr(0, 0, 0, 0);
            Block block(addr, 4096);
            
            block.setRelationName("test_table");
            
            // Crear registros de prueba
            auto record1 = std::make_shared<FixedRecord>(1);
            auto record2 = std::make_shared<FixedRecord>(2);
            
            std::vector<FieldDefinition> schema = {
                FieldDefinition("id", FieldType::INTEGER, 0),
                FieldDefinition("name", FieldType::STRING, 20)
            };
            
            record1->setSchema(schema);
            record1->calculateFixedSize();
            record1->setFieldValues({"1", "Test1"});
            
            record2->setSchema(schema);
            record2->calculateFixedSize();
            record2->setFieldValues({"2", "Test2"});
            
            // Test de inserción
            assert(block.canFit(record1));
            assert(block.addRecord(record1));
            assert(block.addRecord(record2));
            assert(block.getRecordCount() == 2);
            
            // Test de búsqueda
            auto found = block.findRecord(1);
            assert(found != nullptr);
            assert(found->getId() == 1);
            
            // Test de eliminación lógica
            assert(block.deleteRecord(1));
            auto deleted = block.findRecord(1);
            assert(deleted == nullptr); // No debe encontrarse
            
            // Test de compactación
            size_t count_before = block.getRecordCount();
            block.compactBlock();
            assert(block.getRecordCount() < count_before);
            
            // Test de serialización
            std::string serialized = block.serialize();
            assert(!serialized.empty());
            
            passTest("Block");
            
        } catch (const std::exception& e) {
            failTest("Block", e.what());
        }
    }

    /**
     * @brief Test de FileSystemSimulator
     */
    void testFileSystemSimulator() {
        std::cout << "\n--- Test: FileSystemSimulator ---" << std::endl;
        
        try {
            FileSystemSimulator fs(test_path);
            DiskConfig config(1, 2, 10, 8, 1024); // Configuración pequeña
            
            // Test de inicialización
            assert(fs.initialize(config));
            assert(fs.isInitialized());
            
            // Test de direcciones válidas
            PhysicalAddress valid_addr(0, 0, 0, 0);
            PhysicalAddress invalid_addr(5, 0, 0, 0); // Plato inexistente
            
            assert(fs.isValidAddress(valid_addr));
            assert(!fs.isValidAddress(invalid_addr));
            
            // Test de escritura/lectura de bloques
            Block write_block(valid_addr, 1024);
            write_block.setRelationName("test");
            
            assert(fs.writeBlock(valid_addr, write_block));
            
            Block read_block(valid_addr, 1024);
            assert(fs.readBlock(valid_addr, read_block));
            assert(read_block.getRelationName() == "test");
            
            // Test de estadísticas
            auto occupied = fs.getOccupiedSectors();
            assert(occupied.size() == 1);
            
            passTest("FileSystemSimulator");
            
        } catch (const std::exception& e) {
            failTest("FileSystemSimulator", e.what());
        }
    }

    /**
     * @brief Test de DiskManager (integración)
     */
    void testDiskManager() {
        std::cout << "\n--- Test: DiskManager ---" << std::endl;
        
        try {
            DiskManager manager(test_path + "_manager");
            DiskConfig config(1, 2, 10, 8, 1024);
            
            // Test de inicialización
            assert(manager.initialize(config));
            
            // Test de creación de tabla
            std::vector<FieldDefinition> schema = {
                FieldDefinition("id", FieldType::INTEGER, 0),
                FieldDefinition("name", FieldType::STRING, 30),
                FieldDefinition("age", FieldType::INTEGER, 0)
            };
            
            assert(manager.createTable("employees", schema, true));
            
            // Test de inserción
            std::vector<std::string> values1 = {"1", "Juan", "25"};
            std::vector<std::string> values2 = {"2", "Maria", "30"};
            
            assert(manager.insertRecord("employees", values1));
            assert(manager.insertRecord("employees", values2));
            
            // Test de búsqueda
            auto record = manager.findRecord("employees", 1);
            assert(record != nullptr);
            assert(record->getId() == 1);
            
            // Test de eliminación
            assert(manager.deleteRecord("employees", 1));
            
            // Verificar eliminación
            auto deleted = manager.findRecord("employees", 1);
            assert(deleted == nullptr);
            
            passTest("DiskManager");
            
        } catch (const std::exception& e) {
            failTest("DiskManager", e.what());
        }
    }

    /**
     * @brief Registra un test como exitoso
     */
    void passTest(const std::string& test_name) {
        std::cout << "✅ " << test_name << " - PASÓ" << std::endl;
        tests_passed++;
    }

    /**
     * @brief Registra un test como fallido
     */
    void failTest(const std::string& test_name, const std::string& error) {
        std::cout << "❌ " << test_name << " - FALLÓ: " << error << std::endl;
        tests_failed++;
    }

    /**
     * @brief Limpia archivos de test
     */
    void cleanup() {
        try {
            if (fs::exists(test_path)) {
                fs::remove_all(test_path);
            }
            if (fs::exists(test_path + "_manager")) {
                fs::remove_all(test_path + "_manager");
            }
        } catch (const std::exception& e) {
            // Ignorar errores de limpieza
        }
    }
};

/**
 * @brief Test de rendimiento básico
 */
void performanceTest() {
    std::cout << "\n=== TEST DE RENDIMIENTO BÁSICO ===" << std::endl;
    
    DiskManager manager("./perf_test");
    DiskConfig config(1, 2, 100, 32, 4096);
    
    if (!manager.initialize(config)) {
        std::cout << "❌ Error inicializando para test de rendimiento" << std::endl;
        return;
    }
    
    // Crear tabla
    std::vector<FieldDefinition> schema = {
        FieldDefinition("id", FieldType::INTEGER, 0),
        FieldDefinition("data", FieldType::STRING, 100)
    };
    
    if (!manager.createTable("perf_table", schema, true)) {
        std::cout << "❌ Error creando tabla para test de rendimiento" << std::endl;
        return;
    }
    
    // Insertar múltiples registros y medir tiempo
    auto start = std::chrono::high_resolution_clock::now();
    
    const int NUM_RECORDS = 100;
    for (int i = 1; i <= NUM_RECORDS; ++i) {
        std::vector<std::string> values = {
            std::to_string(i),
            "Datos de prueba para el registro número " + std::to_string(i)
        };
        manager.insertRecord("perf_table", values);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Insertados " << NUM_RECORDS << " registros en " 
              << duration.count() << " ms" << std::endl;
    std::cout << "Promedio: " << (duration.count() / (double)NUM_RECORDS) 
              << " ms por registro" << std::endl;
    
    // Limpiar
    std::filesystem::remove_all("./perf_test");
}

/**
 * @brief Función principal de tests
 */
int main(int argc, char* argv[]) {
    bool run_performance = false;
    
    // Verificar argumentos
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--performance") {
            run_performance = true;
        }
    }
    
    try {
        BasicTests tests;
        tests.runAllTests();
        
        if (run_performance) {
            performanceTest();
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error fatal en tests: " << e.what() << std::endl;
        return 1;
    }
}