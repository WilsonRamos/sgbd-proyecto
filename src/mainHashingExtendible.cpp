/**
 * @file mainHashingExtendible_CORREGIDO.cpp
 * @brief Demostración práctica de Hashing Extensible - VERSIÓN CORREGIDA
 * 
 * COMPILACIÓN:
 * g++ -std=c++17 -I./include mainHashingExtendible_CORREGIDO.cpp -o extendible_demo
 * 
 * EJECUCIÓN:
 * ./extendible_demo
 */

#include "../include/ExtendibleHash.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <bitset>  // ← AGREGADO: Header para std::bitset

// Función para pausar y esperar input del usuario
void pausar() {
    std::cout << "\n⏸️  Presiona Enter para continuar...";
    std::cin.get();
    std::cout << std::endl;
}

// Función para limpiar pantalla (multiplataforma)
void limpiarPantalla() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Función auxiliar para mostrar binario
std::string toBinary(int num, int bits = 8) {
    std::bitset<32> binary(num);
    std::string result = binary.to_string();
    return result.substr(32 - bits);  // Tomar últimos 'bits' bits
}

/**
 * @brief Ejemplo 1: Demostración básica siguiendo el material académico
 */
void ejemplo1_basico() {
    std::cout << "🎯 === EJEMPLO 1: DEMOSTRACIÓN BÁSICA ===" << std::endl;
    std::cout << "Siguiendo el ejemplo del material académico" << std::endl;
    std::cout << "Claves: 4, 12, 32, 16, 1, 5, 7, 13" << std::endl;
    std::cout << "Capacidad por bucket: 4 registros" << std::endl;
    
    // Crear hash table con capacidad 4 por bucket
    ExtendibleHash<int, std::string> hash_table(4);
    
    // Datos del ejemplo académico
    std::vector<std::pair<int, std::string>> datos = {
        {4, "Registro_4"}, {12, "Registro_12"}, {32, "Registro_32"}, 
        {16, "Registro_16"}, {1, "Registro_1"}, {5, "Registro_5"}, 
        {7, "Registro_7"}, {13, "Registro_13"}
    };
    
    std::cout << "\n--- INSERTANDO ELEMENTOS PASO A PASO ---" << std::endl;
    
    for (int i = 0; i < datos.size(); ++i) {
        std::cout << "\n🔹 Insertando clave: " << datos[i].first 
                  << " (Binario: " << toBinary(datos[i].first, 8) << ")" << std::endl;
        
        bool success = hash_table.insert(datos[i].first, datos[i].second);
        
        if (success) {
            std::cout << "✅ Inserción exitosa" << std::endl;
        } else {
            std::cout << "❌ Error en inserción" << std::endl;
        }
        
        // Mostrar estado después de cada inserción
        hash_table.displayStructure();
        
        if (i < datos.size() - 1) {
            pausar();
        }
    }
    
    std::cout << "\n🎉 EJEMPLO 1 COMPLETADO" << std::endl;
    hash_table.displayStatistics();
}

/**
 * @brief Ejemplo 2: Búsquedas y operaciones
 */
void ejemplo2_busquedas() {
    std::cout << "\n🔍 === EJEMPLO 2: BÚSQUEDAS Y OPERACIONES ===" << std::endl;
    
    // Crear y poblar hash table
    ExtendibleHash<int, std::string> hash_table(3);
    
    std::vector<std::pair<int, std::string>> datos = {
        {2, "Dato_2"}, {3, "Dato_3"}, {5, "Dato_5"}, {7, "Dato_7"},
        {11, "Dato_11"}, {17, "Dato_17"}, {19, "Dato_19"}, {23, "Dato_23"}
    };
    
    std::cout << "Insertando datos: ";
    for (const auto& dato : datos) {
        hash_table.insert(dato.first, dato.second);
        std::cout << dato.first << " ";
    }
    std::cout << std::endl;
    
    hash_table.displayStructure();
    
    std::cout << "\n--- PRUEBAS DE BÚSQUEDA ---" << std::endl;
    
    // Búsquedas exitosas
    std::vector<int> claves_buscar = {5, 11, 23, 999};
    
    for (int clave : claves_buscar) {
        std::string valor;
        bool encontrado = hash_table.find(clave, valor);
        
        if (encontrado) {
            std::cout << "✅ Clave " << clave << " encontrada: " << valor << std::endl;
        } else {
            std::cout << "❌ Clave " << clave << " NO encontrada" << std::endl;
        }
    }
    
    std::cout << "\n--- PRUEBAS DE ELIMINACIÓN ---" << std::endl;
    
    // Eliminar algunos elementos
    std::vector<int> claves_eliminar = {7, 999, 11};
    
    for (int clave : claves_eliminar) {
        bool eliminado = hash_table.remove(clave);
        if (eliminado) {
            std::cout << "🗑️ Clave " << clave << " eliminada correctamente" << std::endl;
        } else {
            std::cout << "❌ No se pudo eliminar clave " << clave << std::endl;
        }
    }
    
    std::cout << "\n--- ESTADO FINAL ---" << std::endl;
    hash_table.displayStatistics();
}

