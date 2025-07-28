//
//  StaticTokensCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#ifndef StaticTokensCodeGen_h
#define StaticTokensCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for exposing static token types and provoding implicit member expressions
class StaticTokensCodeGen: public CodeGenBase<StaticTokensCodeGen> {
public:
    StaticTokensCodeGen(const CodeGenRunner* codeGenRunnder);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeMmFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;    
};

#endif /* StaticTokensCodeGen_h */
