#include "record.h"
#include <sstream>   // Para std::istringstream (leer strings como archivos)

// 🏗️ CONSTRUCTOR POR DEFECTO
Record::Record() : is_deleted(false), record_id(-1) {
    // Inicializamos con valores seguros
    // is_deleted = false (no está eliminado)
    // record_id = -1 (ID inválido, debe ser asignado después)
}

// 🏗️ CONSTRUCTOR CON DATOS
Record::Record(const std::map<std::string, std::string>& record_data, int id) 
    : data(record_data), is_deleted(false), record_id(id) {
    // Copiamos los datos, marcamos como no eliminado, asignamos ID
}

// 📝 SERIALIZACIÓN - Convertir objeto a texto
std::string Record::serialize() const {
    std::string result = std::to_string(record_id) + "|";
    result += (is_deleted ? "1" : "0");  // 1 = eliminado, 0 = activo
    result += "|";
    
    // Agregar cada par clave:valor separado por ;
    for (const auto& pair : data) {
        result += pair.first + ":" + pair.second + ";";
    }
    
    return result;
}

// 📝 DESERIALIZACIÓN - Crear objeto desde texto
Record Record::deserialize(const std::string& serialized_data) {
    Record record;
    std::istringstream iss(serialized_data);
    std::string token;
    
    // Leer ID (hasta el primer |)
    std::getline(iss, token, '|');
    record.record_id = std::stoi(token);  // std::stoi convierte string a int
    
    // Leer estado de eliminación (hasta el segundo |)
    std::getline(iss, token, '|');
    record.is_deleted = (token == "1");
    
    // Leer datos (resto del string)
    std::getline(iss, token, '|');
    std::istringstream data_stream(token);
    std::string pair;
    
    // Procesar cada par clave:valor
    while (std::getline(data_stream, pair, ';') && !pair.empty()) {
        size_t colon_pos = pair.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = pair.substr(0, colon_pos);
            std::string value = pair.substr(colon_pos + 1);
            record.data[key] = value;
        }
    }
    
    return record;
}

// 📏 CALCULAR TAMAÑO EN BYTES
int Record::getSize() const {
    return static_cast<int>(serialize().length());
}

// 🖨️ MOSTRAR REGISTRO EN PANTALLA
void Record::print() const {
    std::cout << "📊 Record ID: " << record_id;
    std::cout << " (Estado: " << (is_deleted ? "❌ Eliminado" : "✅ Activo") << ")" << std::endl;
    
    for (const auto& pair : data) {
        std::cout << "   🔹 " << pair.first << ": " << pair.second << std::endl;
    }
}

// 🔍 VERIFICAR SI TIENE UN ATRIBUTO
bool Record::hasAttribute(const std::string& attribute) const {
    return data.find(attribute) != data.end();
}

// 🔍 OBTENER VALOR DE UN ATRIBUTO
std::string Record::getAttribute(const std::string& attribute) const {
    auto it = data.find(attribute);
    if (it != data.end()) {
        return it->second;
    }
    return "";  // Retorna string vacío si no existe
}

// ✏️ CAMBIAR VALOR DE UN ATRIBUTO
void Record::setAttribute(const std::string& attribute, const std::string& value) {
    data[attribute] = value;
}