/*
//
//  OpenUSDSwiftModuleCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/22/24.
//

#include "CodeGen/OpenUSDSwiftModuleCodeGen.h"

OpenUSDSwiftModuleCodeGen::OpenUSDSwiftModuleCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<OpenUSDSwiftModuleCodeGen>(codeGenRunner) {
}

std::string OpenUSDSwiftModuleCodeGen::fileNamePrefix() const {
    return "OpenUSD";
}

OpenUSDSwiftModuleCodeGen::Data OpenUSDSwiftModuleCodeGen::preprocess() const {
    return {};
}

void OpenUSDSwiftModuleCodeGen::prepareWriteSwiftFile() {
    setWritesPrologue(false);
}

std::optional<std::string> OpenUSDSwiftModuleCodeGen::getModuleToAdd(const std::filesystem::path& relativePathOfPxrLibrary) const {
    const FileSystemInfo* fileSystemInfo = getAstAnalysisRunner().getDriver()->getFileSystemInfo();
    
    std::filesystem::path absolutePathOfPxrLibrary = fileSystemInfo->usdSourceRepoPath / relativePathOfPxrLibrary;
    
    std::vector<std::string> components;
    for (const auto& x : relativePathOfPxrLibrary) {
        components.push_back(x);
    }
    
    std::string moduleToAdd;
    for (int i = 0; i < components.size(); i++) {
        moduleToAdd += components[i];
        if (i + 1 < components.size()) {
            moduleToAdd += ".";
        }
    }
    
    return moduleToAdd;
}

void OpenUSDSwiftModuleCodeGen::writeSwiftFile(const Data &data) {
    writeLines({
        "/// C++ modulemap modules.",
        "",
        "// use @_exported to allow clients that `import OpenUSD` to get access",
        "// to all these modules without having to import them one by one.",
        "// use @_documentation(visibility: internal) to hide imported symbols",
        "// from showing up in DocC (e.g. Boost macros, public constants)",
    });
    writeLine("@_documentation(visibility: internal) @_exported import pxr");
    const CMakeParser* cmakeParser = this->getAstAnalysisRunner().getDriver()->getCMakeParser();
    std::set<std::string> writtenModules;
    for (const std::filesystem::path& lib : cmakeParser->getRelativePathsOfPxrLibraries()) {
        
        std::optional<std::string> moduleToAdd = getModuleToAdd(lib);
        if (!moduleToAdd) {
            continue;
        }
        
        if (writtenModules.contains(*moduleToAdd)) { continue; }
        writtenModules.insert(*moduleToAdd);
        writeLine("@_documentation(visibility: internal) @_exported import " + *moduleToAdd);
    }
    
    writeLines({
        "",
        "",
        "/// Swift modules",
        "",
        "// use @_exported to allow clients that `import OpenUSD` to get access",
        "// to all these modules without having to import them one by one.",
        "// might be worth changing CMake to prefix the swiftmodule names with pxr?",
        "// but that might break something if Swift sees a conflict instead of",
        "// merging two modules with the same name",
    });
#warning Hardcoding swiftmodule names
    std::vector<std::string> swiftmoduleNames = {"swiftUsd"};
    for (const auto& x : swiftmoduleNames) {
        writeLine("@_exported import " + x);
    }
        
    writeLines({
        "",
        "",
        "/// Typealiases",
        "",
        "// Makes it easier for clients to access Usd types without",
        "// having to do the full typealias",
        "// use @_documentation(visibility: internal) to hide this",
        "// from showing up in DocC",
        "@_documentation(visibility: internal) public typealias pxr = " PXR_NS,
    });
}
*/
