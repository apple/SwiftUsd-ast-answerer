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

#include "AnalysisResult/FindSchemasAnalysisResult.h"
#include "AnalysisPass/FindSchemasAnalysisPass.h"

FindSchemasAnalysisResult::FindSchemasAnalysisResult() {}

FindSchemasAnalysisResult::operator std::string() const { return "."; }

/* static */
std::optional<FindSchemasAnalysisResult> FindSchemasAnalysisResult::deserialize(const std::string& data, const FindSchemasAnalysisPass* astAnalysisPass) {
    return FindSchemasAnalysisResult();
}

std::ostream& operator <<(std::ostream& os, const FindSchemasAnalysisResult& obj) {
    return os << std::string(obj);
}
