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

#ifndef FindNamedDeclsAnalysisPass_h
#define FindNamedDeclsAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// The first analysis pass. This analysis pass allows future analysis passes to find tags by name, which
// makes testing and deserializing analysis passes much easier. It also makes it easier to write analysis passes and code gen passes,
// by refering to specific C++ types by their string names.
// Unfortunately, this analysis pass has to be run every time, and it takes a bit of time to do so. 
class FindNamedDeclsAnalysisPass final: public ASTAnalysisPass<FindNamedDeclsAnalysisPass, FindNamedDeclsAnalysisResult> {
public:
    using AnalysisResult = FindNamedDeclsAnalysisResult;
    
    FindNamedDeclsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
        
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    void serialize() const override;
    bool deserialize() override;
    bool VisitNamedDecl(clang::NamedDecl* namedDecl) override;
    bool VisitType(clang::Type* type) override;
    
    void test() const override;
    
    const clang::TagDecl* findTagDecl(const std::string& typeName) const;
    const clang::NamedDecl* findNamedDecl(const std::string& name) const;
    const clang::Type* findType(const std::string& name) const;
    const clang::FunctionDecl* findFunctionDecl(const std::string& signature) const;
    
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
private:
    const FindNamedDeclsAnalysisResult::NamedDeclMap& getNamedDeclMap() const;
    FindNamedDeclsAnalysisResult::NamedDeclMap& getNamedDeclMap();
    
    const FindNamedDeclsAnalysisResult::TypeMap& getTypeMap() const;
    FindNamedDeclsAnalysisResult::TypeMap& getTypeMap();
};

#endif /* FindNamedDeclsAnalysisPass_h */
