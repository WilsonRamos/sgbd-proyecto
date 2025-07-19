/**
 * @file test_hash_housing.cpp
 * @brief Prueba completa del Hash Extensible usando Housing.csv - VERSION ASCII
 * 
 * COMPILAR:
 * g++ -std=c++17 -I./include test_hash_housing.cpp -o test_hash
 * 
 * EJECUTAR:
 * ./test_hash
 * 
 * CONCEPTOS DEMOSTRADOS:
 * 1. Hash Extensible con datos reales
 * 2. Global Depth vs Local Depth
 * 3. División automática de buckets
 * 4. Expansión del directorio
 * 5. Búsquedas O(1)
 */

#include "SimpleHashIndex.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <random>

/**
 * @brief Lector de CSV simple
 */
class CSVReader {
private:
    std::string filename;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> data;

public:
    CSVReader(const std::string& file) : filename(file) {}

    bool loadFile() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[ERROR] No se puede abrir " << filename << std::endl;
            std::cerr << "Asegurate de que housing.csv esta en el directorio actual" << std::endl;
            return false;
        }

        std::string line;
        bool first_line = true;

        while (std::getline(file, line)) {
            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string cell;

            while (std::getline(ss, cell, ',')) {
                // Limpiar espacios
                cell.erase(0, cell.find_first_not_of(" \t"));
                cell.erase(cell.find_last_not_of(" \t") + 1);
                row.push_back(cell);
            }

            if (first_line) {
                headers = row;
                first_line = false;
            } else {
                data.push_back(row);
            }
        }

        file.close();
        std::cout << "[OK] Archivo cargado: " << data.size() << " registros" << std::endl;
        return true;
    }

    size_t getRowCount() const { return data.size(); }
    const std::vector<std::string>& getHeaders() const { return headers; }
    const std::vector<std::vector<std::string>>& getData() const { return data; }

    std::string getRowAsString(size_t index) const {
        if (index >= data.size()) return "";
        
        std::string result;
        for (size_t i = 0; i < data[index].size() && i < headers.size(); ++i) {
            result += headers[i] + ":" + data[index][i];
            if (i < data[index].size() - 1) result += " | ";
        }
        return result;
    }
};

/**
 * @brief Demostración del ejemplo de la presentación
 */
void demonstratePresentationExample() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "DEMOSTRACION DEL EJEMPLO DE LA PRESENTACION" << std::endl;
    std::cout << "Ejemplo de Ana Maria Cuadros Valdivia - UNSA" << std::endl;
    std::cout << "Claves: 4, 12, 32, 16, 1, 5, 7, 13" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Crear hash con buckets pequeños para forzar divisiones
    SimpleHashIndex hash(4);  // Capacidad = 4 como en la presentación
    
    std::vector<int> presentation_keys = {4, 12, 32, 16, 1, 5, 7, 13};
    
    std::cout << "\nINSERTANDO CLAVES PASO A PASO:" << std::endl;
    
    for (size_t i = 0; i < presentation_keys.size(); ++i) {
        int key = presentation_keys[i];
        std::cout << "\n" << std::string(50, '-') << std::endl;
        std::cout << "PASO " << (i + 1) << ": Insertando clave " << key << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        // Mostrar valor hash en binario
        std::cout << "Hash(" << key << ") = " << key << " = " 
                  << std::bitset<8>(key) << " (binario)" << std::endl;
        
        hash.insert(key, "data_" + std::to_string(key));
        hash.printDirectory();
        
        // Pausa para visualización
        std::cout << "Presiona Enter para continuar...";
        std::cin.get();
    }
    
    std::cout << "\nPROBANDO BUSQUEDAS:" << std::endl;
    for (int key : {5, 13, 99}) {  // 99 no existe
        std::cout << "\nBuscando clave " << key << ":" << std::endl;
        HashRecord* result = hash.find(key);
        if (result) {
            std::cout << "[OK] Encontrado: " << result->data << std::endl;
        } else {
            std::cout << "[NO] No encontrado" << std::endl;
        }
    }
    
    hash.printStatistics();
}

/**
 * @brief Prueba con datos del Housing.csv
 */
