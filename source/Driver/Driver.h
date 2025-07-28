//
//  Driver.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/9/24.
//

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
