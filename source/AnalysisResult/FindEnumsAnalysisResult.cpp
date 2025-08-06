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

#include "AnalysisResult/FindEnumsAnalysisResult.h"
#include "Util/TestDataLoader.h"

FindEnumsAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    ss << (isScoped ? "scoped" : "unscoped") << ",, ";
    for (int i = 0; i < caseNames.size(); i++) {
        ss << caseNames[i] << ",, " << caseValues[i];
        if (i + 1 < caseNames.size()) {
            ss << ",, ";
        }
    }
    ss << "]";
    return ss.str();
}

FindEnumsAnalysisResult::FindEnumsAnalysisResult(bool isScoped) : isScoped(isScoped) {}

void FindEnumsAnalysisResult::addCase(const std::string &name, int64_t value) {
    const auto& it = std::find(caseNames.begin(), caseNames.end(), name);
    if (it != caseNames.end()) {
        if (caseValues[it - caseNames.begin()] != value) {
            std::cerr << "Got redeclaration of case with different value:" << std::endl;
            std::cerr << "'" << name << "': " << caseValues[it - caseNames.begin()] << " -> " << value << std::endl;
            __builtin_trap();
        }
        return;
    }
    
    caseNames.push_back(name);
    caseValues.push_back(value);
}

/* static */
std::optional<FindEnumsAnalysisResult> FindEnumsAnalysisResult::deserialize(const std::string& data, const FindEnumsAnalysisPass*) {
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    std::string dataTrimBrackets = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
    if (components.size() % 2 == 0) {
        return std::nullopt;
    }
    if (components[0] != "scoped" && components[0] != "unscoped") {
        return std::nullopt;
    }
    
    FindEnumsAnalysisResult result(components[0] == "scoped");
    for (int i = 1; i + 1 < components.size(); i += 2) {
        result.addCase(components[i], std::stoll(components[i + 1]));
    }
    return result;
}

std::ostream& operator <<(std::ostream& os, const FindEnumsAnalysisResult& obj) {
    return os << std::string(obj);
}
