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

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/FindStaticTokensAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"

FindStaticTokensAnalysisPass::FindStaticTokensAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindStaticTokensAnalysisPass, FindStaticTokensAnalysisResult>(astAnalysisRunner) {}

std::string FindStaticTokensAnalysisPass::serializationFileName() const {
    return "FindStaticTokens.txt";
}

std::string FindStaticTokensAnalysisPass::testFileName() const {
    return "testFindStaticTokens.txt";
}

bool FindStaticTokensAnalysisPass::VisitRecordDecl(clang::RecordDecl* recordDecl) {
    if (!recordDecl->isThisDeclarationADefinition()) {
        return true;
    }
    if (!isEarliestDeclLocFromUsd(recordDecl)) {
        return true;
    }
    
    if (clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl)) {
        return true;
    }
    
    std::string name = ASTHelpers::getAsString(recordDecl);
    if (name.find("TokenType") == std::string::npos && name.find("TokensType") == std::string::npos) {
        return true;
    }
    
    // Okay, we have a token type
    if (!areAllUsdDeclsFromPublicHeaders(recordDecl)) {
        insert_or_assign(recordDecl, FindStaticTokensAnalysisResult::Kind::blockedByNonPublicHeaderDefinition);
        return true;
    }
    
    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    if (!importAnalysisPass->find(recordDecl)->second.isImportedAsValue()) {
        std::cerr << "Error! Static token type '" << name << std::endl << "' not imported as value" << std::endl;
        __builtin_trap();
    }
    
    for (const clang::FieldDecl* field : recordDecl->fields()) {
        std::string fieldName = field->getNameAsString();
        if (field->isMutable()) {
            std::cerr << "Error! Field '" << fieldName << "'is mutable" << std::endl;
            __builtin_trap();
        }
        clang::QualType _fieldType = field->getType();
        if (!_fieldType.isConstQualified() && !name.ends_with("_StaticTokenType")) {
            // `_StaticTokenType` types come from a Tf macro that doesn't make the fields const, for some reason
            std::cerr << "Error! Field '" << fieldName << "' on '" << name << "' is non-const" << std::endl;
            __builtin_trap();
        }
        _fieldType.removeLocalConst();
        const clang::TagDecl* fieldType = _fieldType->getAsTagDecl();
        const clang::TagDecl* tfTokenTag = getASTAnalysisRunner().findTagDecl("class " PXR_NS"::TfToken");
        const clang::TagDecl* tokenVectorTag = getASTAnalysisRunner().findTagDecl("class std::vector<class " PXR_NS"::TfToken>");
        
        if (fieldType != tfTokenTag && fieldType != tokenVectorTag) {
            std::cerr << "Error! Field '" << fieldName << "' is '" << ASTHelpers::getAsString(fieldType) << "'" << std::endl;
            __builtin_trap();
        }
    }
    
    insert_or_assign(recordDecl, FindStaticTokensAnalysisResult::Kind::importedAsValue);
    return true;
}
