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

#ifndef Driver_h
#define Driver_h

#include <memory>

struct FileSystemInfo;
class CMakeParser;
class ClangToolHelper;
class ASTAnalysisRunner;
class CodeGenRunner;

// Driver is the overall coordinator for the project. It gets passed around
// as needed, and owns and runs different phases of the project.
class Driver {
public:
    Driver(int argc, const char** argv);
    ~Driver();
    
    // MARK: Accessors
    const FileSystemInfo* getFileSystemInfo() const;
    const CMakeParser* getCMakeParser() const;
    const ClangToolHelper* getClangToolHelper() const;
    const ASTAnalysisRunner* getASTAnalysisRunner() const;
    const CodeGenRunner* getCodeGenRunner() const;
    
private:
    // MARK: Fields
    std::unique_ptr<FileSystemInfo> _fileSystemInfo;
    std::unique_ptr<CMakeParser> _cmakeParser;
    std::unique_ptr<ClangToolHelper> _clangToolHelper;
    std::unique_ptr<ASTAnalysisRunner> _astAnalysisRunner;
    std::unique_ptr<CodeGenRunner> _codeGenRunner;

};

#endif /* Driver_h */
