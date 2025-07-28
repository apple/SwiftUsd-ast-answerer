//
//  ClangToolHelper.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/9/24.
//

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
