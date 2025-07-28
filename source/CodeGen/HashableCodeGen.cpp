//
//  HashableCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/3/24.
//

#include "CodeGen/HashableCodeGen.h"

HashableCodeGen::HashableCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<HashableCodeGen>(codeGenRunner) {}

std::string HashableCodeGen::fileNamePrefix() const {
    return "Hashable";
}

HashableCodeGen::Data HashableCodeGen::preprocess() {
    std::vector<const clang::TagDecl*> result;
    const auto& hashableData = getHashableAnalysisPass()->getData();
    for (const auto& it : hashableData) {
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (it.second.isAvailable() && hasTypeName<CppNameInCpp>(itFirst)) {
            result.push_back(itFirst);
        }
    }
    return result;
}

HashableCodeGen::Data HashableCodeGen::extraSpecialCaseFiltering(const Data& data) const {
    // Do all the same filtering as EquatableCodeGen
    Data result;
    for (const clang::TagDecl* tagDecl : data) {
        auto printer = typeNamePrinter(tagDecl);
        auto swiftName = getTypeNameOpt<SwiftNameInSwift>(printer);        
        if (swiftName == "pxr.TraceThreadId" ||
            swiftName == "pxr.HgiShaderProgramDesc") {
            continue;
        }
        
        result.push_back(tagDecl);
    }
    return result;
}

void HashableCodeGen::writeHeaderFile(const HashableCodeGen::Data& data) {
    writeLine("namespace __Overlay {");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }

        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        writeLine("  int64_t hash_value(const " + cppTypeName + "& x);");
    }
    writeLine("}");
}

void HashableCodeGen::writeCppFile(const HashableCodeGen::Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }

        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        if (isImportedAsAnyReference(tagDecl)) {
            writeLines({
                "int64_t __Overlay::hash_value(const " + cppTypeName + "& x) {",
                "    return (int64_t) &x;",
                "}",
            });
        } else {
            writeLines({
                "int64_t __Overlay::hash_value(const " + cppTypeName + "& x) {",
                "    return pxr::TfHash()(x);",
                "}",
            });
        }
    }
}

void HashableCodeGen::writeSwiftFile(const HashableCodeGen::Data& data) {
    writeLine("// Conformance blocked by no Swift type name:");
    writeLine("");
    clang::PrintingPolicy printingPolicy((clang::LangOptions()));
    printingPolicy.adjustForCPlusPlus();
    printingPolicy.SuppressTagKeyword = 1;
    
    for (const clang::TagDecl* tagDecl : data) {
        writeThatTagDeclHasNoSwiftNameInSwiftIfNeeded(tagDecl);
    }
    
    writeLine("");
    writeLine("");
    writeLine("// Conformance available:");
    writeLine("");
    
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInSwift>(tagDecl)) { continue; }

        auto printer = typeNamePrinter(tagDecl);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        writeLines({
            "extension " + swiftTypeName + ": Hashable {",
            "    public func hash(into hasher: inout Hasher) {",
            "        hasher.combine(__Overlay.hash_value(self))",
            "    }",
            "}",
        });
    }
}

std::vector<std::pair<std::string, HashableCodeGen::Data>> HashableCodeGen::writeDocCFile(std::string* outTitle,
                                    std::string* outOverview,
                                    const HashableCodeGen::Data& processedData) {
    *outTitle = "Hashable protocol conformances";
    *outOverview = "These types conform to `Hashable` in Swift.";
    return {{"", processedData}};
}
