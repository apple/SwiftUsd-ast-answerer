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

#ifndef APINotesAnalysisPass_h
#define APINotesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/APINotesAnalysisResult.h"
#include "clang/AST/RecursiveASTVisitor.h"


class APINotesAnalysisPass final: public ASTAnalysisPass<APINotesAnalysisPass, APINotesAnalysisResult> {
public:
    APINotesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitNamedDecl(clang::NamedDecl* namedDecl) override;
    
private:
    std::vector<const clang::NamedDecl*> getHardCodedOwnedTypes() const;
    std::vector<const clang::FunctionDecl*> getHardCodedReplaceConstRefFunctionsWithCopy() const;
    std::vector<const clang::FunctionDecl*> getHardCodedReplaceMutatingFunctionsWithNonmutating() const;
};

#endif /* APINotesAnalysisPass_h */
