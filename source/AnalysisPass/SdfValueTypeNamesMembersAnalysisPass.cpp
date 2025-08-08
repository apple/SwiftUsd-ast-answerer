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

#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"

SdfValueTypeNamesMembersAnalysisPass::SdfValueTypeNamesMembersAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<SdfValueTypeNamesMembersAnalysisPass, SdfValueTypeNamesMembersAnalysisResult>(astAnalysisRunner) {}

std::string SdfValueTypeNamesMembersAnalysisPass::serializationFileName() const {
    return "SdfValueTypeNamesMembers.txt";
}

std::string SdfValueTypeNamesMembersAnalysisPass::testFileName() const {
    return "testSdfValueTypeNamesMembers.txt";
}

bool SdfValueTypeNamesMembersAnalysisPass::VisitTagDecl(clang::TagDecl *tagDecl) {
    // We're looking for the fields on one specific type with a known name,
    // so we don't want to walk the AST, just pull things out that we already know.
    
    const clang::TagDecl* valueTypeNamesTypeTagDecl = findTagDecl("class " PXR_NS"::Sdf_ValueTypeNamesType");
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(valueTypeNamesTypeTagDecl);
    insert_or_assign(valueTypeNamesTypeTagDecl, SdfValueTypeNamesMembersAnalysisResult());
    
    for (const clang::FieldDecl* field : cxxRecordDecl->fields()) {
        if (field->getType().getAsString() == "SdfValueTypeName") {
            find(valueTypeNamesTypeTagDecl)->second.push_back(field->getNameAsString());
        }
    }
    
    // Return false to abort the AST descent,
    // because we're already done.
    return false;
}
