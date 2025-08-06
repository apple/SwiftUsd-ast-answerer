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

#include "AnalysisPass/BinaryOpProtocolAnalysisPassBase.h"
#include "AnalysisPass/EquatableAnalysisPass.h"
#include "AnalysisPass/ComparableAnalysisPass.h"

BinaryOpFunctionWitnessSet::BinaryOpFunctionWitnessSet() {};

void BinaryOpFunctionWitnessSet::insert(clang::QualType a, clang::QualType b, BinaryOpFunctionWitnessSet::Properties properties) {
    std::pair<clang::QualType, clang::QualType> pair = std::minmax(a.getCanonicalType(), b.getCanonicalType());
    _data[pair] = properties;
}

bool BinaryOpFunctionWitnessSet::find(clang::QualType a, clang::QualType b, BinaryOpFunctionWitnessSet::Properties* properties) {
    std::pair<clang::QualType, clang::QualType> pair = std::minmax(a.getCanonicalType(), b.getCanonicalType());
    
    const auto& it = _data.find(pair);
    if (it == _data.end()) {
        return false;
    }
    *properties = it->second;
    return true;
}




template<>
bool BinaryOpProtocolAnalysisPassBase<EquatableAnalysisPass>::isComparablePass() const {
    return false;
}

template <>
bool BinaryOpProtocolAnalysisPassBase<EquatableAnalysisPass>::checkIfBlockedByEquatable(const clang::CXXRecordDecl* cxxRecordDecl) {
    return false;
}



template <>
bool BinaryOpProtocolAnalysisPassBase<ComparableAnalysisPass>::checkIfBlockedByEquatable(const clang::CXXRecordDecl* cxxRecordDecl) {
   const EquatableAnalysisPass* equatableAnalysisPass = getASTAnalysisRunner().getEquatableAnalysisPass();
   const auto& equatableIt = equatableAnalysisPass->find(cxxRecordDecl);
   if (equatableIt == equatableAnalysisPass->end() || !equatableIt->second.isAvailable()) {
       insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::unavailableBlockedByEquatable);
       return true;
   }
    return false;
}

template<>
bool BinaryOpProtocolAnalysisPassBase<ComparableAnalysisPass>::isComparablePass() const {
    return true;
}


