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

#ifndef APINotesCodeGen_h
#define APINotesCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "AnalysisPass/APINotesAnalysisPass.h"
#include "AnalysisResult/APINotesAnalysisResult.h"

struct NamespaceItem;
struct APINotesNode;

// Code gen for the API notes file
class APINotesCodeGen: public CodeGenBase<APINotesCodeGen> {
public:
    APINotesCodeGen(const CodeGenRunner* codeGenRunner);
    ~APINotesCodeGen();
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeAPINotesFile() override;
    
private:
    struct ReplacedMethod {
        const clang::CXXMethodDecl* method; // method->getParent() may not be owningType
        const clang::CXXRecordDecl* owningType; // may be a type that publicly inherits (directly or indirectly) from method->getParent()
        APINotesAnalysisResult analysisResult;
        
        bool operator<(const ReplacedMethod& other) const;
    };
    
    std::vector<ReplacedMethod> getReplacedMethods(const APINotesNode*);
    
    void writeReplaceMethod(ReplacedMethod replacedMethod,
                            bool isHeader,
                            std::map<const clang::CXXRecordDecl*, std::string>& importAsMemberTypedefs);
    
    std::unique_ptr<NamespaceItem> _root;

};

#endif /* APINotesCodeGen_h */
