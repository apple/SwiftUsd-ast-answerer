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

#include "AnalysisResult/PublicInheritanceAnalysisResult.h"
#include "AnalysisPass/PublicInheritanceAnalysisPass.h"
#include <sstream>
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"


PublicInheritanceAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < _publicBases.size(); i++) {
        ss << ASTHelpers::getAsString(_publicBases[i]);
        if (i + 1 < _publicBases.size()) {
            ss << ",, ";
        }
    }
    ss << "]";
    return ss.str();
}

PublicInheritanceAnalysisResult::PublicInheritanceAnalysisResult(std::vector<const clang::CXXRecordDecl*> publicBases) : _publicBases(publicBases) {
    if (publicBases.size() == 0) {
        std::cerr << "Can't create PublicInheritanceAnalysisResult with empty list of public bases" << std::endl;
        __builtin_trap();
    }
    for (const clang::CXXRecordDecl* x : publicBases) {
        if (x == nullptr) {
            std::cout << "Can't create PublicInheritanceAnalysisResult with nullptr in public bases" << std::endl;
            __builtin_trap();
        }
    }
}

/*static*/
std::optional<PublicInheritanceAnalysisResult> PublicInheritanceAnalysisResult::deserialize(const std::string& data, const PublicInheritanceAnalysisPass* astAnalysisPass) {
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    std::string dataTrimBrackets = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
    
    std::vector<const clang::CXXRecordDecl*> publicBases;
    for (const auto& component : components) {
        if (!component.size()) {
            continue;
        }
        
        const clang::TagDecl* tagDecl = astAnalysisPass->findTagDecl(component);
        if (!tagDecl) {
            std::cerr << "Got bad tag while deserializing: " << component << std::endl;
            return std::nullopt;
            __builtin_trap();
        }
        const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
        if (!cxxRecordDecl) {
            std::cerr << "Got tag that isn't a CXXRecord while deserializing: " << component << std::endl;
            return std::nullopt;
            __builtin_trap();
        }
        publicBases.push_back(cxxRecordDecl);
    }
    if (publicBases.size() == 0) {
        std::cerr << "Got no bases while deserializing" << std::endl;
        return std::nullopt;
        __builtin_trap();
    }
    return PublicInheritanceAnalysisResult(publicBases);
}

std::ostream& operator <<(std::ostream& os, const PublicInheritanceAnalysisResult& obj) {
    return os << std::string(obj);
}

const std::vector<const clang::CXXRecordDecl*>& PublicInheritanceAnalysisResult::getPublicBases() const {
    return _publicBases;
}
