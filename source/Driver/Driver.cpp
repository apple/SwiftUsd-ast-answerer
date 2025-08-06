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

#include "Driver/Driver.h"
#include "Util/FileSystemInfo.h"
#include "Util/CMakeParser.h"
#include "Util/ClangToolHelper.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include "CodeGen/CodeGenRunner.h"
#include "Util/Graph.h"

Driver::Driver(int argc, const char** argv) {
    testDirectedGraph();
    
    _fileSystemInfo = std::make_unique<FileSystemInfo>(this);
    _cmakeParser = std::make_unique<CMakeParser>(this);
    _clangToolHelper = std::make_unique<ClangToolHelper>(this, argc, argv);
    if (!_clangToolHelper->operator bool()) {
        __builtin_trap();
    }
    _astAnalysisRunner = std::make_unique<ASTAnalysisRunner>(this);
    _codeGenRunner = std::make_unique<CodeGenRunner>(this);
}

Driver::~Driver() {
    
}

// MARK: Accessors

const FileSystemInfo* Driver::getFileSystemInfo() const {
    return _fileSystemInfo.get();
}
const CMakeParser* Driver::getCMakeParser() const {
    return _cmakeParser.get();
}
const ClangToolHelper* Driver::getClangToolHelper() const {
    return _clangToolHelper.get();
}
const ASTAnalysisRunner* Driver::getASTAnalysisRunner() const {
    return _astAnalysisRunner.get();
}
const CodeGenRunner* Driver::getCodeGenRunner() const {
    return _codeGenRunner.get();
}

