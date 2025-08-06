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

#ifndef BinaryOpProtocolAnalysisResult_h
#define BinaryOpProtocolAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

template <typename Derived>
class BinaryOpProtocolAnalysisPassBase;

struct BinaryOpProtocolAnalysisResult {
public:
    enum Kind {
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
    
    bool isAvailable() const;
    
    std::string witnessFirstType;
    std::string witnessSecondType;

private:
    template <typename Derived>
    friend class BinaryOpProtocolAnalysisPassBase;
    friend class EquatableCodeGen;
    friend class ComparableCodeGen;
    
    Kind _kind;
    
public:
    static std::string getAsString(Kind kind);
    explicit operator std::string() const;
    static std::vector<Kind> allCases();

    BinaryOpProtocolAnalysisResult();
    BinaryOpProtocolAnalysisResult(Kind kind);
    BinaryOpProtocolAnalysisResult(Kind kind, const std::string& witnessFirstType, const std::string& witnessSecondType);
    template <typename Derived>
    static std::optional<BinaryOpProtocolAnalysisResult> deserialize(const std::string& data, const BinaryOpProtocolAnalysisPassBase<Derived>*) {
        return deserializeImpl(data);
    }
    
    static std::optional<BinaryOpProtocolAnalysisResult> deserializeImpl(const std::string& data);

};
std::ostream& operator <<(std::ostream& os, const BinaryOpProtocolAnalysisResult& obj);


#endif /* BinaryOpProtocolAnalysisResult_h */
