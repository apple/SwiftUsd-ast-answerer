//===----------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer project authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

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