/**
 * @brief Ejemplo 3: Seguimiento del material académico con función hash mod 7
 */
void ejemplo3_academico() {
    std::cout << "\n📚 === EJEMPLO 3: FUNCIÓN HASH ACADÉMICA (mod 7) ===" << std::endl;
    std::cout << "Implementando el Ejemplo 2 del material académico" << std::endl;
    std::cout << "Función hash: h(x) = x mod 7" << std::endl;
    std::cout << "Claves: 2, 3, 5, 7, 11, 17, 19, 23, 29, 31" << std::endl;
    
    // Nota: Para simplicidad, usamos la implementación estándar
    // En una implementación real, se podría personalizar la función hash
    ExtendibleHash<int, std::string> hash_table(3);
    
    std::vector<std::pair<int, std::string>> datos_academicos = {
        {2, "Valor_2"}, {3, "Valor_3"}, {5, "Valor_5"}, {7, "Valor_7"},
        {11, "Valor_11"}, {17, "Valor_17"}, {19, "Valor_19"}, 
        {23, "Valor_23"}, {29, "Valor_29"}, {31, "Valor_31"}
    };
    
    std::cout << "\n--- ANÁLISIS DE FUNCIÓN HASH MOD 7 ---" << std::endl;
    for (const auto& dato : datos_academicos) {
        int hash_value = dato.first % 7;
        std::cout << "h(" << dato.first << ") = " << dato.first << " mod 7 = " 
                  << hash_value << " (binario: " << toBinary(hash_value, 3) << ")" << std::endl;
    }
    
    std::cout << "\n--- INSERTANDO CON SEGUIMIENTO ---" << std::endl;
    
    for (const auto& dato : datos_academicos) {
        std::cout << "\n🔸 Insertando: " << dato.first 
                  << " (hash mod 7: " << (dato.first % 7) << ")" << std::endl;
        
        hash_table.insert(dato.first, dato.second);
        hash_table.displayStructure();
        pausar();
    }
    
    std::cout << "\n📊 ESTADÍSTICAS FINALES:" << std::endl;
    hash_table.displayStatistics();
}

/**
 * @brief Ejemplo 4: Análisis de rendimiento
 */
void ejemplo4_rendimiento() {
    std::cout << "\n⚡ === EJEMPLO 4: ANÁLISIS DE RENDIMIENTO ===" << std::endl;
    
    const int NUM_ELEMENTOS = 1000;
    const int CAPACIDAD_BUCKET = 4;
    
    ExtendibleHash<int, std::string> hash_table(CAPACIDAD_BUCKET);
    
    // Generar datos aleatorios
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10000);
    
    std::vector<int> claves_test;
    for (int i = 0; i < NUM_ELEMENTOS; ++i) {
        claves_test.push_back(dis(gen));
    }
    
    std::cout << "Insertando " << NUM_ELEMENTOS << " elementos aleatorios..." << std::endl;
    
    // Medir tiempo de inserción
    auto inicio = std::chrono::high_resolution_clock::now();
    
    int insertados = 0;
    for (int clave : claves_test) {
        std::string valor = "Valor_" + std::to_string(clave);
        if (hash_table.insert(clave, valor)) {
            insertados++;
        }
    }
    
    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    std::cout << "\n📈 RESULTADOS DE RENDIMIENTO:" << std::endl;
    std::cout << "Elementos insertados: " << insertados << "/" << NUM_ELEMENTOS << std::endl;
    std::cout << "Tiempo total: " << duracion.count() << " microsegundos" << std::endl;
    std::cout << "Tiempo promedio por inserción: " 
              << (insertados > 0 ? duracion.count() / insertados : 0) << " μs" << std::endl;
    
    // Estadísticas de la estructura
    hash_table.displayStatistics();
    
    // Prueba de búsquedas
    std::cout << "\n--- PRUEBA DE BÚSQUEDAS ---" << std::endl;
    const int NUM_BUSQUEDAS = 100;
    
    inicio = std::chrono::high_resolution_clock::now();
    
    int encontrados = 0;
    for (int i = 0; i < NUM_BUSQUEDAS; ++i) {
        int clave_buscar = claves_test[i % claves_test.size()];
        std::string valor;
        if (hash_table.find(clave_buscar, valor)) {
            encontrados++;
        }
    }
    
    fin = std::chrono::high_resolution_clock::now();
    duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    std::cout << "Búsquedas realizadas: " << NUM_BUSQUEDAS << std::endl;
    std::cout << "Elementos encontrados: " << encontrados << std::endl;
    std::cout << "Tiempo promedio por búsqueda: " 
              << duracion.count() / NUM_BUSQUEDAS << " μs" << std::endl;
}

