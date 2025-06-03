#include <iostream>
#include <vector>
#include <memory>
#include "../include/DiskManager.h"

/**
 * EJEMPLO PRÁCTICO: Comparación de Registros Fijos vs Variables
 * 
 * Este ejemplo demuestra las diferencias prácticas entre ambos tipos
 * de registros usando datos reales de empleados.
 */

void ejemploRegistrosFijos() {
    std::cout << "\n=== EJEMPLO: REGISTROS DE LONGITUD FIJA ===" << std::endl;
    
    // Configurar disco para la prueba
    DiskManager disk_manager("./disco_fijos");
    DiskConfig config(1, 2, 100, 32, 4096);  // Disco pequeño para pruebas
    disk_manager.initialize(config);
    
    // Definir esquema con campos de tamaño FIJO
    std::vector<FieldDefinition> schema_fijo = {
        {"nombre", FieldType::STRING, 50},      // SIEMPRE 50 bytes
        {"edad", FieldType::INTEGER, 0},        // SIEMPRE 4 bytes  
        {"puesto", FieldType::STRING, 30},      // SIEMPRE 30 bytes
        {"salario", FieldType::FLOAT, 0}        // SIEMPRE 4 bytes
    };
    
    // Crear tabla con registros FIJOS
    disk_manager.createTable("empleados_fijos", schema_fijo, true);
    
    // Datos de prueba con diferentes longitudes
    std::vector<std::vector<std::string>> empleados = {
        {"Juan", "30", "Dev", "75000"},                    // Nombres cortos
        {"Maria del Carmen", "28", "Senior Developer", "85000"},  // Nombres largos
        {"Luis", "35", "PM", "90000"},                     // Muy corto
        {"Ana Gabriela Martinez", "32", "Tech Lead", "95000"}     // Muy largo
    };
    
    // Insertar registros
    for (const auto& emp : empleados) {
        disk_manager.insertRecord("empleados_fijos", emp);
    }
    
    // Mostrar estadísticas
    std::cout << "Tabla creada con registros FIJOS:" << std::endl;
    disk_manager.displayTable("empleados_fijos");
    
    // Cálculo teórico del tamaño fijo
    size_t tamano_fijo = 12 + 50 + 4 + 30 + 4;  // Header + campos
    tamano_fijo = (tamano_fijo + 3) & ~3;        // Alineación a 4 bytes
    std::cout << "Tamaño fijo calculado: " << tamano_fijo << " bytes por registro" << std::endl;
    
    // Análisis de eficiencia
    std::cout << "ANÁLISIS DE EFICIENCIA (Registros Fijos):" << std::endl;
    std::cout << "- Todos los registros ocupan exactamente " << tamano_fijo << " bytes" << std::endl;
    std::cout << "- 'Juan' (4 chars) ocupa lo mismo que 'Ana Gabriela Martinez' (23 chars)" << std::endl;
    std::cout << "- Desperdicio de espacio en campos cortos" << std::endl;
    std::cout << "- Acceso directo: Registro N = offset_base + (N × " << tamano_fijo << ")" << std::endl;
}

