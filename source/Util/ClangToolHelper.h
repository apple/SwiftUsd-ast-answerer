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

#ifndef ClangToolHelper_h
#define ClangToolHelper_h

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/Mangle.h>
#include "clang/Frontend/CompilerInstance.h"

#include <llvm/Support/CommandLine.h>
#include <iostream>
#include <filesystem>

#include "Driver/Driver.h"

// Helps with building the clang AST, and serializing and loading it from disk. 
class ClangToolHelper {
public:
    ClangToolHelper(const Driver* driver, int argc, const char** argv);
        
    explicit operator bool() const;
    
    const std::vector<std::unique_ptr<clang::ASTUnit>>& getASTUnits() const;
    

private:
    std::unique_ptr<clang::ASTUnit> buildAndSaveAST();
    std::unique_ptr<clang::ASTUnit> loadAST();
    std::unique_ptr<clang::ASTUnit> buildAST();
    
    void addSystemIncludeArguments(clang::tooling::ClangTool& tool) const;
    
private:
    const Driver* _driver;
    bool _isValid;
    std::vector<std::unique_ptr<clang::ASTUnit>> _astUnits;
};


#endif /* ClangToolHelper_h */
