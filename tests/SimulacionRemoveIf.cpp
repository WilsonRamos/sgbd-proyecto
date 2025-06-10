#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

// Simulación paso a paso de cómo funciona std::remove_if
class RemoveIfVisualization {
private:
    struct Record {
        int id;
        bool deleted;
        std::string data;
        
        Record(int i, const std::string& d, bool del = false) 
            : id(i), deleted(del), data(d) {}
        
        bool isDeleted() const { return deleted; }
    };
    
    std::vector<Record> records;
    
public:
    void setupTestData() {
        records = {
            Record(1, "Data1", false),   // Activo
            Record(2, "Data2", true),    // Eliminado
            Record(3, "Data3", false),   // Activo
            Record(4, "Data4", true),    // Eliminado
            Record(5, "Data5", false),   // Activo
            Record(6, "Data6", true),    // Eliminado
            Record(7, "Data7", false)    // Activo
        };
        
        std::cout << "=== DATOS DE PRUEBA INICIALES ===" << std::endl;
        printVector("Estado inicial");
    }
    
    void printVector(const std::string& title) {
        std::cout << "\n" << title << ":" << std::endl;
        std::cout << "Índice:   ";
        for (size_t i = 0; i < records.size(); ++i) {
            std::cout << "[" << i << "]    ";
        }
        std::cout << std::endl;
        
        std::cout << "Record:   ";
        for (const auto& r : records) {
            std::cout << "R" << r.id << "     ";
        }
        std::cout << std::endl;
        
        std::cout << "Estado:   ";
        for (const auto& r : records) {
            std::cout << (r.isDeleted() ? "DEL" : "ACT") << "    ";
        }
        std::cout << std::endl;
        
        std::cout << "Contenido: ";
        for (const auto& r : records) {
            std::cout << r.data.substr(0,3) << "   ";
        }
        std::cout << std::endl;
    }
    
