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

#ifndef FindSendableDependenciesAnalysisResult_h
#define FindSendableDependenciesAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindSendableDependenciesAnalysisPass;
class SendableAnalysisPass;



struct FindSendableDependenciesAnalysisResult {
    explicit operator std::string() const;
    
    FindSendableDependenciesAnalysisResult();
    
    template <typename Pass>
    static std::optional<FindSendableDependenciesAnalysisResult> deserialize(const std::string& data, const Pass* astAnalysisPass) {
        return deserializeResult(data, astAnalysisPass);
    }
    
    enum DependencyKind {
        inheritance,
        field,
        specialAvailable,
        specialImportedAsReference,
        specialConditional
    };
    
    struct Dependency {
        DependencyKind kind;
        std::string fieldName;
        const clang::Type* type;
        
        Dependency(DependencyKind kind, std::string fieldName, const clang::Type* type);
        explicit operator std::string() const;
        
        template <typename Pass>
        static std::optional<Dependency> deserialize(const std::string& data, const Pass* astAnalysisPass) {
            return deserializeDependency(data, astAnalysisPass);
        }        
    };
    
    void addInheritanceDependency(clang::CXXBaseSpecifier base);
    void addFieldDependency(const clang::FieldDecl* fieldDecl);
        
    std::vector<Dependency> dependencies;
};

std::ostream& operator <<(std::ostream& os, const FindSendableDependenciesAnalysisResult& obj);
std::ostream& operator <<(std::ostream& os, const FindSendableDependenciesAnalysisResult::Dependency& obj);

std::optional<FindSendableDependenciesAnalysisResult> deserializeResult(const std::string& data, const FindSendableDependenciesAnalysisPass* astAnalysisPass);
std::optional<FindSendableDependenciesAnalysisResult> deserializeResult(const std::string& data, const SendableAnalysisPass* astAnalysisPass);

std::optional<FindSendableDependenciesAnalysisResult::Dependency> deserializeDependency(const std::string& data, const FindSendableDependenciesAnalysisPass* astAnalysisPass);
std::optional<FindSendableDependenciesAnalysisResult::Dependency> deserializeDependency(const std::string& data, const SendableAnalysisPass* astAnalysisPass);


#endif /* FindSendableDependenciesAnalysisResult_h */