/**
 * @brief Ejemplo 5: Comparación con consultas de rango
 */
void ejemplo5_limitaciones() {
    std::cout << "\n🚫 === EJEMPLO 5: LIMITACIONES DEL HASH EXTENSIBLE ===" << std::endl;
    std::cout << "Demostrando por qué no es eficiente para consultas de rango" << std::endl;
    
    ExtendibleHash<int, std::string> hash_table(4);
    
    // Insertar datos ordenados
    std::cout << "Insertando números del 1 al 20..." << std::endl;
    for (int i = 1; i <= 20; ++i) {
        hash_table.insert(i, "Dato_" + std::to_string(i));
    }
    
    hash_table.displayStructure();
    
    std::cout << "\n--- CONSULTA DE RANGO INEFICIENTE ---" << std::endl;
    std::cout << "Buscando elementos en rango [5, 10]" << std::endl;
    std::cout << "Con hash table: debemos revisar TODOS los buckets" << std::endl;
    
    // Simular búsqueda de rango (ineficiente)
    std::vector<std::pair<int, std::string>> elementos_en_rango;
    auto todos_elementos = hash_table.getAllElements();
    
    for (const auto& elemento : todos_elementos) {
        if (elemento.first >= 5 && elemento.first <= 10) {
            elementos_en_rango.push_back(elemento);
        }
    }
    
    std::cout << "Elementos encontrados en rango [5,10]: ";
    for (const auto& elem : elementos_en_rango) {
        std::cout << elem.first << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n💡 CONCLUSIÓN:" << std::endl;
    std::cout << "✅ Hash Extensible es excelente para:" << std::endl;
    std::cout << "   - Búsquedas por clave exacta (O(1))" << std::endl;
    std::cout << "   - Inserciones rápidas" << std::endl;
    std::cout << "   - Escalabilidad dinámica" << std::endl;
    std::cout << "\n❌ Hash Extensible NO es bueno para:" << std::endl;
    std::cout << "   - Consultas de rango (requiere escaneo completo)" << std::endl;
    std::cout << "   - Búsquedas por prefijo" << std::endl;
    std::cout << "   - Datos ordenados" << std::endl;
    std::cout << "\n🌟 Para consultas de rango, usar B+ Trees" << std::endl;
}

/**
 * @brief Menú principal interactivo
 */
void mostrarMenu() {
    std::cout << "\n🎯 === DEMOSTRACIÓN DE HASHING EXTENSIBLE ===" << std::endl;
    std::cout << "Basado en el material académico de la Universidad Nacional de San Agustín" << std::endl;
    std::cout << "\nSelecciona un ejemplo:" << std::endl;
    std::cout << "1. 📖 Ejemplo básico (siguiendo material académico)" << std::endl;
    std::cout << "2. 🔍 Búsquedas y operaciones" << std::endl;
    std::cout << "3. 📚 Función hash mod 7 (Ejemplo académico)" << std::endl;
    std::cout << "4. ⚡ Análisis de rendimiento" << std::endl;
    std::cout << "5. 🚫 Limitaciones y comparación" << std::endl;
    std::cout << "6. 🎮 Modo interactivo" << std::endl;
    std::cout << "0. 🚪 Salir" << std::endl;
    std::cout << "\nOpción: ";
}

/**
 * @brief Modo interactivo para experimentar
 */
void modoInteractivo() {
    std::cout << "\n🎮 === MODO INTERACTIVO ===" << std::endl;
    std::cout << "Experimenta con tu propio Hash Extensible" << std::endl;
    
    int capacidad;
    std::cout << "Ingresa la capacidad por bucket (recomendado: 3-5): ";
    std::cin >> capacidad;
    
    ExtendibleHash<int, int> hash_table(capacidad);
    
    int opcion;
    do {
        std::cout << "\n--- OPERACIONES DISPONIBLES ---" << std::endl;
        std::cout << "1. Insertar elemento" << std::endl;
        std::cout << "2. Buscar elemento" << std::endl;
        std::cout << "3. Eliminar elemento" << std::endl;
        std::cout << "4. Mostrar estructura" << std::endl;
        std::cout << "5. Mostrar estadísticas" << std::endl;
        std::cout << "6. Insertar datos de prueba" << std::endl;
        std::cout << "0. Volver al menú principal" << std::endl;
        std::cout << "Opción: ";
        std::cin >> opcion;
        
        switch (opcion) {
            case 1: {
                int clave, valor;
                std::cout << "Clave: ";
                std::cin >> clave;
                std::cout << "Valor: ";
                std::cin >> valor;
                
                if (hash_table.insert(clave, valor)) {
                    std::cout << "✅ Elemento insertado correctamente" << std::endl;
                } else {
                    std::cout << "❌ Error al insertar elemento" << std::endl;
                }
                break;
            }
            case 2: {
                int clave, valor;
                std::cout << "Clave a buscar: ";
                std::cin >> clave;
                
                if (hash_table.find(clave, valor)) {
                    std::cout << "✅ Encontrado: " << clave << " -> " << valor << std::endl;
                } else {
                    std::cout << "❌ Clave no encontrada" << std::endl;
                }
                break;
            }
            case 3: {
                int clave;
                std::cout << "Clave a eliminar: ";
                std::cin >> clave;
                
                if (hash_table.remove(clave)) {
                    std::cout << "✅ Elemento eliminado" << std::endl;
                } else {
                    std::cout << "❌ Clave no encontrada" << std::endl;
                }
                break;
            }
            case 4:
                hash_table.displayStructure();
                break;
            case 5:
                hash_table.displayStatistics();
                break;
            case 6: {
                std::cout << "Insertando datos de prueba: 1, 8, 15, 22, 29..." << std::endl;
                std::vector<int> datos = {1, 8, 15, 22, 29, 36, 43, 50};
                for (int dato : datos) {
                    hash_table.insert(dato, dato * 10);
                }
                std::cout << "✅ Datos insertados" << std::endl;
                break;
            }
        }
        
        if (opcion != 0) {
            pausar();
        }
        
    } while (opcion != 0);
}

/**
 * @brief Función principal
 */
int main() {
    std::cout << "🎓 UNIVERSIDAD NACIONAL DE SAN AGUSTÍN DE AREQUIPA" << std::endl;
    std::cout << "📚 Implementación de Hashing Extensible" << std::endl;
    std::cout << "👨‍💻 Demostración práctica y educativa" << std::endl;
    
    int opcion;
    do {
        mostrarMenu();
        std::cin >> opcion;
        std::cin.ignore(); // Limpiar buffer
        
        switch (opcion) {
            case 1:
                limpiarPantalla();
                ejemplo1_basico();
                break;
            case 2:
                limpiarPantalla();
                ejemplo2_busquedas();
                break;
            case 3:
                limpiarPantalla();
                ejemplo3_academico();
                break;
            case 4:
                limpiarPantalla();
                ejemplo4_rendimiento();
                break;
            case 5:
                limpiarPantalla();
                ejemplo5_limitaciones();
                break;
            case 6:
                limpiarPantalla();
                modoInteractivo();
                break;
            case 0:
                std::cout << "\n👋 ¡Gracias por usar la demostración!" << std::endl;
                std::cout << "🎯 Has aprendido sobre Hashing Extensible" << std::endl;
                break;
            default:
                std::cout << "❌ Opción no válida" << std::endl;
                break;
        }
        
        if (opcion != 0) {
            pausar();
            limpiarPantalla();
        }
        
    } while (opcion != 0);
    
    return 0;
}