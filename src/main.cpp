#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "DiskManager.h"

/**
 * @brief Muestra el menú principal
 */
void showMenu() {
    std::cout << "\n=== SGBD FÍSICO - MENÚ PRINCIPAL ===" << std::endl;
    std::cout << "1.  Inicializar nuevo disco" << std::endl;
    std::cout << "2.  Cargar disco existente" << std::endl;
    std::cout << "3.  Crear tabla" << std::endl;
    std::cout << "4.  Insertar registro manual" << std::endl;
    std::cout << "5.  Cargar desde CSV" << std::endl;
    std::cout << "6.  Buscar registro por ID" << std::endl;
    std::cout << "7.  Eliminar registro" << std::endl;
    std::cout << "8.  Mostrar tabla completa" << std::endl;
    std::cout << "9.  Compactar tabla" << std::endl;
    std::cout << "10. Mostrar estadísticas" << std::endl;
    std::cout << "11. Mostrar estructura de directorios" << std::endl;
    std::cout << "12. Crear datos de prueba" << std::endl;
    std::cout << "0.  Salir" << std::endl;
    std::cout << "Opción: ";
}

/**
 * @brief Crea datos de prueba para demostración
 */
void createTestData() {
    // Crear archivo CSV de empleados
    std::ofstream emp_file("empleados.csv");
    if (emp_file.is_open()) {
        emp_file << "Juan Perez,30,Ingeniero,75000" << std::endl;
        emp_file << "Maria Garcia,28,Analista,65000" << std::endl;
        emp_file << "Carlos Rodriguez,35,Gerente,85000" << std::endl;
        emp_file << "Ana Martinez,32,Desarrolladora,70000" << std::endl;
        emp_file << "Luis Gonzalez,29,Tester,60000" << std::endl;
        emp_file.close();
        std::cout << "Archivo empleados.csv creado." << std::endl;
    }
    
    // Crear archivo CSV de productos
    std::ofstream prod_file("productos.csv");
    if (prod_file.is_open()) {
        prod_file << "Laptop HP,1200.50,Computadoras,20" << std::endl;
        prod_file << "Mouse Logitech,25.99,Accesorios,100" << std::endl;
        prod_file << "Monitor Dell,300.00,Pantallas,15" << std::endl;
        prod_file << "Teclado Mecánico,89.99,Accesorios,50" << std::endl;
        prod_file << "Impresora Canon,150.00,Oficina,8" << std::endl;
        prod_file.close();
        std::cout << "Archivo productos.csv creado." << std::endl;
    }
}

/**
 * @brief Función principal
 */
