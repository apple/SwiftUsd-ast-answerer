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

#ifndef ImportAnalysisPass_h
#define ImportAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/ImportAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// Analysis that tries to determine whether or not a C++ type will be imported into Swift,
// how it will be imported (as a value type, as a non-copyable type, or a reference type),
// or why it is not imported if it is not imported.
// Useful for code generation and various analysis passes
class ImportAnalysisPass final: public ASTAnalysisPass<ImportAnalysisPass, ImportAnalysisResult> {
public:
    using AnalysisResult = ImportAnalysisResult;
    
    ImportAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
    void onFindPotentialCandidate(const clang::TagDecl* tagDecl);
    
    // `checkFoo()` methods return true if they've found an answer,
    // and false if they haven't
    
    bool checkSpecialCaseHandling(const clang::TagDecl* tagDecl);
    bool checkPublicHeader(const clang::TagDecl* tagDecl);
    bool checkClassTemplateDecl(const clang::TagDecl* tagDecl);
    bool checkTemplate(const clang::TagDecl* tagDecl);
    bool checkParentScope(const clang::TagDecl* tagDecl);
    bool checkAccessSpecifier(const clang::TagDecl* tagDecl);
    bool checkSharedReferenceType(const clang::TagDecl* tagDecl);
    bool checkValueType(const clang::TagDecl* tagDecl);
    bool checkNonCopyableType(const clang::TagDecl* tagDecl);
    bool checkImmortalReferenceType(const clang::TagDecl* tagDecl);
    bool checkMissingMoveCtor(const clang::TagDecl* tagDecl);
    bool checkMissingDtor(const clang::TagDecl* tagDecl);
};

#endif /* ImportAnalysisResult_h */
