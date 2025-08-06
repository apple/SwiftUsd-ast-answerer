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
