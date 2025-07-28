//
//  SdfValueTypeNamesMembersCodeGen.hpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#include "CodeGen/SdfValueTypeNamesMembersCodeGen.h"
#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"

SdfValueTypeNamesMembersCodeGen::SdfValueTypeNamesMembersCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<SdfValueTypeNamesMembersCodeGen>(codeGenRunner) {
    
}

std::string SdfValueTypeNamesMembersCodeGen::fileNamePrefix() const {
    return "SdfValueTypeNames_Extensions";
}

SdfValueTypeNamesMembersCodeGen::Data SdfValueTypeNamesMembersCodeGen::preprocess() {
    const SdfValueTypeNamesMembersAnalysisPass* analysisPass = getSdfValueTypeNamesMembersAnalysisPass();
    // Important: Return the type so we get proper #include lines
    return {clang::dyn_cast<clang::TagDecl>(analysisPass->getData().begin()->first)};
}

void SdfValueTypeNamesMembersCodeGen::writeHeaderFile(const SdfValueTypeNamesMembersCodeGen::Data& data) {
    
    const SdfValueTypeNamesMembersAnalysisResult& analysisResult = getSdfValueTypeNamesMembersAnalysisPass()->find(data.front())->second;
    
    writeLines({
        "namespace Overlay {",
        "  namespace SdfValueTypeNames {",
    });
    for (const std::string& name : analysisResult._data) {
        writeLine("    extern const pxr::SdfValueTypeName " + name + ";");
    }
    writeLines({
        "  }",
        "}",
    });
}

void SdfValueTypeNamesMembersCodeGen::writeCppFile(const SdfValueTypeNamesMembersCodeGen::Data& data) {
    
    const SdfValueTypeNamesMembersAnalysisResult& analysisResult = getSdfValueTypeNamesMembersAnalysisPass()->find(data.front())->second;
    
    for (const std::string& name : analysisResult._data) {
        writeLine("const pxr::SdfValueTypeName Overlay::SdfValueTypeNames::" + name + " = pxr::SdfValueTypeNames->" + name + ";");
    }
}

void SdfValueTypeNamesMembersCodeGen::writeSwiftFile(const SdfValueTypeNamesMembersCodeGen::Data& data) {
    const SdfValueTypeNamesMembersAnalysisResult& analysisResult = getSdfValueTypeNamesMembersAnalysisPass()->find(data.front())->second;
    
    writeLine("extension pxr.SdfValueTypeName {");
    for (const std::string& name : analysisResult._data) {
        writeLines({
            "    public static var " + name + ": pxr.SdfValueTypeName {",
            "        Overlay.SdfValueTypeNames." + name,
            "    }"
        });
    }
    writeLine("}");
}
