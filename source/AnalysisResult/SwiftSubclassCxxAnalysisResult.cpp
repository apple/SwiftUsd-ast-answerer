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

#include "AnalysisResult/SwiftSubclassCxxAnalysisResult.h"
#include "AnalysisPass/SwiftSubclassCxxAnalysisPass.h"
#include <iostream>
#include <sstream>
#include "AnalysisPass/ASTAnalysisPass.h"

template <typename T>
void mergeAtMostOne(std::vector<T>& thisVector, const std::vector<T>& otherVector, const std::string& name) {
    switch (otherVector.size()) {
        case 0:
            break;
            
        case 1:
            thisVector.push_back(otherVector.front());
            break;
            
        default:
            std::cerr << "Error: Cannot merge when other has >1 " << name << "s" << std::endl;
            __builtin_trap();
    }
}

void SwiftSubclassCxxAnalysisResult::merge(const SwiftSubclassCxxAnalysisResult& other) {
    mergeAtMostOne(bases, other.bases, "base");
    if (destructor && other.destructor) {
        std::cerr << "Error: Cannot merge when this and other both have destructors" << std::endl;
        __builtin_trap();
    }
    destructor = destructor ?: other.destructor;
    mergeAtMostOne(constructors, other.constructors, "constructor");
    mergeAtMostOne(methods, other.methods, "method");
    mergeAtMostOne(fields, other.fields, "field");
}

SwiftSubclassCxxAnalysisResult::operator std::string() const {
    std::stringstream ss;
    std::string parentAsString = "unknown-no-destructor";
    if (destructor) {
        parentAsString = ASTHelpers::getAsString(destructor->getParent());
    }
    
    for (const auto& base : bases) {
        ss << parentAsString << "; ";
        switch (base.first) {
            case clang::AS_public:
                ss << "public ";
                break;
            case clang::AS_protected:
                ss << "protected ";
                break;
            case clang::AS_private:
                ss << "private ";
                break;
            case clang::AS_none:
                std::cerr << "Error: Bases must have a non-none specifier" << std::endl;
                __builtin_trap();
                break;
        }
        ss << ASTHelpers::getAsString(base.second) << ";\n";
    }
    if (destructor) {
        ss << parentAsString << "; " << ASTHelpers::getAsString(destructor) << ";\n";
    }
    for (const auto& constructor : constructors) {
        ss << parentAsString << "; " << ASTHelpers::getAsString(constructor) << ";\n";
    }
    for (const auto& method : methods) {
        ss << parentAsString << "; " << ASTHelpers::getAsString(method) << ";\n";
    }
    for (const auto& field : fields) {
        ss << parentAsString << "; " << ASTHelpers::getAsString(field) << ";\n";
    }
    
    std::string result = ss.str();
    // Get rid of the first `TYPE; ` and the final semicolon and newline
    result = result.substr(parentAsString.size() + 2, result.size() - parentAsString.size() - 4);
    return result;
}

std::ostream& operator <<(std::ostream& os, const SwiftSubclassCxxAnalysisResult& obj) {
    return os << std::string(obj);
}

SwiftSubclassCxxAnalysisResult::SwiftSubclassCxxAnalysisResult() :
bases({}), destructor(nullptr), constructors({}), methods({}), fields({}) {}

/* static */
std::optional<SwiftSubclassCxxAnalysisResult> SwiftSubclassCxxAnalysisResult::deserialize(const std::string& data, const SwiftSubclassCxxAnalysisPass* astAnalysisPass) {
    SwiftSubclassCxxAnalysisResult result;
    const clang::NamedDecl* namedDecl = astAnalysisPass->findNamedDecl(data);
    if (namedDecl == nullptr) {
        size_t spaceIndex = data.find(" ");
        if (spaceIndex == std::string::npos || spaceIndex == 0 || spaceIndex == data.size() - 1) {
            return std::nullopt;
        }
        std::string front = data.substr(0, spaceIndex);
        std::string back = data.substr(spaceIndex + 1);
        const clang::CXXRecordDecl* backTag = clang::dyn_cast<clang::CXXRecordDecl>(astAnalysisPass->findTagDecl(back));
        if (backTag == nullptr) {
            return std::nullopt;
        }
        
        if (front == "public") {
            result.bases.push_back({clang::AccessSpecifier::AS_public, backTag});
        } else if (front == "protected") {
            result.bases.push_back({clang::AccessSpecifier::AS_protected, backTag});
        } else if (front == "private") {
            result.bases.push_back({clang::AccessSpecifier::AS_private, backTag});
        } else {
            return std::nullopt;
        }
    } else if (const clang::CXXDestructorDecl* destructor = clang::dyn_cast<clang::CXXDestructorDecl>(namedDecl)) {
        result.destructor = destructor;
    } else if (const clang::CXXConstructorDecl* constructor = clang::dyn_cast<clang::CXXConstructorDecl>(namedDecl)) {
        result.constructors.push_back(constructor);
    } else if (const clang::CXXMethodDecl* method = clang::dyn_cast<clang::CXXMethodDecl>(namedDecl)) {
        result.methods.push_back(method);
    } else if (const clang::FieldDecl* field = clang::dyn_cast<clang::FieldDecl>(namedDecl)) {
        result.fields.push_back(field);
    } else {
        return std::nullopt;
    }
    
    return result;
}
