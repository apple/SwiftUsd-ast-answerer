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

#ifndef PublicInheritanceAnalysisResult_h
#define PublicInheritanceAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class PublicInheritanceAnalysisPass;


struct PublicInheritanceAnalysisResult {
    explicit operator std::string() const;
    
    PublicInheritanceAnalysisResult(std::vector<const clang::CXXRecordDecl*> publicBases);
    
    static std::optional<PublicInheritanceAnalysisResult> deserialize(const std::string& data, const PublicInheritanceAnalysisPass* astAnalysisPass);
    
    const std::vector<const clang::CXXRecordDecl*>& getPublicBases() const;
    
private:
    std::vector<const clang::CXXRecordDecl*> _publicBases;
};
std::ostream& operator <<(std::ostream& os, const PublicInheritanceAnalysisResult& obj);

#endif /* PublicInheritanceAnalysisResult_h */
