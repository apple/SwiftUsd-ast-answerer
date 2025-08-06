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

#include "AnalysisResult/FindTfNoticeSubclassesAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

FindTfNoticeSubclassesAnalysisResult::operator std::string() const {
    return ".";
}

std::optional<FindTfNoticeSubclassesAnalysisResult> FindTfNoticeSubclassesAnalysisResult::deserialize(const std::string& data, const FindTfNoticeSubclassesAnalysisPass* astAnalysisPass) {
    return FindTfNoticeSubclassesAnalysisResult();
}

std::ostream& operator <<(std::ostream& os, const FindTfNoticeSubclassesAnalysisResult& obj) {
    return os << std::string(obj);
}
