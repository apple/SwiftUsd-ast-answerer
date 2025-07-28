//
//  FileSystemInfo.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/3/24.
//

#ifndef FileSystemInfo_h
#define FileSystemInfo_h

#include <filesystem>
#include <vector>
#include <string>

#include "Driver/Driver.h"

// FileSystemInfo grabs CMake -D/#define'd variables and creates the output directory. 
// All hard-coded file system information/manipulation should happen here.
struct FileSystemInfo {
private:
    const Driver* _driver;
    
public:
    FileSystemInfo(const Driver* driver);

    // MARK: Fields
    std::filesystem::path usdSourceRepoPath;
    std::filesystem::path usdInstalledHeaderPath;
    std::filesystem::path astAnswererRepoPath;
    std::filesystem::path astAnswererBuildPath;
    
    std::filesystem::path resourcesDirectoryPath;
    
    std::string usdDocumentationAttributionLinkPrefix;
    
    std::vector<std::string> systemIncludeArguments;
    
    // MARK: Queries
    std::vector<std::filesystem::path> getListOfPublicHeaders() const;

    // MARK: Serialization
    std::filesystem::path getOutputFileDirectory() const;
    std::filesystem::path getSerializedASTPath() const;
    std::filesystem::path getSerializedAnalysisPath(const std::string& fileName) const;
    std::filesystem::path getGeneratedCodeDirectory() const;
    
private:
    std::filesystem::path getClangDirectory() const;
    std::filesystem::path getClangInputFile() const;
    std::filesystem::path getCustomClangIncludeDirectory() const;
        
private:
    // MARK: Operations
    std::filesystem::path writeSourceFileListIntoFile() const;
    std::vector<std::filesystem::path> getListOfSourceFiles() const;
    
    friend class ClangToolHelper;
};

#endif /* FileSystemInfo_h */
