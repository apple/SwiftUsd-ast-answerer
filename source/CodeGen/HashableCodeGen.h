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

#ifndef HashableCodeGen_h
#define HashableCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

#include "AnalysisPass/HashableAnalysisPass.h"

// Code gen for conformances to `Hashable`
class HashableCodeGen: public CodeGenBase<HashableCodeGen> {
public:
    HashableCodeGen(const CodeGenRunner* codeGenRunner);
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    Data extraSpecialCaseFiltering(const Data& data) const override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                            std::string* outOverview,
                                                            const Data& processedData) override;

};

#endif /* HashableCodeGen_h */
