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

#include "AnalysisResult/CustomStringConvertibleAnalysisResult.h"

bool CustomStringConvertibleAnalysisResult::isAvailable() const {
    switch (_kind) {
        case unknown: return false;
        case noAnalysisBecauseBlockedByImport: return false;
        case noAnalysisBecauseNonCopyable: return false;
        case blockedByNoCandidate: return false;
        case available: return true;
        case availableEnum: return true;
        case availableUsdObjectSubclass: return true;
        case availableSdfSpecSubclass: return true;
        case availableSdfSpecHandleSubclass: return true;
        case availableUsdMetadataValueMap: return true;
        case availableUsdGeomXformOp: return true;
    }
}

CustomStringConvertibleAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: return "unknown";
        case noAnalysisBecauseBlockedByImport: return "noAnalysisBecauseBlockedByImport";
        case noAnalysisBecauseNonCopyable: return "noAnalysisBecauseNonCopyable";
        case blockedByNoCandidate: return "blockedByNoCandidate";
        case available: return "available";
        case availableEnum: return "availableEnum";
        case availableUsdObjectSubclass: return "availableUsdObjectSubclass";
        case availableSdfSpecSubclass: return "availableSdfSpecSubclass";
        case availableSdfSpecHandleSubclass: return "availableSdfSpecHandleSubclass";
        case availableUsdMetadataValueMap: return "availableUsdMetadataValueMap";
        case availableUsdGeomXformOp: return "availableUsdGeomXformOp";
        default: return "__errCase";
    }
}

/* static */
std::vector<CustomStringConvertibleAnalysisResult::Kind> CustomStringConvertibleAnalysisResult::allCases() {
    return {
        unknown,
        noAnalysisBecauseBlockedByImport,
        noAnalysisBecauseNonCopyable,
        blockedByNoCandidate,
        available,
        availableEnum,
        availableUsdObjectSubclass,
        availableSdfSpecSubclass,
        availableSdfSpecHandleSubclass,
        availableUsdMetadataValueMap,
        availableUsdGeomXformOp,
    };
}

CustomStringConvertibleAnalysisResult::CustomStringConvertibleAnalysisResult() : CustomStringConvertibleAnalysisResult(unknown) {}

CustomStringConvertibleAnalysisResult::CustomStringConvertibleAnalysisResult(CustomStringConvertibleAnalysisResult::Kind kind) :
    _kind(kind) {}

/* static */
std::optional<CustomStringConvertibleAnalysisResult> CustomStringConvertibleAnalysisResult::deserialize(const std::string &data, const CustomStringConvertibleAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(CustomStringConvertibleAnalysisResult(kind)) == data) {
            return CustomStringConvertibleAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const CustomStringConvertibleAnalysisResult& obj) {
    return os << std::string(obj);
}
