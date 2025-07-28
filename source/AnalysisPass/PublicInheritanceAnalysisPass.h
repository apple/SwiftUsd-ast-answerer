//
//  PublicInheritanceAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 11/8/24.
//

#ifndef PublicInheritanceAnalysisPass_h
#define PublicInheritanceAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/PublicInheritanceAnalysisResult.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <unordered_map>
#include <unordered_set>

class PublicInheritanceAnalysisPass final: public ASTAnalysisPass<PublicInheritanceAnalysisPass, PublicInheritanceAnalysisResult> {
public:
    PublicInheritanceAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    void analysisPassIsFinished() override;
    
    // Returns the public subtypes of base, including base and indirect/multiple levels of inheritance.
    // It is an error to call this before analysisPassIsFinished() is called
    std::unordered_set<const clang::CXXRecordDecl*> getPublicSubtypes(const clang::CXXRecordDecl* base) const;
    
private:
    bool _analysisPassIsFinished = false;
    mutable std::unordered_map<const clang::CXXRecordDecl*, std::unordered_set<const clang::CXXRecordDecl*>> _publicSubtypes;
};

#endif /* PublicInheritanceAnalysisPass_h */
