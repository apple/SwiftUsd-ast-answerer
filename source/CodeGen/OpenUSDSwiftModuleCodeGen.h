/*
//
//  OpenUSDSwiftModuleCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/22/24.
//

#ifndef OpenUSDSwiftModuleCodeGen_h
#define OpenUSDSwiftModuleCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for the `OpenUSD.swift` swift module that @_exported imports clang modules from the modulemap. 
class OpenUSDSwiftModuleCodeGen: public CodeGenBase<OpenUSDSwiftModuleCodeGen> {
public:
    OpenUSDSwiftModuleCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() const override;
    void prepareWriteSwiftFile() override;
    void writeSwiftFile(const Data& data) override;
    
    std::optional<std::string> getModuleToAdd(const std::filesystem::path& relativePathOfPxrLibrary) const;
};

#endif *//* OpenUSDSwiftModuleCodeGen_h */

