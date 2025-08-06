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

#include "AnalysisPass/TypedefAnalysisPass.h"
#include "Util/CMakeParser.h"

TypedefAnalysisPass::TypedefAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<TypedefAnalysisPass, TypedefAnalysisResult>(astAnalysisRunner) {}

std::string TypedefAnalysisPass::serializationFileName() const {
    return "Typedef.txt";
}
std::string TypedefAnalysisPass::testFileName() const {
    return "testTypedef.txt";
}

bool TypedefAnalysisPass::isTypedefInAllowableDeclContext(const clang::TypedefNameDecl* typedefNameDecl) const {
    const clang::DeclContext* declContext = typedefNameDecl->getLexicalDeclContext();
    if (!declContext) {
        return false;
    }
#define DISALLOW(TYPE) \
if (clang::dyn_cast<clang::TYPE>(declContext)) {\
    return false;\
}
    
    DISALLOW(HLSLBufferDecl)
    DISALLOW(LabelDecl)
    DISALLOW(ObjCCompatibleAliasDecl)
    DISALLOW(ObjCContainerDecl)
    DISALLOW(ObjCMethodDecl)
    DISALLOW(ObjCPropertyDecl)
    DISALLOW(TemplateDecl)
    DISALLOW(ClassTemplateSpecializationDecl)
    DISALLOW(UnresolvedUsingIfExistsDecl)
    DISALLOW(UsingPackDecl)
    DISALLOW(UsingShadowDecl)
    DISALLOW(ValueDecl)
    
    if (!clang::dyn_cast<clang::NamedDecl>(declContext)) {
        return false;
    }
    
    return true;
    
#undef DISALLOW
}

bool TypedefAnalysisPass::VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) {
    if (!isEarliestDeclLocFromUsd(typedefNameDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(typedefNameDecl)) {
        return true;
    }
    if (ASTHelpers::isNotVisibleToSwift(typedefNameDecl->getAccess())) {
        return true;
    }
    
    std::string sugaredName = typedefNameDecl->getNameAsString();
    if (sugaredName.size() == 0) {
        return true;
    }
    
    clang::QualType unsugaredType = typedefNameDecl->getUnderlyingType().getCanonicalType();
    const clang::TagDecl* unsugaredTagDecl = unsugaredType->getAsTagDecl();
    if (!unsugaredTagDecl) {
        return true;
    }
    if (!doesTypeContainUsdTypes(unsugaredTagDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(unsugaredTagDecl)) {
        return true;
    }
    if (!isTypedefInAllowableDeclContext(typedefNameDecl)) {
        return true;
    }
    
    const clang::NamedDecl* enclosingNamedDecl = clang::dyn_cast<clang::NamedDecl>(typedefNameDecl->getLexicalDeclContext());
    
    auto it = find(unsugaredTagDecl);
    if (it == end()) {
        insert_or_assign(unsugaredTagDecl, TypedefAnalysisResult());
        it = find(unsugaredTagDecl);
    }
    it->second.insert(enclosingNamedDecl, sugaredName);
    return true;
}

bool TypedefAnalysisPass::comparesEqualWhileTesting(const TypedefAnalysisResult& expected, const TypedefAnalysisResult& actual) const {
    return expected.isSubset(actual);
}