void testWithHousingData() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "PRUEBA CON DATOS DE HOUSING.CSV" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Cargar datos
    CSVReader csv("housing.csv");
    if (!csv.loadFile()) {
        // Intentar otras ubicaciones comunes
        std::vector<std::string> paths = {"../data/housing.csv", "./data/housing.csv", 
                                         "../housing.csv", "../../data/housing.csv"};
        bool loaded = false;
        for (const auto& path : paths) {
            CSVReader csv_alt(path);
            if (csv_alt.loadFile()) {
                csv = csv_alt;
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            std::cout << "[ERROR] No se encontro housing.csv en ubicaciones comunes" << std::endl;
            return;
        }
    }

    // Mostrar información del dataset
    std::cout << "\nINFORMACION DEL DATASET:" << std::endl;
    std::cout << "Registros: " << csv.getRowCount() << std::endl;
    std::cout << "Columnas: " << csv.getHeaders().size() << std::endl;
    
    std::cout << "\nPrimeras columnas: ";
    const auto& headers = csv.getHeaders();
    for (size_t i = 0; i < std::min((size_t)5, headers.size()); ++i) {
        std::cout << headers[i];
        if (i < std::min((size_t)4, headers.size() - 1)) std::cout << ", ";
    }
    std::cout << std::endl;

    // Crear hash index
    SimpleHashIndex hash(5);  // Buckets más grandes para datos reales
    
    std::cout << "\nINSERTANDO DATOS EN HASH INDEX:" << std::endl;
    
    const auto& data = csv.getData();
    size_t inserted = 0;
    size_t max_to_insert = std::min((size_t)50, data.size());  // Limitar para demo
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < max_to_insert; ++i) {
        // Usar índice de fila como ID
        int id = static_cast<int>(i + 1);
        std::string row_data = csv.getRowAsString(i);
        
        if (hash.insert(id, row_data)) {
            inserted++;
        }
        
        // Mostrar progreso cada 10 registros
        if ((i + 1) % 10 == 0) {
            std::cout << "Insertados: " << (i + 1) << "/" << max_to_insert << std::endl;
            hash.printDirectory();
            std::cout << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "\nINSERCION COMPLETADA:" << std::endl;
    std::cout << "Registros insertados: " << inserted << "/" << max_to_insert << std::endl;
    std::cout << "Tiempo total: " << duration.count() << " microsegundos" << std::endl;
    std::cout << "Promedio por insercion: " << (double)duration.count() / inserted << " us" << std::endl;
    
    hash.printStatistics();
}

/**
 * @brief Benchmark de rendimiento
 */
void benchmarkPerformance() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BENCHMARK DE RENDIMIENTO" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Crear hash con muchos datos
    SimpleHashIndex hash(6);
    
    std::cout << "Insertando 1000 registros..." << std::endl;
    
    // Insertar datos secuenciales
    auto start_insert = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= 1000; ++i) {
        hash.insert(i, "record_" + std::to_string(i));
    }
    auto end_insert = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_insert - start_insert);
    
    std::cout << "[OK] Insercion completada en " << insert_duration.count() << " us" << std::endl;
    std::cout << "Promedio por insercion: " << (double)insert_duration.count() / 1000 << " us" << std::endl;
    
    // Generar claves aleatorias para búsqueda
    std::vector<int> search_keys;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (int i = 0; i < 100; ++i) {
        search_keys.push_back(dis(gen));
    }
    
    // Benchmark de búsqueda en hash
    std::cout << "\nBenchmark de busqueda - Hash Index:" << std::endl;
    int found_hash = 0;
    auto start_hash = std::chrono::high_resolution_clock::now();
    
    for (int key : search_keys) {
        if (hash.find(key)) {
            found_hash++;
        }
    }
    
    auto end_hash = std::chrono::high_resolution_clock::now();
    auto hash_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_hash - start_hash);
    
    // Simular búsqueda secuencial para comparación
    std::cout << "\nBenchmark de busqueda - Busqueda Secuencial (simulada):" << std::endl;
    int found_sequential = 0;
    auto start_seq = std::chrono::high_resolution_clock::now();
    
    for (int key : search_keys) {
        // Simular búsqueda secuencial en 1000 elementos
        for (int i = 1; i <= 1000; ++i) {
            if (i == key) {
                found_sequential++;
                break;
            }
        }
    }
    
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto seq_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_seq - start_seq);
    
    // Mostrar resultados
    std::cout << "\nRESULTADOS DEL BENCHMARK:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "Hash Index:" << std::endl;
    std::cout << "  - Tiempo: " << hash_duration.count() << " us" << std::endl;
    std::cout << "  - Encontrados: " << found_hash << "/100" << std::endl;
    std::cout << "  - Promedio: " << (double)hash_duration.count() / 100 << " us/busqueda" << std::endl;
    
    std::cout << "\nBusqueda Secuencial:" << std::endl;
    std::cout << "  - Tiempo: " << seq_duration.count() << " us" << std::endl;
    std::cout << "  - Encontrados: " << found_sequential << "/100" << std::endl;
    std::cout << "  - Promedio: " << (double)seq_duration.count() / 100 << " us/busqueda" << std::endl;
    
    double speedup = (double)seq_duration.count() / hash_duration.count();
    std::cout << "\nSPEEDUP: " << std::fixed << std::setprecision(2) 
              << speedup << "x mas rapido" << std::endl;
    
    hash.printStatistics();
}

