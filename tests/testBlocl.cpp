#include "Block.h"

int main() {
    PhysicalAddress addr(0, 0, 0, 0);
    Block block(addr, 512); // pequeño bloque para pruebas

    FieldDefinition f1("nombre", FieldType::STRING, 20);
    FieldDefinition f2("edad", FieldType::INTEGER);
    std::vector<FieldDefinition> schema = {f1, f2};

    auto rec = std::make_shared<FixedRecord>(1);
    rec->setSchema(schema);
    rec->setFieldValues({"Luis", "25"});
    rec->calculateFixedSize();

    if (block.addRecord(rec)) {
        std::cout << "Registro añadido al bloque." << std::endl;
    }

    block.displayInfo();
    block.displayRecords();
}
