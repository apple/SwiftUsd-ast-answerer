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

#ifndef FindSchemasAnalysisResult_h
#define FindSchemasAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindSchemasAnalysisPass;

struct FindSchemasAnalysisResult {
    explicit operator std::string() const;
    
    FindSchemasAnalysisResult();
    static std::optional<FindSchemasAnalysisResult> deserialize(const std::string& data, const FindSchemasAnalysisPass* astAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const FindSchemasAnalysisResult& obj);

#endif /* FindSchemasAnalysisResult */
