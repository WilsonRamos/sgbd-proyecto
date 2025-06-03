#include "Record.h"

int main() {
    
    FieldDefinition campo1("nombre", FieldType::STRING, 50);
    FieldDefinition campo2("edad", FieldType::INTEGER);
    FieldDefinition campo3("peso",FieldType::FLOAT);


    std::vector<FieldDefinition> schema = {campo1, campo2,campo3};

    std::shared_ptr<FixedRecord> r = std::make_shared<FixedRecord>(1);

    r->setSchema(schema);
    r->setFieldValues({"Juan Perez", "30","56.78"});
    r->calculateFixedSize();

    std::cout << "Registro serializado: " << r->serialize() << std::endl;
   // r->display();
}
