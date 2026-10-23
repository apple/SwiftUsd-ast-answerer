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

#include "AnalysisPass/FindNamedDeclsAnalysisPass.h"
#include "Linting/LinterRunner.h"

#include "Linting/StdTypeInfoEqualsEqualsLinter.h"

// MARK: Entry
LinterRunner::~LinterRunner() {}

LinterRunner::LinterRunner(const Driver* driver) :
    _driver(driver)
{
    _stdTypeInfoEqualsEqualsLinter = LinterFactory::makeLinter<StdTypeInfoEqualsEqualsLinter>(this);
}

void LinterRunner::dumpLintingOutputs() const {
    _stdTypeInfoEqualsEqualsLinter->dumpLinterOutput();
}

// MARK: Accessors
const Driver* LinterRunner::getDriver() const {
    return _driver;
}

const clang::TagDecl* LinterRunner::findTagDecl(const std::string &typeName) const {
    return _driver->getASTAnalysisRunner()->findTagDecl(typeName);
}

const clang::NamedDecl* LinterRunner::findNamedDecl(const std::string &name) const {
    return _driver->getASTAnalysisRunner()->findNamedDecl(name);
}

const clang::Type* LinterRunner::findType(const std::string &name) const {
    return _driver->getASTAnalysisRunner()->findType(name);
}

const clang::FunctionDecl* LinterRunner::findFunctionDecl(const std::string &signature) const {
    return _driver->getASTAnalysisRunner()->findFunctionDecl(signature);
}