int main() {
    DiskManager disk_manager("./mi_disco_sgbd");
    std::string input;
    int option;
    
    std::cout << "=== SISTEMA DE GESTIÓN DE BASE DE DATOS FÍSICO ===" << std::endl;
    std::cout << "Implementación educativa basada en el Capítulo 13" << std::endl;
    std::cout << "Almacenamiento Secundario - Database System Implementation" << std::endl;
    
    while (true) {
        showMenu();
        std::cin >> option;
        std::cin.ignore(); // Limpiar buffer
        
        switch (option) {
            case 1: {
                // Inicializar nuevo disco
                std::cout << "Configurando nuevo disco..." << std::endl;
                
                // Configuración personalizable
                std::cout << "¿Usar configuración por defecto? (s/n): ";
                std::getline(std::cin, input);
                
                DiskConfig config;
                if (input != "s" && input != "S") {
                    int platters, surfaces, tracks, sectors, bytes_sector;
                    std::cout << "Número de platos: ";
                    std::cin >> platters;
                    std::cout << "Superficies por plato: ";
                    std::cin >> surfaces;
                    std::cout << "Pistas por superficie: ";
                    std::cin >> tracks;
                    std::cout << "Sectores por pista: ";
                    std::cin >> sectors;
                    std::cout << "Bytes por sector: ";
                    std::cin >> bytes_sector;
                    
                    config = DiskConfig(platters, surfaces, tracks, sectors, bytes_sector);
                }
                
                if (disk_manager.initialize(config)) {
                    std::cout << "Disco inicializado exitosamente." << std::endl;
                } else {
                    std::cout << "Error inicializando el disco." << std::endl;
                }
                break;
            }
            
            case 2: {
                // Cargar disco existente
                if (disk_manager.loadExistingDisk()) {
                    std::cout << "Disco cargado exitosamente." << std::endl;
                } else {
                    std::cout << "Error cargando el disco o no existe." << std::endl;
                }
                break;
            }
            
            case 3: {
                // Crear tabla
                std::string table_name;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                
                std::cout << "Tipo de registro (f=fijo, v=variable): ";
                std::getline(std::cin, input);
                bool use_fixed = (input == "f" || input == "F");
                
                std::vector<FieldDefinition> schema;
                std::cout << "Número de campos: ";
                int num_fields;
                std::cin >> num_fields;
                std::cin.ignore();
                
                for (int i = 0; i < num_fields; ++i) {
                    std::string field_name;
                    int type_int;
                    size_t max_length = 0;
                    
                    std::cout << "Campo " << (i+1) << " - Nombre: ";
                    std::getline(std::cin, field_name);
                    
                    std::cout << "Tipo (0=INTEGER, 1=FLOAT, 2=STRING, 3=DATE): ";
                    std::cin >> type_int;
                    
                    if (type_int == 2) { // STRING
                        std::cout << "Longitud máxima: ";
                        std::cin >> max_length;
                    }
                    std::cin.ignore();
                    
                    FieldType type = static_cast<FieldType>(type_int);
                    schema.emplace_back(field_name, type, max_length);
                }
                
                if (disk_manager.createTable(table_name, schema, use_fixed)) {
                    std::cout << "Tabla creada exitosamente." << std::endl;
                } else {
                    std::cout << "Error creando la tabla." << std::endl;
                }
                break;
            }
            
            case 4: {
                // Insertar registro manual
                std::string table_name;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                
                std::cout << "Valores separados por comas: ";
                std::string values_str;
                std::getline(std::cin, values_str);
                
                // Parsear valores
                std::vector<std::string> values;
                std::istringstream iss(values_str);
                std::string value;
                while (std::getline(iss, value, ',')) {
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    values.push_back(value);
                }
                
                if (disk_manager.insertRecord(table_name, values)) {
                    std::cout << "Registro insertado exitosamente." << std::endl;
                } else {
                    std::cout << "Error insertando el registro." << std::endl;
                }
                break;
            }
            
            case 5: {
                // Cargar desde CSV
                std::string table_name, csv_file;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                std::cout << "Archivo CSV: ";
                std::getline(std::cin, csv_file);
                
                if (disk_manager.loadFromCSV(table_name, csv_file)) {
                    std::cout << "Datos cargados exitosamente." << std::endl;
                } else {
                    std::cout << "Error cargando datos." << std::endl;
                }
                break;
            }
            
            case 6: {
                // Buscar registro por ID
                std::string table_name;
                int record_id;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                std::cout << "ID del registro: ";
                std::cin >> record_id;
                
                auto record = disk_manager.findRecord(table_name, record_id);
                if (record) {
                    std::cout << "Registro encontrado:" << std::endl;
                    record->display();
                } else {
                    std::cout << "Registro no encontrado." << std::endl;
                }
                break;
            }
            
            case 7: {
                // Eliminar registro
                std::string table_name;
                int record_id;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                std::cout << "ID del registro: ";
                std::cin >> record_id;
                
                if (disk_manager.deleteRecord(table_name, record_id)) {
                    std::cout << "Registro eliminado exitosamente." << std::endl;
                } else {
                    std::cout << "Error eliminando el registro." << std::endl;
                }
                break;
            }
            
            case 8: {
                // Mostrar tabla completa
                std::string table_name;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                
                disk_manager.displayTable(table_name);
                break;
            }
            
            case 9: {
                // Compactar tabla
                std::string table_name;
                std::cout << "Nombre de la tabla: ";
                std::getline(std::cin, table_name);
                
                disk_manager.compactTable(table_name);
                break;
            }
            
            case 10: {
                // Mostrar estadísticas
                disk_manager.displayStatistics();
                break;
            }
            
            case 11: {
                // Mostrar estructura de directorios
                disk_manager.showDirectoryStructure();
                break;
            }
            
            case 12: {
                // Crear datos de prueba
                createTestData();
                
                // Sugerir crear tablas de ejemplo
                std::cout << "\nPara probar el sistema, puedes:" << std::endl;
                std::cout << "1. Crear tabla 'empleados' con campos: nombre(STRING,50), edad(INTEGER), puesto(STRING,30), salario(FLOAT)" << std::endl;
                std::cout << "2. Crear tabla 'productos' con campos: nombre(STRING,50), precio(FLOAT), categoria(STRING,20), stock(INTEGER)" << std::endl;
                std::cout << "3. Cargar datos desde empleados.csv y productos.csv" << std::endl;
                break;
            }
            
            case 0: {
                std::cout << "¡Gracias por usar el SGBD Físico!" << std::endl;
                return 0;
            }
            
            default: {
                std::cout << "Opción no válida." << std::endl;
                break;
            }
        }
        
        std::cout << "\nPresiona Enter para continuar...";
        std::cin.ignore();
        std::getline(std::cin, input);
    }
    
    return 0;
}