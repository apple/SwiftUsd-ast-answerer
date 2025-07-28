//
//  FileSystemInfo.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/3/24.
//

#include <random>
#include <iostream>
#include <regex>
#include <fstream>

#include "Util/FileSystemInfo.h"
#include "Util/CMakeParser.h"

FileSystemInfo::FileSystemInfo(const Driver* driver) :
    _driver(driver),
    // Absolute paths set by CMake. See other C++ flags in Build Settings.
    usdSourceRepoPath(USD_SOURCE_REPO_PATH),
    usdInstalledHeaderPath(USD_INSTALL_NO_PYTHON_PATH + std::string("/include")),
    astAnswererRepoPath(AST_ANSWERER_REPO_PATH),
    astAnswererBuildPath(AST_ANSWERER_BUILD_PATH),
    usdDocumentationAttributionLinkPrefix(USD_DOC_ATTRIBUTION + std::string("/"))
{
    resourcesDirectoryPath = astAnswererRepoPath / "resources";
    

    
    
    // Compute the local clang's system include arguments.
    // These are the arguments we need to pass so that the local clang can find the C++ stdlib.
    // Note: In the future, it would be great to find a more portable version of this,
    // but for now this is fine, most users won't be running ast-answerer.
    systemIncludeArguments.push_back("-isysroot/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk");
    systemIncludeArguments.push_back("-resource-dir=" LLVM_INSTALL_DIR"/lib/clang/19");
    
    // OpenUSD includes some headers that aren't covered by our local clang's include paths.
    // So, patch things up by copying individual headers into a standalone temporary include directory as needed
    std::vector<std::pair<std::filesystem::path, int>> pathsToCopy = {
        {"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/TargetConditionals.h", 1},
        {"/Library/Developer/CommandLineTools/usr/include/swift/bridging", 2}
    };
    std::filesystem::path customIncludeDirectory = getCustomClangIncludeDirectory();
    systemIncludeArguments.push_back("-isystem" + customIncludeDirectory.string());
    for (const std::pair<std::filesystem::path, int>& pair : pathsToCopy) {
        std::filesystem::path src = pair.first;
        int componentsToPreserve = pair.second;
        
        std::vector<std::string> srcComponents;
        for (const auto& it : src) {
            srcComponents.push_back(it.filename());
        }
        std::filesystem::path dest = customIncludeDirectory;
        for (auto i = srcComponents.size() - componentsToPreserve; i < srcComponents.size(); i++) {
            dest /= srcComponents[i];
        }
        std::filesystem::create_directories(dest.parent_path());
        if (!std::filesystem::exists(dest)) {
            std::filesystem::copy(src, dest);
        }
    }
}

// MARK: Serialization

std::filesystem::path FileSystemInfo::getOutputFileDirectory() const {
    return astAnswererBuildPath / "AstAnswererOutputs";
}

std::filesystem::path FileSystemInfo::getSerializedASTPath() const {
    return getClangDirectory() / "serializedAST";
}

std::filesystem::path FileSystemInfo::getSerializedAnalysisPath(const std::string& fileName) const {
    return getOutputFileDirectory() / "analysis" / fileName;
}

std::filesystem::path FileSystemInfo::getGeneratedCodeDirectory() const {
    return getOutputFileDirectory() / "codeGen";
}

std::filesystem::path FileSystemInfo::getClangDirectory() const {
    return getOutputFileDirectory() / "clang";
}

std::filesystem::path FileSystemInfo::getClangInputFile() const {
    return getClangDirectory() / "Input.h";
}

std::filesystem::path FileSystemInfo::getCustomClangIncludeDirectory() const {
    return getClangDirectory() / "CustomIncludes";
}


std::filesystem::path FileSystemInfo::writeSourceFileListIntoFile() const {
    // We're going to put a bunch of `#include`s of .cpp files into one header.
    //
    // That is almost never the right thing to do, because if multiple .cpp files
    // define local functions with the same name, that ends up being a violation
    // of the One Definition Rule, which is a syntax error. And, by doing this,
    // we get about 3000 ODR violations.
    //
    // But, Clang is still able to build an AST even if it has errors,
    // and this makes building the AST take one minute,
    // saving the AST take 1.5 GB, and loading the AST take one second.
    // Without this, it would take 40 minutes to build the AST,
    // 50 GB to save it, and we would have to deal with having
    // a lot of ASTUnits which would make processing the AST much more complicated.
    //
    // So, in this very niche case, putting a bunch of `#include`s of .cpp files
    // into one header actually is the thing we want to do.
    //
    // We need this because TfSingleton specializations might only be found in .cpp
    // files, but we need to see those to know if a type can safely be imported
    // as immortal. Also, in the future, it might be useful to write analysis passes
    // that look at the bodies of functions/definitions to better understand
    // and reason about the patterns that OpenUSD uses. 
    
    std::filesystem::path result = getClangInputFile();
    std::cout << "Writing source files into " << result.string() << std::endl;
    
    std::ofstream outfile(result);
    
    for (const std::filesystem::path& path : getListOfSourceFiles()) {
        std::string relativePath = path.string();
        
        // Use `+ 1` to include trim leading slash
        if (relativePath.starts_with(usdSourceRepoPath.string())) {
            relativePath = relativePath.substr(usdSourceRepoPath.string().size() + 1);
            
        } else {
            std::cerr << "Bad source file " << path.string() << std::endl;
            __builtin_trap();
        }
        
        outfile << "#include \"" << relativePath << "\"" << std::endl;
    }
    outfile.close();
    return result;
}

std::vector<std::filesystem::path> FileSystemInfo::getListOfSourceFiles() const {
    const CMakeParser* cmakeParser = _driver->getCMakeParser();
    return cmakeParser->getListOfSourceFiles();
}

std::vector<std::filesystem::path> FileSystemInfo::getListOfPublicHeaders() const {
    const CMakeParser* cmakeParser = _driver->getCMakeParser();
    return cmakeParser->getListOfPublicHeaders();
}
