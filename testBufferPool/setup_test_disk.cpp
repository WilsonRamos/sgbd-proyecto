#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "../include/DiskManager.h"
#include "../include/DiskConfig.h"
#include "../include/Record.h"

/**
 * @brief Configuración de disco de prueba para Buffer Pool Testing
 * 
 * Crea un disco con 4 tablas (2 fijas, 2 variables) y datos de prueba
 * para validar el funcionamiento del Buffer Pool Management System
 */

int main() {
    std::cout << "🔧 CONFIGURANDO DISCO DE PRUEBA PARA BUFFER POOL" << std::endl;
    std::cout << "═══════════════════════════════════════════════════" << std::endl;
    
    try {
        // Configurar y crear disco
        DiskConfig config;
        auto disk_manager = std::make_unique<DiskManager>("./test_disk");
        
        std::cout << "📀 Inicializando disco de prueba..." << std::endl;
        if (!disk_manager->initialize(config)) {
            std::cerr << "❌ Error inicializando disco" << std::endl;
            return 1;
        }
        
        // ═══════════════════════════════════════════════════════════════
        // TABLA 1: EMPLEADOS (REGISTROS FIJOS)
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n📋 Creando tabla EMPLEADOS (registros fijos)..." << std::endl;
        std::vector<FieldDefinition> empleados_schema = {
            FieldDefinition("id", FieldType::INTEGER),
            FieldDefinition("nombre", FieldType::STRING, 30),
            FieldDefinition("edad", FieldType::INTEGER),
            FieldDefinition("salario", FieldType::FLOAT)
        };
        
        disk_manager->createTable("empleados", empleados_schema, true);  // true = registros fijos
        
        std::vector<std::vector<std::string>> empleados_data = {
            {"1", "Juan Pérez", "30", "3500.50"},
            {"2", "María García", "25", "4200.75"},
            {"3", "Carlos López", "35", "5000.00"},
            {"4", "Ana Martín", "28", "3800.25"},
            {"5", "Luis Rodríguez", "32", "4500.00"},
            {"6", "Elena Sánchez", "29", "3900.50"},
            {"7", "Miguel Torres", "31", "4100.00"},
            {"8", "Carmen Ruiz", "27", "3700.75"}
        };
        
        for (const auto& empleado : empleados_data) {
            disk_manager->insertRecord("empleados", empleado);
        }
        std::cout << "✅ Tabla EMPLEADOS: " << empleados_data.size() << " registros insertados" << std::endl;
        
        // ═══════════════════════════════════════════════════════════════
        // TABLA 2: PRODUCTOS (REGISTROS FIJOS)
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n📋 Creando tabla PRODUCTOS (registros fijos)..." << std::endl;
        std::vector<FieldDefinition> productos_schema = {
            FieldDefinition("id", FieldType::INTEGER),
            FieldDefinition("nombre", FieldType::STRING, 25),
            FieldDefinition("precio", FieldType::FLOAT),
            FieldDefinition("stock", FieldType::INTEGER)
        };
        
        disk_manager->createTable("productos", productos_schema, true);  // true = registros fijos
        
        std::vector<std::vector<std::string>> productos_data = {
            {"1", "Laptop Dell", "899.99", "15"},
            {"2", "Mouse Logitech", "25.50", "100"},
            {"3", "Teclado Mecánico", "89.99", "45"},
            {"4", "Monitor 24\"", "199.99", "30"},
            {"5", "Webcam HD", "59.99", "75"},
            {"6", "Auriculares", "79.99", "60"},
            {"7", "Tablet Samsung", "299.99", "20"},
            {"8", "Smartphone", "699.99", "25"},
            {"9", "Cable USB-C", "12.99", "200"},
            {"10", "Power Bank", "39.99", "85"}
        };
        
        for (const auto& producto : productos_data) {
            disk_manager->insertRecord("productos", producto);
        }
        std::cout << "✅ Tabla PRODUCTOS: " << productos_data.size() << " registros insertados" << std::endl;
        
        // ═══════════════════════════════════════════════════════════════
        // TABLA 3: CLIENTES (REGISTROS VARIABLES)
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n📋 Creando tabla CLIENTES (registros variables)..." << std::endl;
        std::vector<FieldDefinition> clientes_schema = {
            FieldDefinition("id", FieldType::INTEGER),
            FieldDefinition("nombre", FieldType::STRING, 0),      // Variable length
            FieldDefinition("email", FieldType::STRING, 0),       // Variable length
            FieldDefinition("direccion", FieldType::STRING, 0),   // Variable length
            FieldDefinition("telefono", FieldType::STRING, 0)     // Variable length
        };
        
        disk_manager->createTable("clientes", clientes_schema, false);  // false = registros variables
        
        std::vector<std::vector<std::string>> clientes_data = {
            {"1", "Francisco Javier Hernández", "fj.hernandez@email.com", "Calle Mayor 123, 4º B, Madrid", "912345678"},
            {"2", "Ana", "ana@test.com", "Plaza Sol 1", "600111222"},
            {"3", "Roberto Carlos Fernández Jiménez", "roberto.fernandez.jimenez@empresa.es", "Avenida de la Constitución 45, Bajo Derecha, Sevilla", "955444555"},
            {"4", "Luis", "luis@mail.com", "C/ Corta 2", "677888999"},
            {"5", "María del Carmen González López", "carmen.gonzalez@universidad.edu", "Paseo de la Castellana 150, Oficina 12A, Madrid", "913333444"},
            {"6", "José Antonio Ruiz Martínez", "ja.ruiz@corporativo.com", "Gran Vía 28, 3º Izquierda, Barcelona", "934567890"},
            {"7", "Isabel", "isa@short.net", "Plaza 3", "655123456"}
        };
        
        for (const auto& cliente : clientes_data) {
            disk_manager->insertRecord("clientes", cliente);
        }
        std::cout << "✅ Tabla CLIENTES: " << clientes_data.size() << " registros insertados" << std::endl;
        
        // ═══════════════════════════════════════════════════════════════
        // TABLA 4: ORDENES (REGISTROS VARIABLES)
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n📋 Creando tabla ORDENES (registros variables)..." << std::endl;
        std::vector<FieldDefinition> ordenes_schema = {
            FieldDefinition("id", FieldType::INTEGER),
            FieldDefinition("cliente_id", FieldType::INTEGER),
            FieldDefinition("fecha", FieldType::STRING, 12),      // Fixed: "YYYY-MM-DD"
            FieldDefinition("descripcion", FieldType::STRING, 0), // Variable length
            FieldDefinition("comentarios", FieldType::STRING, 0)  // Variable length
        };
        
        disk_manager->createTable("ordenes", ordenes_schema, false);  // false = registros variables
        
        std::vector<std::vector<std::string>> ordenes_data = {
            {"1", "1", "2024-01-15", "Laptop Dell + Mouse + Teclado", "Cliente VIP - Envío urgente"},
            {"2", "2", "2024-01-16", "Monitor", "Entrega normal"},
            {"3", "3", "2024-01-17", "Smartphone + Auriculares + Power Bank + Cable USB-C", "Descuento aplicado del 10% por cliente corporativo. Facturación a empresa."},
            {"4", "4", "2024-01-18", "Webcam", ""},
            {"5", "1", "2024-01-19", "Tablet Samsung + Cable USB-C", "Segunda compra del cliente"},
            {"6", "5", "2024-01-20", "Mouse Logitech + Teclado Mecánico + Auriculares", "Pedido para oficina universitaria. Requiere factura con datos fiscales específicos."},
            {"7", "2", "2024-01-21", "Power Bank", "Regalo"},
            {"8", "6", "2024-01-22", "Laptop Dell + Monitor 24\" + Webcam HD + Auriculares", "Setup completo de trabajo remoto. Cliente solicita instalación de software específico."},
            {"9", "3", "2024-01-23", "Cable USB-C", "Compra adicional"},
            {"10", "7", "2024-01-24", "Smartphone + Tablet Samsung", "Oferta especial 2x1 aplicada"}
        };
        
        for (const auto& orden : ordenes_data) {
            disk_manager->insertRecord("ordenes", orden);
        }
        std::cout << "✅ Tabla ORDENES: " << ordenes_data.size() << " registros insertados" << std::endl;
        
        // ═══════════════════════════════════════════════════════════════
        // RESUMEN Y ESTADÍSTICAS
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n📊 RESUMEN DEL DISCO DE PRUEBA:" << std::endl;
        std::cout << "════════════════════════════════════════" << std::endl;
        std::cout << "✅ EMPLEADOS:  8 registros (FIJOS)" << std::endl;
        std::cout << "✅ PRODUCTOS: 10 registros (FIJOS)" << std::endl;
        std::cout << "✅ CLIENTES:   7 registros (VARIABLES)" << std::endl;
        std::cout << "✅ ORDENES:   10 registros (VARIABLES)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📦 TOTAL:     35 registros en 4 tablas" << std::endl;
        
        // Mostrar estadísticas del disco
        disk_manager->displayStatistics();
        
        std::cout << "\n🎯 CARACTERÍSTICAS PARA TESTING:" << std::endl;
        std::cout << "• Registros fijos vs variables" << std::endl;
        std::cout << "• Diferentes tamaños de datos" << std::endl;
        std::cout << "• Múltiples bloques por tabla" << std::endl;
        std::cout << "• Datos realistas para pruebas" << std::endl;
        
        std::cout << "\n💾 Disco guardado en: ./test_disk/" << std::endl;
        std::cout << "🚀 ¡Listo para pruebas del Buffer Pool!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

/*
═══════════════════════════════════════════════════════════════════════════════
📋 ESTRUCTURA DE DATOS CREADA:

EMPLEADOS (Fijos - 30+4+4 = ~38 bytes por registro):
- 8 empleados con nombres, edades y salarios

PRODUCTOS (Fijos - 25+4+4 = ~33 bytes por registro):
- 10 productos con precios y stock

CLIENTES (Variables - longitudes diferentes):
- 7 clientes con nombres, emails y direcciones de longitud variable
- Desde registros cortos hasta muy largos

ORDENES (Variables - longitudes diferentes):
- 10 órdenes con descripciones y comentarios de longitud variable
- Desde comentarios vacíos hasta descripciones muy largas

═══════════════════════════════════════════════════════════════════════════════
🎯 PERFECT PARA TESTING:

1. Diferentes tipos de registros (fijos vs variables)
2. Diferentes tamaños que forzarán múltiples bloques
3. Datos realistas para demos comprensibles
4. Suficientes registros para probar evicción LRU
5. Estructura simple pero completa

═══════════════════════════════════════════════════════════════════════════════
*/