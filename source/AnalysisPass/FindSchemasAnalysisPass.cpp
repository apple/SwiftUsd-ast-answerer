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

#include "AnalysisPass/FindSchemasAnalysisPass.h"


FindSchemasAnalysisPass::FindSchemasAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindSchemasAnalysisPass, FindSchemasAnalysisResult>(astAnalysisRunner) {
    
}

std::string FindSchemasAnalysisPass::serializationFileName() const {
    return "FindSchemas.txt";
}

std::string FindSchemasAnalysisPass::testFileName() const {
    return "testFindSchemas.txt";
}

bool FindSchemasAnalysisPass::VisitCXXRecordDecl(clang::CXXRecordDecl *cxxRecordDecl) {
    const clang::TagDecl* tagDecl = findTagDecl("class " PXR_NS"::UsdSchemaBase");
    const clang::CXXRecordDecl* usdSchemaBase = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    
    if (cxxRecordDecl->isThisDeclarationADefinition()) {
        if (ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(cxxRecordDecl, usdSchemaBase)) {
            insert_or_assign(cxxRecordDecl, FindSchemasAnalysisResult());
        }
    }
    return true;
}
