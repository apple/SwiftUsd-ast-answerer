//
//  StaticTokensCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#include "CodeGen/StaticTokensCodeGen.h"


StaticTokensCodeGen::StaticTokensCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<StaticTokensCodeGen>(codeGenRunner) {}

std::string StaticTokensCodeGen::fileNamePrefix() const {
    return "StaticTokens";
}

StaticTokensCodeGen::Data StaticTokensCodeGen::preprocess() {
    Data result;
    
    for (const auto& it : getFindStaticTokensAnalysisPass()->getData()) {
        if (it.second.kind == FindStaticTokensAnalysisResult::Kind::importedAsValue) {
            result.push_back(clang::dyn_cast<clang::TagDecl>(it.first));
        }
    }
    return result;
}

std::string externConstName(std::string swiftNameInCpp) {
    std::string prefix = "pxr::";
    std::string suffix;
    if (swiftNameInCpp.ends_with("TokensType")) {
        suffix = "Type";
    } else if (swiftNameInCpp.ends_with("_StaticTokenType")) {
        suffix = "_StaticTokenType";
    } else {
        std::cout << "Error! Unexpected static token type name '" << swiftNameInCpp << std::endl;
        __builtin_trap();
    }
    
    return swiftNameInCpp.substr(prefix.size(), swiftNameInCpp.size() - prefix.size() - suffix.size());
}

void StaticTokensCodeGen::writeHeaderFile(const Data& data) {
    writeLine("namespace __Overlay {");
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        std::string varName = externConstName(cppTypeName);
        writeLine("    extern const " + cppTypeName + "* const " + varName + ";");
    }
    writeLine("}");
}
void StaticTokensCodeGen::writeMmFile(const Data& data) {
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        std::string varName = externConstName(cppTypeName);

        writeLine("const " + cppTypeName + "* const __Overlay::" + varName + " = pxr::" + varName + ".Get();");
    }
}
void StaticTokensCodeGen::writeSwiftFile(const Data& data) {
    writeLine("extension pxr.TfToken {");
    
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        std::string varName = externConstName(cppTypeName);
        writeLines({
            "    public static var " + varName + ": " + swiftTypeName + " {",
            "        __Overlay." + varName + ".pointee",
            "    }",
        });
    }
    
    writeLine("}");
}
