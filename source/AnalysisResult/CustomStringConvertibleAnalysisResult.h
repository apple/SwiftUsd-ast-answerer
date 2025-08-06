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

#ifndef CustomStringConvertibleAnalysisResult_h
#define CustomStringConvertibleAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class CustomStringConvertibleAnalysisPass;

struct CustomStringConvertibleAnalysisResult {
public:
    enum Kind {
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
    
    bool isAvailable() const;

private:
    friend class CustomStringConvertibleAnalysisPass;
    friend class CustomStringConvertibleCodeGen;
    Kind _kind;

public:
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    
    CustomStringConvertibleAnalysisResult();
    CustomStringConvertibleAnalysisResult(Kind kind);
    static std::optional<CustomStringConvertibleAnalysisResult> deserialize(const std::string& data, const CustomStringConvertibleAnalysisPass* ASTAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const CustomStringConvertibleAnalysisResult& obj);

#endif /* CustomStringConvertibleAnalysisResult_h */
