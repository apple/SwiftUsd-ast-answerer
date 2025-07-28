//
//  EnumsCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#ifndef EnumsCodeGen_h
#define EnumsCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for wrapping enums and providing implicit member expressions for unscoped enums
class EnumsCodeGen: public CodeGenBase<EnumsCodeGen> {
public:
    EnumsCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeMmFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    
    std::vector<std::string> namespacesForExternConstDecl(const clang::EnumDecl* enumDecl, TypeNamePrinter& printer) const;
    std::string overlayMember(const clang::EnumDecl* enumDecl, const std::string& name, bool cpp, TypeNamePrinter& printer) const;
    std::string pxrMember(const clang::EnumDecl* enumDecl, const std::string& name, bool cpp, TypeNamePrinter& printer) const;
};

#endif /* EnumsCodeGen_h */
