#ifndef RECORD_H
#define RECORD_H

#include <map>        // Para std::map (diccionario)
#include <string>     // Para std::string
#include <iostream>   // Para std::cout

/**
 * 📊 CLASE RECORD
 * 
 * Un Record representa una fila de datos en nuestra base de datos.
 * Es como una entrada de un diccionario donde cada clave es el nombre
 * de una columna y cada valor es el dato correspondiente.
 * 
 * Ejemplo:
 * Record persona;
 * persona.data["nombre"] = "Juan";
 * persona.data["edad"] = "25";
 */
class Record {
public:
    // 📊 ATRIBUTOS PÚBLICOS
    std::map<std::string, std::string> data;  // Diccionario de datos
    bool is_deleted;                          // ¿Está marcado como eliminado?
    int record_id;                           // ID único del registro

    // 🏗️ CONSTRUCTORES
    Record();                                           // Constructor vacío
    Record(const std::map<std::string, std::string>& record_data, int id); // Constructor con datos

    // 📝 MÉTODOS PRINCIPALES
    std::string serialize() const;                      // Convertir a texto
    static Record deserialize(const std::string& serialized_data); // Crear desde texto
    
    // 📏 MÉTODOS DE INFORMACIÓN
    int getSize() const;                               // Tamaño en bytes
    void print() const;                                // Mostrar en pantalla
    
    // 🔍 MÉTODOS DE BÚSQUEDA
    bool hasAttribute(const std::string& attribute) const;  // ¿Tiene este atributo?
    std::string getAttribute(const std::string& attribute) const; // Obtener valor
    void setAttribute(const std::string& attribute, const std::string& value); // Cambiar valor
};

#endif // RECORD_H