void ejemploRegistrosVariables() {
    std::cout << "\n=== EJEMPLO: REGISTROS DE LONGITUD VARIABLE ===" << std::endl;
    
    // Configurar disco para la prueba
    DiskManager disk_manager("./disco_variables");
    DiskConfig config(1, 2, 100, 32, 4096);
    disk_manager.initialize(config);
    
    // Definir esquema con campos VARIABLES (sin tamaño máximo estricto)
    std::vector<FieldDefinition> schema_variable = {
        {"nombre", FieldType::STRING, 0},       // Tamaño variable según contenido
        {"edad", FieldType::INTEGER, 0},        // Fijo: 4 bytes
        {"puesto", FieldType::STRING, 0},       // Tamaño variable según contenido  
        {"salario", FieldType::FLOAT, 0}        // Fijo: 4 bytes
    };
    
    // Crear tabla con registros VARIABLES
    disk_manager.createTable("empleados_variables", schema_variable, false);
    
    // Los mismos datos de prueba
    std::vector<std::vector<std::string>> empleados = {
        {"Juan", "30", "Dev", "75000"},                    
        {"Maria del Carmen", "28", "Senior Developer", "85000"},
        {"Luis", "35", "PM", "90000"},                     
        {"Ana Gabriela Martinez", "32", "Tech Lead", "95000"}
    };
    
    // Insertar registros
    for (const auto& emp : empleados) {
        disk_manager.insertRecord("empleados_variables", emp);
    }
    
    // Mostrar estadísticas
    std::cout << "Tabla creada con registros VARIABLES:" << std::endl;
    disk_manager.displayTable("empleados_variables");
    
    // Análisis de tamaños variables
    std::cout << "ANÁLISIS DE EFICIENCIA (Registros Variables):" << std::endl;
    
    // Calcular tamaños reales de cada registro
    std::vector<size_t> tamanos_reales;
    for (const auto& emp : empleados) {
        size_t tamano = 20;  // Header variable (más complejo)
        tamano += emp[0].length() + 1;  // nombre + null terminator
        tamano += 4;                    // edad (INTEGER)
        tamano += emp[2].length() + 1;  // puesto + null terminator  
        tamano += 4;                    // salario (FLOAT)
        tamanos_reales.push_back(tamano);
        
        std::cout << "- '" << emp[0] << "': " << tamano << " bytes" << std::endl;
    }
    
    // Comparar con registros fijos
    size_t tamano_fijo = 100;  // Aproximado para comparación
    size_t total_fijo = empleados.size() * tamano_fijo;
    size_t total_variable = 0;
    for (size_t t : tamanos_reales) total_variable += t;
    
    std::cout << "COMPARACIÓN:" << std::endl;
    std::cout << "- Total registros fijos: " << total_fijo << " bytes" << std::endl;
    std::cout << "- Total registros variables: " << total_variable << " bytes" << std::endl;
    std::cout << "- Ahorro de espacio: " << (100.0 * (total_fijo - total_variable) / total_fijo) << "%" << std::endl;
}

void demostracionTablaOffsets() {
    std::cout << "\n=== DEMOSTRACIÓN: TABLA DE OFFSETS ===" << std::endl;
    
    // Simular un registro variable en memoria
    std::cout << "Simulando registro variable 'Ana Gabriela Martinez':" << std::endl;
    
    std::string nombre = "Ana Gabriela Martinez";
    int edad = 32;
    std::string puesto = "Tech Lead";
    float salario = 95000.0f;
    
    // Calcular offsets como lo hace la clase VariableRecord
    std::vector<size_t> offsets;
    size_t offset_actual = 20;  // Después del header
    
    // Offset del nombre
    offsets.push_back(offset_actual);
    offset_actual += nombre.length() + 1;
    
    // Offset de la edad  
    offsets.push_back(offset_actual);
    offset_actual += sizeof(int);
    
    // Offset del puesto
    offsets.push_back(offset_actual);
    offset_actual += puesto.length() + 1;
    
    // Offset del salario
    offsets.push_back(offset_actual);
    offset_actual += sizeof(float);
    
    size_t tamano_total = offset_actual;
    
    std::cout << "TABLA DE OFFSETS:" << std::endl;
    std::cout << "┌─────────────┬─────────┬──────────────┬─────────────┐" << std::endl;
    std::cout << "│ Campo       │ Offset  │ Tamaño       │ Contenido   │" << std::endl;
    std::cout << "├─────────────┼─────────┼──────────────┼─────────────┤" << std::endl;
    std::cout << "│ Header      │      0  │ 20 bytes     │ Metadatos   │" << std::endl;
    std::cout << "│ nombre      │     " << std::setw(2) << offsets[0] << "  │ " 
              << std::setw(2) << (nombre.length() + 1) << " bytes     │ " << nombre << " │" << std::endl;
    std::cout << "│ edad        │     " << std::setw(2) << offsets[1] << "  │  4 bytes     │ " 
              << edad << "        │" << std::endl;
    std::cout << "│ puesto      │     " << std::setw(2) << offsets[2] << "  │ " 
              << std::setw(2) << (puesto.length() + 1) << " bytes     │ " << puesto << "   │" << std::endl;
    std::cout << "│ salario     │     " << std::setw(2) << offsets[3] << "  │  4 bytes     │ " 
              << salario << "     │" << std::endl;
    std::cout << "└─────────────┴─────────┴──────────────┴─────────────┘" << std::endl;
    std::cout << "Tamaño total del registro: " << tamano_total << " bytes" << std::endl;
    
    std::cout << "\nPROCESO DE ACCESO A CAMPO:" << std::endl;
    std::cout << "1. Para leer 'puesto', ir a offset " << offsets[2] << std::endl;
    std::cout << "2. Leer desde offset " << offsets[2] << " hasta encontrar '\\0'" << std::endl;
    std::cout << "3. Resultado: '" << puesto << "'" << std::endl;
}

