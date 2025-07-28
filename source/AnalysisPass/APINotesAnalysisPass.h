//
//  APINotesAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/25.
//

#ifndef APINotesAnalysisPass_h
#define APINotesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/APINotesAnalysisResult.h"
#include "clang/AST/RecursiveASTVisitor.h"


class APINotesAnalysisPass final: public ASTAnalysisPass<APINotesAnalysisPass, APINotesAnalysisResult> {
public:
    APINotesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitNamedDecl(clang::NamedDecl* namedDecl) override;
    
private:
    std::vector<const clang::NamedDecl*> getHardCodedOwnedTypes() const;
    std::vector<const clang::FunctionDecl*> getHardCodedReplaceConstRefFunctionsWithCopy() const;
    std::vector<const clang::FunctionDecl*> getHardCodedReplaceMutatingFunctionsWithNonmutating() const;
};

#endif /* APINotesAnalysisPass_h */