/**
 * @brief Menú interactivo
 */
void showMenu() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "DEMO DEL HASH EXTENSIBLE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "1. Demostracion del ejemplo de la presentacion" << std::endl;
    std::cout << "2. Prueba con datos de Housing.csv" << std::endl;
    std::cout << "0. Salir" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Opcion: ";
}

/**
 * @brief Explicación de conceptos
 */
void explainConcepts() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "CONCEPTOS DEL HASH EXTENSIBLE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n1. QUE ES EL HASH EXTENSIBLE?" << std::endl;
    std::cout << "- Estructura de datos para indexacion rapida" << std::endl;
    std::cout << "- Permite busquedas O(1) para consultas de igualdad" << std::endl;
    std::cout << "- Crece dinamicamente sin rehashing completo" << std::endl;
    std::cout << "- NO es optimo para consultas de rango" << std::endl;
    
    std::cout << "\n2. COMPONENTES PRINCIPALES:" << std::endl;
    std::cout << "- DIRECTORIO: Array de punteros a buckets" << std::endl;
    std::cout << "- BUCKETS: Contenedores de registros" << std::endl;
    std::cout << "- GLOBAL DEPTH: Bits para indexar el directorio" << std::endl;
    std::cout << "- LOCAL DEPTH: Bits que distinguen registros en un bucket" << std::endl;
    
    std::cout << "\n3. ALGORITMO DE INSERCION:" << std::endl;
    std::cout << "1. Calcular indice = hash(clave) & mascara" << std::endl;
    std::cout << "2. Bucket tiene espacio? -> Insertar" << std::endl;
    std::cout << "3. Bucket lleno?" << std::endl;
    std::cout << "   a. Local Depth = Global Depth? -> Duplicar directorio" << std::endl;
    std::cout << "   b. Dividir bucket" << std::endl;
    std::cout << "   c. Redistribuir registros" << std::endl;
    std::cout << "   d. Insertar registro" << std::endl;
    
    std::cout << "\n4. POR QUE BITS MENOS SIGNIFICATIVOS?" << std::endl;
    std::cout << "- Permite duplicacion eficiente del directorio" << std::endl;
    std::cout << "- Distribucion uniforme de claves" << std::endl;
    std::cout << "- Facilita redistribucion tras division" << std::endl;
    
    std::cout << "\n5. VENTAJAS:" << std::endl;
    std::cout << "[+] Busquedas O(1)" << std::endl;
    std::cout << "[+] Crecimiento dinamico" << std::endl;
    std::cout << "[+] Sin rehashing completo" << std::endl;
    std::cout << "[+] Uso eficiente del espacio" << std::endl;
    
    std::cout << "\n6. DESVENTAJAS:" << std::endl;
    std::cout << "[-] No eficiente para rangos" << std::endl;
    std::cout << "[-] Overhead del directorio" << std::endl;
    std::cout << "[-] Complejidad de implementacion" << std::endl;
    
    std::cout << "\nPresiona Enter para continuar...";
    std::cin.get();
}

/**
 * @brief Función principal
 */
int main() {
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "SISTEMA DE HASH EXTENSIBLE - DEMO COMPLETA" << std::endl;
     std::cout << "Usando datos de Housing.csv" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    int option;
    do {
        showMenu();
        std::cin >> option;
        std::cin.ignore();  // Limpiar buffer
        
        switch (option) {
            case 1:
                demonstratePresentationExample();
                break;
            case 2:
                testWithHousingData();
                break;
            case 3:
                benchmarkPerformance();
                break;
            case 4:
                explainConcepts();
                break;
            case 0:
                std::cout << "\nGracias por usar el demo del Hash Extensible!" << std::endl;
                break;
            default:
                std::cout << "[ERROR] Opcion invalida" << std::endl;
        }
        
        if (option != 0) {
            std::cout << "\nPresiona Enter para volver al menu...";
            std::cin.get();
        }
        
    } while (option != 0);
    
    return 0;
}