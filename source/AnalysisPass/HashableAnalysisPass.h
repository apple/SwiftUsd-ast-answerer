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

#ifndef HashableAnalysisPass_h
#define HashableAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/HashableAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// Analysis for the Swift `Hashable` protocol
class HashableAnalysisPass final: public ASTAnalysisPass<HashableAnalysisPass, HashableAnalysisResult> {
public:
    HashableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    void onFindPotentialCandidate(clang::QualType qualType);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    void finalize(const clang::NamedDecl* namedDecl) override;
    
    bool decideResultViaImportIfPossible(const clang::TagDecl* tagDecl);
    void decideResultViaEquatableGivenCandidateExists(const clang::TagDecl* tagDecl);
};

#endif /* HashableAnalysisPass_h */
