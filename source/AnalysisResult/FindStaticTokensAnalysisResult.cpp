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

#include "AnalysisResult/FindStaticTokensAnalysisResult.h"
#include "Util/TestDataLoader.h"

FindStaticTokensAnalysisResult::operator std::string() const {
    switch (kind) {
        case importedAsValue: return "importedAsValue";
        case blockedByNonPublicHeaderDefinition: return "blockedByNonPublicHeaderDefinition";
    }
}

/* static */
std::vector<FindStaticTokensAnalysisResult::Kind> FindStaticTokensAnalysisResult::allCases() {
    return {
        importedAsValue,
        blockedByNonPublicHeaderDefinition
    };
}

FindStaticTokensAnalysisResult::FindStaticTokensAnalysisResult(Kind theKind) : kind(theKind) {}

/* static */
std::optional<FindStaticTokensAnalysisResult> FindStaticTokensAnalysisResult::deserialize(const std::string& data, const FindStaticTokensAnalysisPass*) {
    for (auto kind : allCases()) {
        if (std::string(FindStaticTokensAnalysisResult(kind)) == data) {
            return FindStaticTokensAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const FindStaticTokensAnalysisResult& obj) {
    return os << std::string(obj);
}

