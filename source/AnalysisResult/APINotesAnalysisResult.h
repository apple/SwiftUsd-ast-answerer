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

#ifndef APINotesAnalysisResult_h
#define APINotesAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class APINotesAnalysisPass;

struct APINotesAnalysisResult {
public:
    enum class Kind {
        importTagAsShared,
        importTagAsImmortal,
        importTagAsOwned,
        makeFunctionUnavailable,
        replaceMutatingFunctionWithNonmutatingWrapper,
        replaceConstRefFunctionWithCopyingWrapper,
        renameTfNoticeRegisterFunctionSpecialCase,
        markTfRemnantAsUnavailableImmortalFrtSpecialCase,
        renameSdfZipFileIteratorSpecialCase
    };
        
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    APINotesAnalysisResult(Kind kind);
    
    static std::optional<APINotesAnalysisResult> deserialize(const std::string& data, const APINotesAnalysisPass* astAnalysisPass);
    
    Kind getKind() const;
    
private:
    Kind _kind;
};

std::ostream& operator <<(std::ostream& os, const APINotesAnalysisResult& obj);

#endif /* APINotesAnalysisResult_h */
