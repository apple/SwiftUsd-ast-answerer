//
//  SdfValueTypeNamesMembersAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef SdfValueTypeNamesMembersAnalysisPass_h
#define SdfValueTypeNamesMembersAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/SdfValueTypeNamesMembersAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds members of Sdf_ValueTypeNamesType to wrap them and improve ergonomics for Swift
class SdfValueTypeNamesMembersAnalysisPass final: public ASTAnalysisPass<SdfValueTypeNamesMembersAnalysisPass, SdfValueTypeNamesMembersAnalysisResult> {
public:
    SdfValueTypeNamesMembersAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
};

#endif /* SdfValueTypeNamesMembersAnalysisPass_h */
