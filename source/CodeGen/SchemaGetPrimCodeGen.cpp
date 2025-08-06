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

#include "CodeGen/SchemaGetPrimCodeGen.h"
#include "AnalysisPass/FindSchemasAnalysisPass.h"

SchemaGetPrimCodeGen::SchemaGetPrimCodeGen(const CodeGenRunner* codeGenRunner) :
CodeGenBase<SchemaGetPrimCodeGen>(codeGenRunner) {}

std::string SchemaGetPrimCodeGen::fileNamePrefix() const {
    return "SchemaUtil";
}

SchemaGetPrimCodeGen::Data SchemaGetPrimCodeGen::preprocess() {
    Data result;
    for (const auto& x : getFindSchemasAnalysisPass()->getData()) {
        result.push_back(clang::dyn_cast<clang::TagDecl>(x.first));
    }
    return result;
}

void SchemaGetPrimCodeGen::writeHeaderFile(const SchemaGetPrimCodeGen::Data &data) {
    writeLine("namespace Overlay {");
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string name = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "    pxr::UsdPrim GetPrim(const " + name + "& x);",
        });
    }
    writeLines({
        "}",
        "",
        "namespace __Overlay {"
    });
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string name = getTypeName<SwiftNameInCpp>(printer);
        writeLine("    bool convertToBool(const " + name + "& x);");
    }
    writeLine("}");
}

void SchemaGetPrimCodeGen::writeCppFile(const SchemaGetPrimCodeGen::Data &data) {
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string name = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "pxr::UsdPrim Overlay::GetPrim(const " + name + "& x) { return x.GetPrim(); }",
        });
    }
    
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string name = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "bool __Overlay::convertToBool(const " + name + "& x) { return (bool)x; }",
        });
    }
}

void SchemaGetPrimCodeGen::writeSwiftFile(const SchemaGetPrimCodeGen::Data &data) {
    writeLine("extension Bool {");
    for (const auto& x : data) {
        auto printer = typeNamePrinter(x);
        std::string name = getTypeName<SwiftNameInSwift>(printer);
        writeLine("    public init(_ x: " + name + ") { self.init(__Overlay.convertToBool(x)) }");
    }
    writeLine("}");
}
