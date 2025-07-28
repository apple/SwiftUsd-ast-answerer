//
//  ReferenceTypeConformanceCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/3/24.
//

#ifndef ReferenceTypeConformanceCodeGen_h
#define ReferenceTypeConformanceCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for conformances to `_SwiftUsdReferenceTypeProtocol` and related protocols
class ReferenceTypeConformanceCodeGen: public CodeGenBase<ReferenceTypeConformanceCodeGen> {
public:
    ReferenceTypeConformanceCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    
private:
    
    bool isTfRefBaseSubclass(const clang::TagDecl* tagDecl) const;
    bool isTfWeakBaseSubclass(const clang::TagDecl* tagDecl) const;
    bool isTfSingletonImmortalSpecialization(const clang::TagDecl* tagDecl) const;
    bool isExactlyTfRefBase(const clang::TagDecl* tagDecl) const;
    
    
    struct RefTypeNameHelper {
        std::string swiftNameInSwift;
        std::string swiftNameInCpp;
        std::string mangledName;
        std::string frtTypedef;
        std::string refPtrInCpp;
        std::string refPtrTypedef;
        std::string weakPtrInCpp;
        std::string weakPtrTypedef;
        std::string constRefPtrInCpp;
        std::string constRefPtrTypedef;
        std::string constWeakPtrInCpp;
        std::string constWeakPtrTypedef;
    };
    
    const RefTypeNameHelper nameHelper(TypeNamePrinter&);
};

#endif /* ReferenceTypeConformanceCodeGen_h */
