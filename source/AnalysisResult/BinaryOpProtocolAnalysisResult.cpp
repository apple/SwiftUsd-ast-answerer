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

#include "AnalysisResult/BinaryOpProtocolAnalysisResult.h"
#include "Util/TestDataLoader.h"


bool BinaryOpProtocolAnalysisResult::isAvailable() const {
    switch (_kind) {
        case unknown: return false;
        case noAnalysisBecauseBlockedByImport: return false;
        case noAnalysisBecauseNonCopyable: return false;
        case unavailable: return false;
        case unavailableBlockedByEquatable: return false;
        case availableFoundBySwift: return true;
        case availableShouldBeFoundBySwiftButIsnt: return true;
        case availableImportedAsReference: return true;
        case availableClassTemplateSpecialization: return true;
        case availableFriendFunction: return true;
        case availableInlineMethodDefinedAfterDeclaration: return true;
        case availableDifferentArgumentTypes: return true;
    }
}

std::string BinaryOpProtocolAnalysisResult::getAsString(BinaryOpProtocolAnalysisResult::Kind kind) {
    switch (kind) {
        case unknown: return "unknown";
        case noAnalysisBecauseBlockedByImport: return "noAnalysisBecauseBlockedByImport";
        case noAnalysisBecauseNonCopyable: return "noAnalysisBecauseNonCopyable";
        case unavailable: return "unavailable";
        case unavailableBlockedByEquatable: return "unavailableBlockedByEquatable";
        case availableFoundBySwift: return "availableFoundBySwift";
        case availableShouldBeFoundBySwiftButIsnt: return "availableShouldBeFoundBySwiftButIsnt";
        case availableImportedAsReference: return "availableImportedAsReference";
        case availableClassTemplateSpecialization: return "availableClassTemplateSpecialization";
        case availableFriendFunction: return "availableFriendFunction";
        case availableInlineMethodDefinedAfterDeclaration: return "availableInlineMethodDefinedAfterDeclaration";
        case availableDifferentArgumentTypes: return "availableDifferentArgumentTypes";
        default: return "__errCase";
    }
}

BinaryOpProtocolAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: // fallthrough
        case noAnalysisBecauseBlockedByImport: // fallthrough
        case noAnalysisBecauseNonCopyable: // fallthrough
        case unavailable: // fallthrough
        case unavailableBlockedByEquatable: // fallthrough
        case availableFoundBySwift: // fallthrough
        case availableShouldBeFoundBySwiftButIsnt: // fallthrough
        case availableImportedAsReference: // fallthrough
        case availableClassTemplateSpecialization: // fallthrough
        case availableFriendFunction: // fallthrough
        case availableInlineMethodDefinedAfterDeclaration: return getAsString(_kind);
        case availableDifferentArgumentTypes: {
            std::stringstream ss;
            ss << "[";
            ss << getAsString(_kind) << ",, ";
            ss << witnessFirstType << ",, ";
            ss << witnessSecondType << "]";
            return ss.str();
        }
    }
}

/* static */
std::vector<BinaryOpProtocolAnalysisResult::Kind> BinaryOpProtocolAnalysisResult::allCases() {
    return {
        unknown,
        noAnalysisBecauseBlockedByImport,
        noAnalysisBecauseNonCopyable,
        unavailable,
        unavailableBlockedByEquatable,
        availableFoundBySwift,
        availableShouldBeFoundBySwiftButIsnt,
        availableImportedAsReference,
        availableClassTemplateSpecialization,
        availableFriendFunction,
        availableInlineMethodDefinedAfterDeclaration,
        availableDifferentArgumentTypes,
    };
}

BinaryOpProtocolAnalysisResult::BinaryOpProtocolAnalysisResult() : BinaryOpProtocolAnalysisResult(unknown) {}

BinaryOpProtocolAnalysisResult::BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::Kind kind) :
    _kind(kind) {}

BinaryOpProtocolAnalysisResult::BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::Kind kind, const std::string& witnessFirstType, const std::string& witnessSecondType) :
witnessFirstType(witnessFirstType),
witnessSecondType(witnessSecondType),
_kind(kind)
{}


/* static */
std::optional<BinaryOpProtocolAnalysisResult> BinaryOpProtocolAnalysisResult::deserializeImpl(const std::string& data) {
    
    for (auto kind : allCases()) {
        if (std::string(BinaryOpProtocolAnalysisResult(kind)) == data) {
            return BinaryOpProtocolAnalysisResult(kind);
        }
        
        if (data.starts_with("[") && data.ends_with("]")) {
            std::string dataTrimBrackets = data.substr(1, data.size() - 2);
            std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
            if (components.size() != 3) {
                continue;
            }
            if (components[0] == std::string(getAsString(kind))) {
                return BinaryOpProtocolAnalysisResult(kind, components[1], components[2]);
            }
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const BinaryOpProtocolAnalysisResult& obj) {
    return os << std::string(obj);
}
