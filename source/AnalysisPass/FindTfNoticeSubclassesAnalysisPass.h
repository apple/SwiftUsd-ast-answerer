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

#ifndef FindTfNoticeSubclassesAnalysisPass_h
#define FindTfNoticeSubclassesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindTfNoticeSubclassesAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindTfNoticeSubclassesAnalysisPass final: public ASTAnalysisPass<FindTfNoticeSubclassesAnalysisPass, FindTfNoticeSubclassesAnalysisResult> {
public:
    FindTfNoticeSubclassesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    
private:
    bool _checkInheritance(const clang::TagDecl*);
    
    std::map<const clang::TagDecl*, bool> _cache;
};

#endif /* FindTfNoticeSubclassesAnalysisPass_h */
