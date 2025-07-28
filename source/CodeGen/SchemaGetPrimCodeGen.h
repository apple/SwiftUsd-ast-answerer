//
//  SchemaGetPrimCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef SchemaGetPrimCodeGen_h
#define SchemaGetPrimCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for `Overlay.GetPrim(someUsdSchemaBaseSubclass)` to workaround `someUsdSchemaBaseSubclass.GetPrim()` not always working
// Also provides `Bool.init(_:someUsdSchemaBaseSubclass)`
class SchemaGetPrimCodeGen: public CodeGenBase<SchemaGetPrimCodeGen> {
public:
    SchemaGetPrimCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
};

#endif /* SchemaGetPrimCodeGen */
