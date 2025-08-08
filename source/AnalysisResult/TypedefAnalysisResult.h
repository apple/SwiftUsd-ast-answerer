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

#ifndef TypedefAnalysisResult_h
#define TypedefAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include <set>

class TypedefAnalysisPass;

struct TypedefAnalysisResult {
public:
    typedef std::pair<const clang::NamedDecl*, std::string> Pair;
    
    explicit operator std::string() const;
    
    TypedefAnalysisResult();
    static std::optional<TypedefAnalysisResult> deserialize(const std::string& data, const TypedefAnalysisPass* astAnalysisPass);
    
    void insert(const clang::NamedDecl* namedDecl, const std::string& typedefSpelling);
    
    bool isSubset(const TypedefAnalysisResult& other) const;
    
    const std::set<Pair>& getTypedefSpellings() const;
private:
    std::set<Pair> _typedefSpellings;
};
std::ostream& operator <<(std::ostream& os, const TypedefAnalysisResult& obj);

#endif /* TypedefAnalysisResult_h */
