//
//  Driver.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/9/24.
//

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

