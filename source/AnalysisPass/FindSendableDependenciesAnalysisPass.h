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

#ifndef FindSendableDependenciesAnalysisPass_h
#define FindSendableDependenciesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindSendableDependenciesAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Sendable analysis occurs in two sub-passes. This is the first sub-pass, that identifies "dependencies" of a given type for Sendable analysis. See also SendableAnalysisPass. 
// Dependencies can be the names and types of fields, base classes, and special-cased behavior
// (e.g. std::vector<T> depends on T, FRTs are not Sendable, TfToken is Sendable). 
class FindSendableDependenciesAnalysisPass final: public ASTAnalysisPass<FindSendableDependenciesAnalysisPass, FindSendableDependenciesAnalysisResult> {
public:
    FindSendableDependenciesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
    void handleSpecialDependencies(const clang::RecordDecl* recordDecl, FindSendableDependenciesAnalysisResult& analysisResult);
};

#endif /* FindSendableDependenciesAnalysisResult_h */
