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

#ifndef TypedefAnalysisPass_h
#define TypedefAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/TypedefAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds a subset of typedefs of C++ tags.
// Used by code gen, because Swift-Cxx interop does not currently support specializing
// template classes when used as a type. 
class TypedefAnalysisPass final: public ASTAnalysisPass<TypedefAnalysisPass, TypedefAnalysisResult> {
public:
    TypedefAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    
    bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) override;
    bool isTypedefInAllowableDeclContext(const clang::TypedefNameDecl* typedefNameDecl) const;
    
    bool comparesEqualWhileTesting(const TypedefAnalysisResult& expected, const TypedefAnalysisResult& actual) const override;
};

#endif /* TypedefAnalysisPass_h */