    // Simulación manual de std::remove_if para mostrar el proceso
    void manualRemoveIf() {
        std::cout << "\n=== SIMULACIÓN MANUAL DE std::remove_if ===" << std::endl;
        
        auto lambda = [](const Record& r) { return r.isDeleted(); };
        
        size_t write_pos = 0;  // Posición donde escribir elementos válidos
        size_t read_pos = 0;   // Posición de lectura
        
        std::cout << "\nProceso paso a paso:" << std::endl;
        std::cout << "Lambda: [](const Record& r) { return r.isDeleted(); }" << std::endl;
        std::cout << "Objetivo: Mover elementos NO eliminados al frente" << std::endl;
        
        // Vector copia para mostrar cambios
        std::vector<Record> working_copy = records;
        
        for (read_pos = 0; read_pos < working_copy.size(); ++read_pos) {
            std::cout << "\n--- Iteración " << (read_pos + 1) << " ---" << std::endl;
            std::cout << "Leyendo posición " << read_pos << ": R" << working_copy[read_pos].id;
            std::cout << " (Estado: " << (working_copy[read_pos].isDeleted() ? "ELIMINADO" : "ACTIVO") << ")" << std::endl;
            
            if (!lambda(working_copy[read_pos])) {  // Si NO está eliminado
                std::cout << "  → NO eliminado: copiar a posición " << write_pos << std::endl;
                
                if (write_pos != read_pos) {
                    working_copy[write_pos] = working_copy[read_pos];
                    std::cout << "  → Movimiento: R" << working_copy[read_pos].id 
                              << " desde [" << read_pos << "] hacia [" << write_pos << "]" << std::endl;
                } else {
                    std::cout << "  → Sin movimiento necesario (misma posición)" << std::endl;
                }
                
                write_pos++;
                std::cout << "  → write_pos avanza a: " << write_pos << std::endl;
            } else {
                std::cout << "  → ELIMINADO: saltar (no copiar)" << std::endl;
            }
            
            std::cout << "  → read_pos avanza a: " << (read_pos + 1) << std::endl;
            
            // Mostrar estado actual
            std::cout << "  Estado actual del vector:" << std::endl;
            std::cout << "    Válidos: [0.." << write_pos << "), Basura: [" << write_pos << ".." << working_copy.size() << ")" << std::endl;
            std::cout << "    ";
            for (size_t i = 0; i < working_copy.size(); ++i) {
                if (i == write_pos) std::cout << "| ";
                std::cout << "R" << working_copy[i].id << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << "\n=== RESULTADO FINAL ===" << std::endl;
        std::cout << "new_end apuntaría al índice: " << write_pos << std::endl;
        std::cout << "Elementos válidos: [0.." << write_pos << ")" << std::endl;
        std::cout << "Elementos basura: [" << write_pos << ".." << working_copy.size() << ")" << std::endl;
        
        // Aplicar el resultado a nuestro vector
        records = working_copy;
        printVector("Después de remove_if manual");
        
        // Simular erase
        std::cout << "\nSimulando records.erase(new_end, records.end()):" << std::endl;
        records.erase(records.begin() + write_pos, records.end());
        printVector("Después de erase");
    }
    
    // Usar la función real de STL para comparar
    void realRemoveIf() {
        std::cout << "\n=== USANDO std::remove_if REAL ===" << std::endl;
        
        // Resetear datos
        setupTestData();
        
        std::cout << "\nAplicando std::remove_if..." << std::endl;
        auto new_end = std::remove_if(records.begin(), records.end(),
            [](const Record& r) { return r.isDeleted(); });
        
        std::cout << "new_end está en posición: " << std::distance(records.begin(), new_end) << std::endl;
        printVector("Después de std::remove_if");
        
        std::cout << "\nAplicando erase..." << std::endl;
        records.erase(new_end, records.end());
        printVector("Después de erase");
    }
    
    void demonstrateOffsetRecalculation() {
        std::cout << "\n=== DEMOSTRACIÓN DE RECÁLCULO DE OFFSETS ===" << std::endl;
        
        // Simular offsets antes de compactación
        std::vector<size_t> old_offsets = {64, 164, 284, 394, 509, 624, 739}; // Offsets originales
        std::cout << "\nOffsets ANTES de compactación:" << std::endl;
        std::cout << "R1[64] R2[164-DEL] R3[284] R4[394-DEL] R5[509] R6[624-DEL] R7[739]" << std::endl;
        
        // Después de compactación quedan: R1, R3, R5, R7
        std::vector<size_t> new_offsets;
        size_t used_space = 64; // header_size
        std::vector<size_t> record_sizes = {100, 110, 115, 130}; // Tamaños de R1, R3, R5, R7
        
        std::cout << "\nRecalculando offsets:" << std::endl;
        std::cout << "header_size = 64, used_space = 64" << std::endl;
        
        for (size_t i = 0; i < record_sizes.size(); ++i) {
            std::cout << "\nRegistro " << (i+1) << ":" << std::endl;
            std::cout << "  offset_table.push_back(" << used_space << ")" << std::endl;
            new_offsets.push_back(used_space);
            
            size_t increment = record_sizes[i] + sizeof(size_t);
            std::cout << "  used_space += " << record_sizes[i] << " + " << sizeof(size_t) 
                      << " = " << increment << std::endl;
            used_space += increment;
            std::cout << "  used_space = " << used_space << std::endl;
        }
        
        std::cout << "\nOffsets DESPUÉS de compactación:" << std::endl;
        std::cout << "R1[" << new_offsets[0] << "] R3[" << new_offsets[1] 
                  << "] R5[" << new_offsets[2] << "] R7[" << new_offsets[3] << "]" << std::endl;
        
        std::cout << "\nEspacio total usado:" << std::endl;
        std::cout << "  Antes: " << (739 + 130 + sizeof(size_t)) << " bytes" << std::endl;
        std::cout << "  Después: " << used_space << " bytes" << std::endl;
        std::cout << "  Ahorro: " << ((739 + 130 + sizeof(size_t)) - used_space) << " bytes" << std::endl;
    }
};

int main() {
    RemoveIfVisualization demo;
    
    std::cout << "DEMOSTRACIÓN COMPLETA DEL PROCESO DE COMPACTACIÓN\n" << std::endl;
    
    // Preparar datos
    demo.setupTestData();
    
    // Mostrar cómo funciona remove_if paso a paso
    demo.manualRemoveIf();
    
    // Comparar con la función real
    demo.realRemoveIf();
    
    // Mostrar el recálculo de offsets
    demo.demonstrateOffsetRecalculation();
    
    std::cout << "\n🎯 PUNTOS CLAVE:" << std::endl;
    std::cout << "1. std::remove_if NO elimina, solo reorganiza" << std::endl;
    std::cout << "2. Los elementos válidos se mueven al frente" << std::endl;
    std::cout << "3. erase() elimina físicamente la 'basura'" << std::endl;
    std::cout << "4. Los offsets se recalculan para reflejar la nueva disposición" << std::endl;
    std::cout << "5. El resultado es un bloque completamente compacto" << std::endl;
    
    return 0;
}