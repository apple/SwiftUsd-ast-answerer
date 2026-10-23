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

#ifndef LinterRunner_h
#define LinterRunner_h

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "Driver/Driver.h"
#include "Util/ClangToolHelper.h"

#include <memory>
#include <set>

class StdTypeInfoEqualsEqualsLinter;

// Owns and coordinates running different linters
class LinterRunner {
public:
    // MARK: Entry
    LinterRunner(const Driver* driver);
    ~LinterRunner();
    
    void dumpLintingOutputs() const;
    
public:
    // MARK: Accessors
    const Driver* getDriver() const;
        
    const clang::TagDecl* findTagDecl(const std::string& typeName) const;
    const clang::NamedDecl* findNamedDecl(const std::string& name) const;
    const clang::Type* findType(const std::string& name) const;
    const clang::FunctionDecl* findFunctionDecl(const std::string& signature) const;

private:
    // MARK: Fields
    const Driver* _driver;

    std::unique_ptr<StdTypeInfoEqualsEqualsLinter> _stdTypeInfoEqualsEqualsLinter;
};

#endif /* LinterRunner_h */
