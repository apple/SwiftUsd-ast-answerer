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

#ifndef PublicInheritanceAnalysisPass_h
#define PublicInheritanceAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/PublicInheritanceAnalysisResult.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <unordered_map>
#include <unordered_set>

class PublicInheritanceAnalysisPass final: public ASTAnalysisPass<PublicInheritanceAnalysisPass, PublicInheritanceAnalysisResult> {
public:
    PublicInheritanceAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    void analysisPassIsFinished() override;
    
    // Returns the public subtypes of base, including base and indirect/multiple levels of inheritance.
    // It is an error to call this before analysisPassIsFinished() is called
    std::unordered_set<const clang::CXXRecordDecl*> getPublicSubtypes(const clang::CXXRecordDecl* base) const;
    
private:
    bool _analysisPassIsFinished = false;
    mutable std::unordered_map<const clang::CXXRecordDecl*, std::unordered_set<const clang::CXXRecordDecl*>> _publicSubtypes;
};

#endif /* PublicInheritanceAnalysisPass_h */
