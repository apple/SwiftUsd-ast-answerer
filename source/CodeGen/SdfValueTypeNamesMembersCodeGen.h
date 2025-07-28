//
//  SdfValueTypeNamesMembersCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef SdfValueTypeNamesMembersCodeGen_h
#define SdfValueTypeNamesMembersCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for wrapping and implicit member expressions of SdfValueTypeName constants
class SdfValueTypeNamesMembersCodeGen: public CodeGenBase<SdfValueTypeNamesMembersCodeGen> {
public:
    SdfValueTypeNamesMembersCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
};

#endif /* SdfValueTypeNamesMembersCodeGen_h */
