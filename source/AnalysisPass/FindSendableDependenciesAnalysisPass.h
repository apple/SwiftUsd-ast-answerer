//
//  FindSendableDependenciesAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/13/24.
//

#ifndef FindSendableDependenciesAnalysisPass_h
#define FindSendableDependenciesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindSendableDependenciesAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Sendable analysis occurs in two sub-passes. This is the first sub-pass, that identifies "dependencies" of a given type for Sendable analysis. See also SendableAnalysisPass. 
// Dependencies can be the names and types of fields, base classes, and special-cased behavior
// (e.g. std::vector<T> depends on T, FRTs are not Sendable, TfToken is Sendable). 
class FindSendableDependenciesAnalysisPass final: public ASTAnalysisPass<FindSendableDependenciesAnalysisPass, FindSendableDependenciesAnalysisResult> {
public:
    FindSendableDependenciesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
    void handleSpecialDependencies(const clang::RecordDecl* recordDecl, FindSendableDependenciesAnalysisResult& analysisResult);
};

#endif /* FindSendableDependenciesAnalysisResult_h */
