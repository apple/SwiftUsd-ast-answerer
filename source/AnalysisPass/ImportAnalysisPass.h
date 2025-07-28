//
//  ImportAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef ImportAnalysisPass_h
#define ImportAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/ImportAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// Analysis that tries to determine whether or not a C++ type will be imported into Swift,
// how it will be imported (as a value type, as a non-copyable type, or a reference type),
// or why it is not imported if it is not imported.
// Useful for code generation and various analysis passes
class ImportAnalysisPass final: public ASTAnalysisPass<ImportAnalysisPass, ImportAnalysisResult> {
public:
    using AnalysisResult = ImportAnalysisResult;
    
    ImportAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
    void onFindPotentialCandidate(const clang::TagDecl* tagDecl);
    
    // `checkFoo()` methods return true if they've found an answer,
    // and false if they haven't
    
    bool checkSpecialCaseHandling(const clang::TagDecl* tagDecl);
    bool checkPublicHeader(const clang::TagDecl* tagDecl);
    bool checkClassTemplateDecl(const clang::TagDecl* tagDecl);
    bool checkTemplate(const clang::TagDecl* tagDecl);
    bool checkParentScope(const clang::TagDecl* tagDecl);
    bool checkAccessSpecifier(const clang::TagDecl* tagDecl);
    bool checkSharedReferenceType(const clang::TagDecl* tagDecl);
    bool checkValueType(const clang::TagDecl* tagDecl);
    bool checkNonCopyableType(const clang::TagDecl* tagDecl);
    bool checkImmortalReferenceType(const clang::TagDecl* tagDecl);
    bool checkMissingMoveCtor(const clang::TagDecl* tagDecl);
    bool checkMissingDtor(const clang::TagDecl* tagDecl);
};

#endif /* ImportAnalysisResult_h */
