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

#include "Util/TestDataLoader.h"
#include "AnalysisResult/TypedefAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/TypedefAnalysisPass.h"
#include <fstream>

TypedefAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    uint64_t i = 0;
    for (const auto& x : _typedefSpellings) {
        ss << ASTHelpers::getAsString(x.first) << ",, ";
        ss << x.second;
        if (i + 1 < _typedefSpellings.size()) {
            // Use two commas because that's weird in C++,
            // and we're dealing with type names without properly parsing them
            ss << ",, ";
        }
        i += 1;
    }
    ss << "]";
    return ss.str();
}

TypedefAnalysisResult::TypedefAnalysisResult() {}


/* static */
std::optional<TypedefAnalysisResult> TypedefAnalysisResult::deserialize(const std::string& _data, const TypedefAnalysisPass* astAnalysisPass) {
    
    std::string data = _data;
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    
    data = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(data, std::regex(",, "));
    if (components.size() % 2 == 1) {
        return std::nullopt;
    }
    
    TypedefAnalysisResult result;
    for (int i = 0; i < components.size(); i += 2) {
        const clang::NamedDecl* namedDecl = astAnalysisPass->findNamedDecl(components[i]);
        if (!namedDecl) {
            return std::nullopt;
        }
        result.insert(namedDecl, components[i + 1]);
    }
    
    return result;
}

void TypedefAnalysisResult::insert(const clang::NamedDecl* namedDecl, const std::string& typedefSpelling) {
    _typedefSpellings.insert({namedDecl, typedefSpelling});
}

bool TypedefAnalysisResult::isSubset(const TypedefAnalysisResult& other) const {
    for (const auto& thisElement : this->_typedefSpellings)  {
        bool foundMatch = false;
        for (const auto& otherElement : other._typedefSpellings) {
            if (thisElement.first->getCanonicalDecl() == otherElement.first->getCanonicalDecl()) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            return false;
        }
    }
    return true;
}

const std::set<std::pair<const clang::NamedDecl *, std::string>>& TypedefAnalysisResult::getTypedefSpellings() const {
    return _typedefSpellings;
}

std::ostream& operator<<(std::ostream& os, const TypedefAnalysisResult& obj) {
    return os << std::string(obj);
}
