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

#include "AnalysisResult/HashableAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

HashableAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: return "unknown";
        case blockedByImport: return "blockedByImport";
        case noAnalysisBecauseNonCopyable: return "noAnalysisBecauseNonCopyable";
        case foundCandidateButBlockedByEquatable: return "foundCandidateButBlockedByEquatable";
        case blockedByNoCandidate: return "blockedByNoCandidate";
        case available: return "available";
        default: return "__errCase";
    }
}

bool HashableAnalysisResult::isAvailable() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByImport: return false;
        case noAnalysisBecauseNonCopyable: return false;
        case foundCandidateButBlockedByEquatable: return false;
        case blockedByNoCandidate: return false;
        case available: return true;
    }
}

/* static */
std::vector<HashableAnalysisResult::Kind> HashableAnalysisResult::allCases() {
    return {
        unknown,
        blockedByImport,
        noAnalysisBecauseNonCopyable,
        foundCandidateButBlockedByEquatable,
        blockedByNoCandidate,
        available
    };
}

HashableAnalysisResult::HashableAnalysisResult() : HashableAnalysisResult(unknown) {}

HashableAnalysisResult::HashableAnalysisResult(HashableAnalysisResult::Kind kind) : _kind(kind) {}

/* static */
std::optional<HashableAnalysisResult> HashableAnalysisResult::deserialize(const std::string &data, const HashableAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(HashableAnalysisResult(kind)) == data) {
            return HashableAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const HashableAnalysisResult& obj) {
    return os << std::string(obj);
}

