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

#include "Util/TestDataLoader.h"
#include "AnalysisResult/SdfValueTypeNamesMembersAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include <fstream>


SdfValueTypeNamesMembersAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    uint64_t i = 0;
    for (const auto& x : _data) {
        ss << x;
        if (i + 1 < _data.size()) {
            ss << ",, ";
        }
        i += 1;
    }
    ss << "]";
    return ss.str();
}

SdfValueTypeNamesMembersAnalysisResult::SdfValueTypeNamesMembersAnalysisResult() {}
/* static */
std::optional<SdfValueTypeNamesMembersAnalysisResult> SdfValueTypeNamesMembersAnalysisResult::deserialize(const std::string& _data, const SdfValueTypeNamesMembersAnalysisPass* astAnalysisPass) {
    std::string data = _data;
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    
    data = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(data, std::regex(",, "));
    SdfValueTypeNamesMembersAnalysisResult result;
    result._data = components;
    return result;
}

void SdfValueTypeNamesMembersAnalysisResult::push_back(const std::string &s) {
    _data.push_back(s);
}

std::ostream& operator<<(std::ostream& os, const SdfValueTypeNamesMembersAnalysisResult& obj) {
    return os << std::string(obj);
}
