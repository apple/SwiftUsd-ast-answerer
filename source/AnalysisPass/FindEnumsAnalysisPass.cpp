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

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"


FindEnumsAnalysisPass::FindEnumsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<FindEnumsAnalysisPass, FindEnumsAnalysisResult>(astAnalysisRunner) {}
    
std::string FindEnumsAnalysisPass::serializationFileName() const {
    return "FindEnums.txt";
}
std::string FindEnumsAnalysisPass::testFileName() const {
    return "testFindEnums.txt";
}

bool FindEnumsAnalysisPass::VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl) {
    if (!isEarliestDeclLocFromUsd(enumConstantDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(enumConstantDecl)) {
        return true;
    }
    
    const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(enumConstantDecl->getDeclContext());
            
    if (find(enumDecl) == end()) {
        insert_or_assign(enumDecl, FindEnumsAnalysisResult(enumDecl->isScoped()));
    }
    
    std::string name = enumConstantDecl->getNameAsString();
    llvm::APSInt apsInt = enumConstantDecl->getInitVal();
    int64_t value = apsInt.getExtValue();
    
    find(enumDecl)->second.addCase(name, value);
    
    return true;
}
