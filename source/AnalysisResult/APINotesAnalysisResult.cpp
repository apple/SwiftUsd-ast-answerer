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

#include "AnalysisResult/APINotesAnalysisResult.h"
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/APINotesAnalysisPass.h"

APINotesAnalysisResult::operator std::string() const {
    switch (_kind) {
        case Kind::importTagAsShared: return "importTagAsShared";
        case Kind::importTagAsImmortal: return "importTagAsImmortal";
        case Kind::importTagAsOwned: return "importTagAsOwned";
        case Kind::makeFunctionUnavailable: return "makeFunctionUnavailable";
        case Kind::replaceMutatingFunctionWithNonmutatingWrapper: return "replaceMutatingFunctionWithNonmutatingWrapper";
        case Kind::replaceConstRefFunctionWithCopyingWrapper: return "replaceConstRefFunctionWithCopyingWrapper";
        case Kind::renameTfNoticeRegisterFunctionSpecialCase: return "renameTfNoticeRegisterFunctionSpecialCase";
        case Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase: return "markTfRemnantAsUnavailableImmortalFrtSpecialCase";
        case Kind::renameSdfZipFileIteratorSpecialCase: return "renameSdfZipFileIteratorSpecialCase";
    }
}

/* static */
std::vector<APINotesAnalysisResult::Kind> APINotesAnalysisResult::allCases() {
    return {
        Kind::importTagAsShared,
        Kind::importTagAsImmortal,
        Kind::importTagAsOwned,
        Kind::makeFunctionUnavailable,
        Kind::replaceMutatingFunctionWithNonmutatingWrapper,
        Kind::replaceConstRefFunctionWithCopyingWrapper,
        Kind::renameTfNoticeRegisterFunctionSpecialCase,
        Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase,
        Kind::renameSdfZipFileIteratorSpecialCase,
    };
}

APINotesAnalysisResult::APINotesAnalysisResult(APINotesAnalysisResult::Kind kind) : _kind(kind) {}

/* static */
std::optional<APINotesAnalysisResult> APINotesAnalysisResult::deserialize(const std::string& data, const APINotesAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(APINotesAnalysisResult(kind)) == data) {
            return APINotesAnalysisResult(kind);
        }
    }
    return std::nullopt;
}

APINotesAnalysisResult::Kind APINotesAnalysisResult::getKind() const {
    return _kind;
}

std::ostream& operator <<(std::ostream& os, const APINotesAnalysisResult& obj) {
    return os << std::string(obj);
}
