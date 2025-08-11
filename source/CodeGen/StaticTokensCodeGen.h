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

#ifndef StaticTokensCodeGen_h
#define StaticTokensCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for exposing static token types and provoding implicit member expressions
class StaticTokensCodeGen: public CodeGenBase<StaticTokensCodeGen> {
public:
    StaticTokensCodeGen(const CodeGenRunner* codeGenRunnder);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeHeaderFile(const Data& data) override;
    void writeMmFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;    
};

#endif /* StaticTokensCodeGen_h */