void comparacionRendimiento() {
    std::cout << "\n=== COMPARACIÓN DE RENDIMIENTO ===" << std::endl;
    
    std::cout << "REGISTROS FIJOS:" << std::endl;
    std::cout << "✅ Acceso directo: O(1)" << std::endl;
    std::cout << "✅ Cálculo simple: offset = registro_id × tamaño_fijo" << std::endl;
    std::cout << "✅ Predicible: Siempre el mismo tamaño" << std::endl;
    std::cout << "✅ Eficiente: Sin overhead de metadatos por campo" << std::endl;
    std::cout << "❌ Desperdicio: Espacio no utilizado en campos cortos" << std::endl;
    std::cout << "❌ Inflexible: No se adapta al contenido real" << std::endl;
    
    std::cout << "\nREGISTROS VARIABLES:" << std::endl;
    std::cout << "✅ Eficiencia de espacio: Solo usa lo necesario" << std::endl;
    std::cout << "✅ Flexibilidad: Se adapta al contenido" << std::endl;
    std::cout << "✅ Escalabilidad: Mejor para datos heterogéneos" << std::endl;
    std::cout << "❌ Complejidad: Requiere tabla de offsets" << std::endl;
    std::cout << "❌ Acceso indirecto: Debe consultar tabla primero" << std::endl;
    std::cout << "❌ Overhead: Metadatos adicionales por registro" << std::endl;
    
    std::cout << "\nRECOMENDACIONES DE USO:" << std::endl;
    std::cout << "USAR REGISTROS FIJOS cuando:" << std::endl;
    std::cout << "- Los datos tienen tamaños similares" << std::endl;
    std::cout << "- Se requiere acceso muy rápido" << std::endl;
    std::cout << "- La simplicidad es importante" << std::endl;
    std::cout << "- Ejemplo: Tablas de contadores, IDs, fechas" << std::endl;
    
    std::cout << "\nUSAR REGISTROS VARIABLES cuando:" << std::endl;
    std::cout << "- Los datos varían mucho en tamaño" << std::endl;
    std::cout << "- El espacio en disco es limitado" << std::endl;
    std::cout << "- Se tienen campos opcionales (NULLs)" << std::endl;
    std::cout << "- Ejemplo: Tablas de texto, comentarios, descripciones" << std::endl;
}

int main() {
    std::cout << "=== DEMOSTRACIÓN: REGISTROS FIJOS vs VARIABLES ===" << std::endl;
    
    // Ejecutar ejemplos
    ejemploRegistrosFijos();
    ejemploRegistrosVariables();
    demostracionTablaOffsets();
    comparacionRendimiento();
    
    return 0;
}