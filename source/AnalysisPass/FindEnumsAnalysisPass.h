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

#ifndef FindEnumsAnalysisPass_h
#define FindEnumsAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindEnumsAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds C++ enums and their cases. Used to power enum wrapping code gen
// to work around a limitation in Swift-Cxx interop where nested unscoped enums aren't imported correctly. 
class FindEnumsAnalysisPass final: public ASTAnalysisPass<FindEnumsAnalysisPass, FindEnumsAnalysisResult> {
public:
    FindEnumsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl) override;
};

#endif /* FindEnumsAnalysisPass_h */
