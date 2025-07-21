#include "include/QueryExecutor.h"
#include "include/Record.h"
#include <iostream>
#include <memory>

/**
 * @brief Demostración del flujo completo de consultas en el SGBD
 * 
 * Este ejemplo muestra cómo se integran todos los componentes:
 * - B+ Tree para indexación
 * - BufferManager para gestión de memoria
 * - DiskManager para almacenamiento persistente
 * - QueryExecutor para coordinar las operaciones
 */

// Clase de ejemplo para registros de empleados
class EmpleadoRecord : public FixedRecord {
public:
    EmpleadoRecord(int id = -1) : FixedRecord(id) {
        // Definir esquema del empleado
        std::vector<FieldDefinition> schema = {
            FieldDefinition("id", FieldType::INTEGER),
            FieldDefinition("nombre", FieldType::STRING, 50),
            FieldDefinition("edad", FieldType::INTEGER),
            FieldDefinition("salario", FieldType::FLOAT),
            FieldDefinition("fecha_ingreso", FieldType::DATE, 12)
        };
        
        setSchema(schema);
        calculateFixedSize();
    }
    
    std::unique_ptr<Record> clone() const override {
        auto cloned = std::make_unique<EmpleadoRecord>(getId());
        cloned->setFieldValues(getFieldValues());
        cloned->setPhysicalAddress(getPhysicalAddress());
        if (isDeleted()) cloned->markAsDeleted();
        return cloned;
    }
    
    void setEmpleadoData(const std::string& nombre, int edad, double salario, const std::string& fecha) {
        std::vector<std::string> values = {
            std::to_string(getId()),
            nombre,
            std::to_string(edad),
            std::to_string(salario),
            fecha
        };
        setFieldValues(values);
    }
};

int main() {
    std::cout << "[*] DEMOSTRACION DEL SGBD INTEGRADO [*]" << std::endl;
    std::cout << "====================================" << std::endl;
    
    try {
        // Inicializar sistema
        QueryExecutor query_executor(4, 16); // B+ Tree orden 4, Buffer pool 16 frames
        
        std::cout << "\n[*] FASE 1: INSERTANDO REGISTROS DE EMPLEADOS" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Insertar varios empleados
        struct EmpleadoData {
            std::string dni;
            std::string nombre;
            int edad;
            double salario;
            std::string fecha_ingreso;
        };
        
        std::vector<EmpleadoData> empleados = {
            {"12345678", "Juan Perez", 30, 75000.50, "2020-01-15"},
            {"23456789", "Maria Garcia", 28, 68000.00, "2019-03-22"},
            {"34567890", "Carlos Rodriguez", 35, 82000.75, "2018-07-10"},
            {"45678901", "Ana Martinez", 32, 71500.25, "2021-02-18"},
            {"56789012", "Luis Gonzalez", 29, 63000.00, "2020-11-05"},
            {"67890123", "Sofia Lopez", 31, 77000.50, "2019-09-12"},
            {"78901234", "Miguel Torres", 33, 79500.75, "2018-12-03"},
            {"89012345", "Elena Ruiz", 27, 65000.00, "2021-06-20"}
        };
        
        // Insertar cada empleado
        for (const auto& emp_data : empleados) {
            auto empleado = std::make_unique<EmpleadoRecord>(std::stoi(emp_data.dni));
            empleado->setEmpleadoData(emp_data.nombre, emp_data.edad, emp_data.salario, emp_data.fecha_ingreso);
            
            bool success = query_executor.insertRecord(emp_data.dni, std::move(empleado));
            if (success) {
                std::cout << "[OK] Empleado insertado: " << emp_data.nombre << " (DNI: " << emp_data.dni << ")" << std::endl;
            } else {
                std::cout << "[ERROR] Error insertando empleado: " << emp_data.nombre << std::endl;
            }
        }
        
        std::cout << "\n[*] FASE 2: CONSULTAS INDIVIDUALES (SELECT)" << std::endl;
        std::cout << "==========================================" << std::endl;
        
        // Realizar búsquedas individuales
        std::vector<std::string> dnis_buscar = {"23456789", "45678901", "78901234", "99999999"};
        
        for (const auto& dni : dnis_buscar) {
            std::cout << "\n" << std::string(50, '-') << std::endl;
            auto empleado = query_executor.selectRecord(dni);
            
            if (empleado) {
                std::cout << "[ENCONTRADO] EMPLEADO:" << std::endl;
                empleado->display();
            } else {
                std::cout << "[NO ENCONTRADO] Empleado con DNI " << dni << " no encontrado" << std::endl;
            }
        }
        
        std::cout << "\n[*] FASE 3: CONSULTAS POR RANGO (RANGE SELECT)" << std::endl;
        std::cout << "==============================================" << std::endl;
        
        // Búsqueda por rango de DNIs
        std::cout << "\n[*] Buscando empleados con DNI entre 30000000 y 70000000:" << std::endl;
        auto empleados_rango = query_executor.selectRange("30000000", "70000000");
        
        std::cout << "\n[*] EMPLEADOS EN EL RANGO:" << std::endl;
        for (const auto& emp : empleados_rango) {
            std::cout << "  - ";
            emp->display();
            std::cout << std::endl;
        }
        
        std::cout << "\n[*] FASE 4: ANALISIS DE RENDIMIENTO" << std::endl;
        std::cout << "===================================" << std::endl;
        
        // Realizar múltiples consultas para analizar rendimiento
        std::cout << "\n[*] Realizando 50 consultas aleatorias para analisis..." << std::endl;
        
        for (int i = 0; i < 50; i++) {
            std::string dni = std::to_string(12345678 + (i % 8) * 11111111);
            auto empleado = query_executor.selectRecord(dni);
            // No mostrar output individual para no saturar
        }
        
        // Mostrar estadísticas finales
        query_executor.displaySystemStatistics();
        
        std::cout << "\n[*] FASE 5: ESTADO FINAL DEL SISTEMA" << std::endl;
        std::cout << "====================================" << std::endl;
        
        query_executor.displaySystemState();
        
        std::cout << "\n[OK] DEMOSTRACION COMPLETADA EXITOSAMENTE" << std::endl;
        std::cout << "=======================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error durante la demostracion: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// Función auxiliar para generar datos de prueba adicionales
void generateTestData(QueryExecutor& executor, int count) {
    std::cout << "\n[*] Generando " << count << " registros de prueba adicionales..." << std::endl;
    
    for (int i = 0; i < count; i++) {
        std::string dni = std::to_string(90000000 + i);
        auto empleado = std::make_unique<EmpleadoRecord>(90000000 + i);
        
        std::string nombre = "Empleado_" + std::to_string(i);
        int edad = 25 + (i % 15);
        double salario = 50000.0 + (i % 10) * 5000.0;
        std::string fecha = "2020-01-01";
        
        empleado->setEmpleadoData(nombre, edad, salario, fecha);
        
        bool success = executor.insertRecord(dni, std::move(empleado));
        (void)success; // Suppress unused variable warning
        if (i % 100 == 0) {
            std::cout << "  Procesados " << i << " registros..." << std::endl;
        }
    }
    
    std::cout << "[OK] Generacion de datos completada" << std::endl;
}
