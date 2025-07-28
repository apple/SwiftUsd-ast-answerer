//
//  FindSchemasAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef FindSchemasAnalysisPass_h
#define FindSchemasAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindSchemasAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds all subclasses of UsdSchemaBase, to power ergonomic improvements and workarounds
// for Swift-Cxx interop limitations
class FindSchemasAnalysisPass final: public ASTAnalysisPass<FindSchemasAnalysisPass, FindSchemasAnalysisResult> {
public:
    FindSchemasAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
};

#endif /* FindSchemasAnalysisPass_h */
