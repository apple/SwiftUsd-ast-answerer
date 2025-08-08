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
