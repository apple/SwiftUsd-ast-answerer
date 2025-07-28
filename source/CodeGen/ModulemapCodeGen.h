/*
//
//  ModulemapCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/22/24.
//

#ifndef ModulemapCodeGen_h
#define ModulemapCodeGen_h

#include "CodeGen/CodeGenBase.h"

struct ModulemapModule;

// Code gen for the modulemap file
class ModulemapCodeGen: public CodeGenBase<ModulemapCodeGen> {
public:
    ModulemapCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() const override;
    void writeModulemapFile() override;
    void writeIndentedLine(int x, const std::string& line);
    std::unique_ptr<ModulemapModule> getModulemapModule() const;
    void writeModulemapModule(const ModulemapModule& x, int indentation);
};

#endif *//* ModulemapCodeGen_h */


