//
//  FindStaticTokensAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 3/21/25.
//

#ifndef FindStaticTokensAnalysisPass_h
#define FindStaticTokensAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindStaticTokensAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds tokens types like `UsdShadeTokensType` and `KindTokens_StaticTokenType`. Used to import TfStaticData<T>
// in an ergonomically friendly way for Swift.
class FindStaticTokensAnalysisPass final: public ASTAnalysisPass<FindStaticTokensAnalysisPass, FindStaticTokensAnalysisResult> {
public:
    FindStaticTokensAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitRecordDecl(clang::RecordDecl*) override;
};

#endif /* FindStaticTokensAnalysisPass_h */
