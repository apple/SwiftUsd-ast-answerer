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

#include "StdTypeInfoEqualsEqualsLinter.h"
#include "clang/AST/ParentMapContext.h"

StdTypeInfoEqualsEqualsLinter::StdTypeInfoEqualsEqualsLinter(LinterRunner* linterRunner) :
LinterBase<>(linterRunner) {}

std::string StdTypeInfoEqualsEqualsLinter::serializationFileName() const {
    return "StdTypeInfoEqualsEquals.txt";
}

std::string StdTypeInfoEqualsEqualsLinter::testFileName() const {
    return "testStdTypeInfoEqualsEqualsLinter.txt";
}

std::vector<const clang::FunctionDecl*> StdTypeInfoEqualsEqualsLinter::getDeniedFunctions() const {
    std::vector<const clang::FunctionDecl*> result;
    
    std::vector<std::string> names = {
        "_Bool std::type_info::operator==(const class std::type_info & __arg) const",
        "_Bool std::type_info::operator!=(const class std::type_info & __arg) const",
        "_Bool std::type_index::operator==(const class std::type_index & __y) const",
        "_Bool std::type_index::operator!=(const class std::type_index & __y) const",
    };
    
    for (const auto& x : names) {
        const clang::FunctionDecl* f = findFunctionDecl(x);
        if (!f) {
            std::cerr << "Couldn't find " << x << std::endl;
            __builtin_trap();
        }
        result.push_back(f);
    }
    
    return result;
}


bool StdTypeInfoEqualsEqualsLinter::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *cxxOperatorCallExpr) {
    if (cxxOperatorCallExpr->getOperator() != clang::OO_EqualEqual && cxxOperatorCallExpr->getOperator() != clang::OO_ExclaimEqual) {
        return true;
    }
        
    clang::FunctionDecl* callee = cxxOperatorCallExpr->getDirectCallee();
    if (!callee) {
        return true;
    }
    
    static std::vector<const clang::FunctionDecl*> deniedFunctions = getDeniedFunctions();
    
    bool isDeniedFunction = false;
    for (const auto& x : deniedFunctions) {
        if (callee == x) {
            isDeniedFunction = true;
            break;
        }
    }
    if (!isDeniedFunction) {
        return true;
    }
    
    
    LinterResult::Entry entry(this, false, cxxOperatorCallExpr);
    if (entry.filePath == "pxr/base/tf/safeTypeCompare.h" && entry.lineNumber == 24) {
        entry.allow = true;
    }
        
    getLinterResult()->addEntry(entry);
    
    return true;
}
