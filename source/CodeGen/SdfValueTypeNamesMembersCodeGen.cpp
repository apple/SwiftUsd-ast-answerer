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
