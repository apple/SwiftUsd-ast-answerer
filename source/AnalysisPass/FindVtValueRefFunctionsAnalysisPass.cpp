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

#include "AnalysisPass/FindVtValueRefFunctionsAnalysisPass.h"

FindVtValueRefFunctionsAnalysisPass::FindVtValueRefFunctionsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindVtValueRefFunctionsAnalysisPass, FindVtValueRefFunctionsAnalysisResult>(astAnalysisRunner) {
    
}

std::string FindVtValueRefFunctionsAnalysisPass::serializationFileName() const {
    return "FindVtValueRefFunctions.txt";
}

std::string FindVtValueRefFunctionsAnalysisPass::testFileName() const {
    return "testFindVtValueRefFunctions.txt";
}

bool FindVtValueRefFunctionsAnalysisPass::VisitFunctionDecl(clang::FunctionDecl *functionDecl) {
    if (!functionDecl->isThisDeclarationADefinition()) { return true; }
    if (isFromUsdLibrary(functionDecl, "vt")) { return true; }
    if (isFromUsdLibraryStrictlyBefore(functionDecl, "vt")) { return true; }
    
    const clang::TagDecl* vtValueRef = findTagDecl("class " PXR_NS"::VtValueRef");
    
    bool usesVtValueRefAsParameter = false;
    for (const clang::ParmVarDecl* parmVarDecl : functionDecl->parameters()) {
        const clang::TagDecl* tagDecl = parmVarDecl->getType().getTypePtr()->getAsTagDecl();
        if (tagDecl == vtValueRef) {
            usesVtValueRefAsParameter = true;
            break;
        }
    }
    
    if (!usesVtValueRefAsParameter) { return true; }
    
    std::string fName = functionDecl->getNameAsString();
    if (fName.starts_with("_")) { return true; }
    if (fName.starts_with("operator ")) { return true; }
    if (functionDecl->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) { return true; }
    
    insert_or_assign(functionDecl, {});
    return true;
}
