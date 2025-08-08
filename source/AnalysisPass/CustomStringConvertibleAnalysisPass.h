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

#ifndef CustomStringConvertibleAnalysisPass_h
#define CustomStringConvertibleAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/CustomStringConvertibleAnalysisResult.h"

// A type T is CustomStringConvertible iff there is a function `std::ostream& operator<<(std::ostream& os, const U& obj)` where T is convertible to U.
// CustomStringConvertible is inheritable

class CustomStringConvertibleAnalysisPass final: public ASTAnalysisPass<CustomStringConvertibleAnalysisPass, CustomStringConvertibleAnalysisResult> {
public:
    CustomStringConvertibleAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitEnumDecl(clang::EnumDecl* enumDecl) override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) override;
    void finalize(const clang::NamedDecl* namedDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
private:
    std::set<clang::QualType> _lessThanLessThanFunctionTypes;
};

#endif /* CustomStringConvertibleAnalysisPass_h */
