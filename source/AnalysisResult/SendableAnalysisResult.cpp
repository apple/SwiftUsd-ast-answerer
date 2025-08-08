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

#include "AnalysisResult/SendableAnalysisResult.h"
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/SendableAnalysisPass.h"

bool SendableAnalysisResult::isAvailable() const {
    switch (_kind) {
        case unknown: return false;
        case unavailable: return false;
        case available: return true;
    }
}

std::string SendableAnalysisResult::getAsString(Kind kind) {
    switch (kind) {
        case unknown: return "unknown";
        case unavailable: return "unavailable";
        case available: return "available";
    }
}

SendableAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: return getAsString(_kind);
        case unavailable: return getAsString(_kind) + " " + std::string(_unavailableDependencies);
        case available: return getAsString(_kind);
    }
}

std::vector<SendableAnalysisResult::Kind> SendableAnalysisResult::allCases() {
    return {
        unknown,
        unavailable,
        available,
    };
}

SendableAnalysisResult::SendableAnalysisResult() : _kind(unknown) {}

SendableAnalysisResult::SendableAnalysisResult(Kind kind) : _kind(kind) {}

SendableAnalysisResult::SendableAnalysisResult(Kind kind, FindSendableDependenciesAnalysisResult unavailableDependencies) : _unavailableDependencies(unavailableDependencies), _kind(kind) {}


std::optional<SendableAnalysisResult> SendableAnalysisResult::deserialize(const std::string& data, const SendableAnalysisPass* astAnalysisPass) {
    
    for (auto kind : allCases()) {
        if (std::string(SendableAnalysisResult(kind)) == data) {
            return SendableAnalysisResult(kind);
        }

        if (data.starts_with(getAsString(unavailable) + " ")) {
            std::string dataPastUnavailable = data.substr((getAsString(unavailable) + " ").size(), std::string::npos);
            std::optional<FindSendableDependenciesAnalysisResult> unavailableDependencies = FindSendableDependenciesAnalysisResult::deserialize(dataPastUnavailable, astAnalysisPass);
            if (unavailableDependencies) {
                return SendableAnalysisResult(unavailable, *unavailableDependencies);
            }
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const SendableAnalysisResult& obj) {
    return os << std::string(obj);
}
