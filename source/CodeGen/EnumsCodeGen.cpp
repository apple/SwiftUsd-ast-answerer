// ===-------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer authors. All Rights Reserved. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at: 
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.     
// 
// SPDX-License-Identifier: Apache-2.0
// ===-------------------------------------------------------------------===//

#include "CodeGen/EnumsCodeGen.h"

EnumsCodeGen::EnumsCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<EnumsCodeGen>(codeGenRunner) {}

std::string EnumsCodeGen::fileNamePrefix() const {
    return "Enums";
}

EnumsCodeGen::Data EnumsCodeGen::preprocess() {
    Data result;
    
    const auto& enumsData = getFindEnumsAnalysisPass()->getData();
    const auto& importData = getImportAnalysisPass()->getData();
    for (const auto& it : enumsData) {
        if (importData.find(it.first) == importData.end()) {
            continue;
        }
        if (!importData.find(it.first)->second.isImportedSomehow()) {
            continue;
        }
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (!hasTypeName<SwiftNameInCpp>(itFirst)) { continue; }
        auto printer = typeNamePrinter(itFirst);
        if (!getTypeName<SwiftNameInCpp>(printer).starts_with("pxr::")) { continue; }
        result.push_back(itFirst);
    }
    return result;
}

std::vector<std::string> EnumsCodeGen::namespacesForExternConstDecl(const clang::EnumDecl* enumDecl, TypeNamePrinter& printer) const {
    std::vector<std::string> result = splitStringByRegex(getTypeName<SwiftNameInCpp>(printer), std::regex("::"));
    if (result[0] == "pxr") {
        result[0] = "Overlay";
    }
    
    const auto& enumsData = getFindEnumsAnalysisPass()->getData();
    FindEnumsAnalysisResult analysisResult = enumsData.find(enumDecl)->second;
    
    if (!analysisResult.isScoped) {
        result.erase(result.begin() + (result.size() - 1));
    }
    return result;
}

std::string EnumsCodeGen::overlayMember(const clang::EnumDecl* enumDecl, const std::string& name, bool cpp, TypeNamePrinter& printer) const {
    std::vector<std::string> namespaces = namespacesForExternConstDecl(enumDecl, printer);
    namespaces.push_back(name);
    std::string separator = cpp ? "::" : ".";
    
    std::string result;
    for (int i = 0; i < namespaces.size(); i++) {
        result += namespaces[i];
        if (i + 1 < namespaces.size()) {
            result += separator;
        }
    }
    return result;
}

std::string EnumsCodeGen::pxrMember(const clang::EnumDecl* enumDecl, const std::string& name, bool cpp, TypeNamePrinter& printer) const {
    std::vector<std::string> namespaces = namespacesForExternConstDecl(enumDecl, printer);
    namespaces.push_back(name);
    namespaces[0] = "pxr";
    std::string separator = cpp ? "::" : ".";
    
    std::string result;
    for (int i = 0; i < namespaces.size(); i++) {
        result += namespaces[i];
        if (i + 1 < namespaces.size()) {
            result += separator;
        }
    }
    return result;
}


void EnumsCodeGen::writeHeaderFile(const Data& data) {
    const auto& enumsData = getFindEnumsAnalysisPass()->getData();
    
    for (const clang::TagDecl* tagDecl : data) {
        const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(tagDecl);
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        auto printer = typeNamePrinter(enumDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        std::vector<std::string> namespaces = namespacesForExternConstDecl(enumDecl, printer);
        
        std::function<std::string(unsigned long)> makePadding = [](unsigned long x) {
            std::string result;
            for (unsigned long i = 0; i < x; i++) {
                result += "  ";
            }
            return result;
        };
        
        FindEnumsAnalysisResult analysisResult = enumsData.find(tagDecl)->second;
        for (int i = 0; i < namespaces.size(); i++) {
            writeLine(makePadding(i) + "namespace " + namespaces[i] + " {");
        }
        for (const auto& caseName : analysisResult.caseNames) {
            writeLine(makePadding(namespaces.size()) + "extern const " + cppTypeName + " " + caseName + ";");
        }
        for (int i = 0; i < namespaces.size(); i++) {
            writeLine(makePadding(namespaces.size() - i - 1) + "}");
        }
    }
}

void EnumsCodeGen::writeMmFile(const Data& data) {
    const auto& enumsData = getFindEnumsAnalysisPass()->getData();
    
    for (const clang::TagDecl* tagDecl : data) {
        const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(tagDecl);
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
                
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        FindEnumsAnalysisResult analysisResult = enumsData.find(tagDecl)->second;
        for (const auto& name : analysisResult.caseNames) {
            writeLine("const " + cppTypeName + " " + overlayMember(enumDecl, name, true, printer) + " = " + pxrMember(enumDecl, name, true, printer) + ";");
        }
        writeLine("");
    }
}


void EnumsCodeGen::writeSwiftFile(const Data& data) {
    const auto& enumsData = getFindEnumsAnalysisPass()->getData();
    
    for (const clang::TagDecl* tagDecl : data) {
        const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(tagDecl);
        if (!hasTypeName<SwiftNameInSwift>(tagDecl)) { continue; }
                
        FindEnumsAnalysisResult analysisResult = enumsData.find(tagDecl)->second;
        if (analysisResult.isScoped) { continue; }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        writeLine("extension " + swiftTypeName + " {");
        for (const auto& name : analysisResult.caseNames) {
            writeLine("  @_documentation(visibility: internal) public static var " + name + ": " + swiftTypeName + " { " + overlayMember(enumDecl, name, false, printer) + " }");
        }
        writeLine("}");
    }
}
