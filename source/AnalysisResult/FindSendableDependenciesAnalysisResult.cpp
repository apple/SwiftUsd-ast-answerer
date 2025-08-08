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

#include "AnalysisResult/FindSendableDependenciesAnalysisResult.h"
#include "AnalysisPass/FindSendableDependenciesAnalysisPass.h"
#include "AnalysisPass/SendableAnalysisPass.h"
#include <sstream>
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"


FindSendableDependenciesAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < dependencies.size(); i++) {
        ss << dependencies[i];
        if (i + 1 < dependencies.size()) {
            ss << ",, ";
        }
    }
    ss << "]";
    return ss.str();
}

FindSendableDependenciesAnalysisResult::FindSendableDependenciesAnalysisResult() {}

template <typename Pass>
std::optional<FindSendableDependenciesAnalysisResult> deserializeResultImpl(const std::string& data, const Pass* astAnalysisPass) {
    
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    std::string dataTrimBrackets = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
    
    FindSendableDependenciesAnalysisResult result;

    for (const auto& component : components) {
        if (!component.size()) {
            continue;
        }
        std::optional<FindSendableDependenciesAnalysisResult::Dependency> dependency = deserializeDependencyImpl(component, astAnalysisPass);
        if (!dependency) {
            std::cerr << "Got bad dependency while deserializing: " << component << std::endl;
            deserializeDependencyImpl(component, astAnalysisPass);
            __builtin_trap();
        }
        result.dependencies.push_back(*dependency);
    }
    
    return result;
}

FindSendableDependenciesAnalysisResult::Dependency::Dependency(DependencyKind kind, std::string fieldName, const clang::Type* typeParam) : kind(kind), fieldName(fieldName), type(typeParam) {
    if (type) {
        type = type->getCanonicalTypeUnqualified().getTypePtr();
    }
}


FindSendableDependenciesAnalysisResult::Dependency::operator std::string() const {
    switch (kind) {
        case inheritance: return "$inheritance " + ASTHelpers::getAsString(type);
        case field: return ASTHelpers::getAsString(type) + " " + fieldName;
        case specialAvailable: return "$specialAvailable";
        case specialImportedAsReference: return "$specialImportedAsReference";
        case specialConditional: return "$specialConditional " + ASTHelpers::getAsString(type);
    }
}

template <typename Pass>
std::optional<FindSendableDependenciesAnalysisResult::Dependency> deserializeDependencyImpl(const std::string &data, const Pass *astAnalysisPass) {
    
    if (data.starts_with("$inheritance ")) {
        std::string maybeTypeString = data.substr(std::string("$inheritance ").size());
        const clang::Type* maybeType = astAnalysisPass->findType(maybeTypeString);
        if (!maybeType) {
            return std::nullopt;
        }
        return FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::inheritance, "", maybeType);
    }
    
    if (data == "$specialAvailable") {
        return FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::specialAvailable, "", nullptr);
    }
    
    if (data == "$specialImportedAsReference") {
        return FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::specialImportedAsReference, "", nullptr);
    }
    
    if (data.starts_with("$specialConditional ")) {
        std::string maybeTypeString = data.substr(std::string("$specialConditional ").size());
        const clang::Type* maybeType = astAnalysisPass->findType(maybeTypeString);
        if (!maybeType) {
            return std::nullopt;
        }
        return FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::specialConditional, "", maybeType);
    }
    
    if (true) {
        auto spacePos = data.rfind(" ");
        if (spacePos == std::string::npos) {
            return std::nullopt;
        }
        std::string fieldName = data.substr(spacePos + 1);
        std::string maybeTypeString = data.substr(0, spacePos);
        const clang::Type* maybeType = astAnalysisPass->findType(maybeTypeString);
        if (!maybeType) {
            return std::nullopt;
        }
        
        return FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::field, fieldName, maybeType);
    }
    
    return std::nullopt;
}

void FindSendableDependenciesAnalysisResult::addInheritanceDependency(clang::CXXBaseSpecifier base) {
    dependencies.push_back(Dependency(inheritance, "", base.getType().getTypePtr()));
}

void FindSendableDependenciesAnalysisResult::addFieldDependency(const clang::FieldDecl *fieldDecl) {
    dependencies.push_back(Dependency(field, fieldDecl->getNameAsString(), fieldDecl->getType().getTypePtr()));
}

std::ostream& operator <<(std::ostream& os, const FindSendableDependenciesAnalysisResult& obj) {
    return os << std::string(obj);
}
std::ostream& operator <<(std::ostream& os, const FindSendableDependenciesAnalysisResult::Dependency& obj) {
    return os << std::string(obj);
}

std::optional<FindSendableDependenciesAnalysisResult> deserializeResult(const std::string& data, const FindSendableDependenciesAnalysisPass* astAnalysisPass) {
    return deserializeResultImpl(data, astAnalysisPass);
}
std::optional<FindSendableDependenciesAnalysisResult> deserializeResult(const std::string& data, const SendableAnalysisPass* astAnalysisPass) {
    return deserializeResultImpl(data, astAnalysisPass);
}

std::optional<FindSendableDependenciesAnalysisResult::Dependency> deserializeDependency(const std::string& data, const FindSendableDependenciesAnalysisPass* astAnalysisPass) {
    return deserializeDependencyImpl(data, astAnalysisPass);
}
std::optional<FindSendableDependenciesAnalysisResult::Dependency> deserializeDependency(const std::string& data, const SendableAnalysisPass* astAnalysisPass) {
    return deserializeDependencyImpl(data, astAnalysisPass);
}
