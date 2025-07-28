//
//  FindEnumsAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#ifndef FindEnumsAnalysisPass_h
#define FindEnumsAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindEnumsAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds C++ enums and their cases. Used to power enum wrapping code gen
// to work around a limitation in Swift-Cxx interop where nested unscoped enums aren't imported correctly. 
class FindEnumsAnalysisPass final: public ASTAnalysisPass<FindEnumsAnalysisPass, FindEnumsAnalysisResult> {
public:
    FindEnumsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl) override;
};

#endif /* FindEnumsAnalysisPass_h */
