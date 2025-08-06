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

#include "Util/ClangToolHelper.h"
#include "Util/FileSystemInfo.h"
#include <fstream>
#include <regex>
#include <chrono>
#include <fstream>
#include <random>
#include <thread>
#include <iterator>
#include <unistd.h>
#include "clang/tooling/JSONCompilationDatabase.h"



static llvm::cl::OptionCategory astAnswererCategory("ast-answerer options");
static llvm::cl::extrahelp commonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

ClangToolHelper::ClangToolHelper(const Driver* driver, int argc, const char **argv) :
_driver(driver),
_isValid(true),
_astUnits() {
    
    std::unique_ptr<clang::ASTUnit> temp = loadAST();
    if (!temp) {
        temp = buildAndSaveAST();
        // Workaround: getAsString() for TagDecls that point at class lambdas
        // have relative paths when ASTUnits are made in memory (e.g. buildAndSaveAST()),
        // but absolute paths when ASTUnits are loaded from disk (e.g. loadAST()).
        // To have deserialization not break if you run this twice in a row without
        // clearing the serializedAnalysis, we always load from disk after building the AST. 
        temp = loadAST();
    }
    if (temp) {
        _astUnits.push_back(std::move(temp));
    } else {
        _isValid = false;
    }
}

ClangToolHelper::operator bool() const {
    return _isValid;
}

std::unique_ptr<clang::ASTUnit> ClangToolHelper::buildAST() {
    // Just fake a compilation database, because the ClangTool insists on it
    std::string errorMessage;
    auto compilationDatabase = clang::tooling::FixedCompilationDatabase(".", {});
    
    
    const FileSystemInfo* fileSystemInfo = _driver->getFileSystemInfo();
    std::filesystem::path path = fileSystemInfo->writeSourceFileListIntoFile();
    
    clang::tooling::ClangTool clangTool = clang::tooling::ClangTool(compilationDatabase,
                                                                    {path.string()});
    
    
    addSystemIncludeArguments(clangTool);
    clangTool.appendArgumentsAdjuster([=](const clang::tooling::CommandLineArguments& baseArgs, llvm::StringRef filename) {
        std::vector<std::string> result = baseArgs;
        result.insert(result.begin() + 1, "-ObjC++"); // Make sure we're in Obj-C++ mode
        result.push_back("-std=gnu++17");
        result.push_back("--target=arm64-apple-macos15.0.0");
        result.push_back("-I" + fileSystemInfo->usdSourceRepoPath.string());
        result.push_back("-I" + fileSystemInfo->usdInstalledHeaderPath.string());
        result.push_back("-DPXR_MATERIALX_SUPPORT_ENABLED");
        result.push_back("-w"); // silence warnings because they clog the console when iterating
        // Allow unlimited errors because we have ODR violations from concatenating .cpp files
        // We need this because TfSingleton specializations might only be found in .cpp files, but we need
        // to see those to know if a type can safely be imported as immortal
        result.push_back("-ferror-limit=0");
        return result;
    });
    
    clangTool.appendArgumentsAdjuster([](const clang::tooling::CommandLineArguments& baseArgs, llvm::StringRef filename) {
        std::cout << "Command line arguments are: ";
        for (auto& x : baseArgs) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
        return baseArgs;
    });
        
    std::vector<std::unique_ptr<clang::ASTUnit>> units;
    clangTool.buildASTs(units);
    
    if (units.front()) {
        std::cout << "Built AST for " << path.string() << std::endl;
    }
    
    return std::move(units.front());
}
std::unique_ptr<clang::ASTUnit> ClangToolHelper::loadAST() {
    std::filesystem::path path = _driver->getFileSystemInfo()->getSerializedASTPath();
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }
    
    clang::CompilerInstance ci;
    ci.createDiagnostics();
    std::unique_ptr<clang::ASTUnit> result = clang::ASTUnit::LoadFromASTFile(path, // Filename
                                                                             ci.getPCHContainerReader(), // PCHContainerRdr
                                                                             clang::ASTUnit::LoadEverything, // ToLoad
                                                                             &ci.getDiagnostics(), // Diags
                                                                             ci.getFileSystemOpts(), // FileSystemOpts
                                                                             ci.getHeaderSearchOptsPtr(), // HSOpts
                                                                             nullptr, // LangOpts = nullptr
                                                                             false, // OnlyLocalDecls = false,
                                                                             clang::CaptureDiagsKind::None, // CaptureDiagnostics = CaptureDiagsKind::None
                                                                             true, // AllowASTWithCompilerErrors = false
                                                                             false, // UserFilesAreVolatile = false
                                                                             llvm::vfs::getRealFileSystem() // VFS = llvm::vfs::getRealFileSystem()
                                                                             );
    if (result) {
        std::cout << "Loaded prebuilt AST for " << path.string() << std::endl;
    }
    return result;
}



std::unique_ptr<clang::ASTUnit> ClangToolHelper::buildAndSaveAST() {

    std::unique_ptr<clang::ASTUnit> result = buildAST();
    
    std::filesystem::path savePath = _driver->getFileSystemInfo()->getSerializedASTPath();
    std::filesystem::create_directories(savePath.parent_path());
    result->Save(savePath.string());
    
    return result;
}


const std::vector<std::unique_ptr<clang::ASTUnit>>& ClangToolHelper::getASTUnits() const {
    return _astUnits;
}

void ClangToolHelper::addSystemIncludeArguments(clang::tooling::ClangTool& tool) const {
    tool.appendArgumentsAdjuster([=, this](const clang::tooling::CommandLineArguments& baseArgs, llvm::StringRef filename) {
        std::vector<std::string> result = baseArgs;
        result.insert(result.end(), _driver->getFileSystemInfo()->systemIncludeArguments.begin(), _driver->getFileSystemInfo()->systemIncludeArguments.end());
        return result;
    });
}
