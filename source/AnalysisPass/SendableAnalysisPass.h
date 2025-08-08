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

#ifndef SendableAnalysisPass_h
#define SendableAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/SendableAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Sendable analysis occurs in two sub-passes. This is the second sub-pass, that
// examines the dependencies of a given type to decide if it is Sendable or not. See all FindSendableDependenciesAnalysisPass.
// Sendable analysis examines the directed graph of dependencies, induces a graph on
// the strongly connected components of the dependency graph, and then classifies the
// nodes in the induced graph as Sendable or not Sendable as a whole.
// Sendability is the default, and non-sendability is viral and assigned iff an SCC node
// has an outgoing edge to a non-sendable type
class SendableAnalysisPass final: public ASTAnalysisPass<SendableAnalysisPass, SendableAnalysisResult> {
public:
    SendableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitNamedDecl(clang::NamedDecl* namedDecl) override;
    
    bool _isSendable(const clang::TagDecl *tagDecl) const;
    bool _isSendable(const clang::Type *type) const;
    bool _isSendable(const clang::TemplateArgument& templateArg) const;

    bool comparesEqualWhileTesting(const SendableAnalysisResult& expected, const SendableAnalysisResult& actual) const override;
};

#endif /* SendableAnalysisPass_h */
