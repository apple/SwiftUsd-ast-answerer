//
//  FindTfNoticeSubclassesAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 1/7/25.
//

#ifndef FindTfNoticeSubclassesAnalysisPass_h
#define FindTfNoticeSubclassesAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindTfNoticeSubclassesAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindTfNoticeSubclassesAnalysisPass final: public ASTAnalysisPass<FindTfNoticeSubclassesAnalysisPass, FindTfNoticeSubclassesAnalysisResult> {
public:
    FindTfNoticeSubclassesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitTagDecl(clang::TagDecl* tagDecl) override;
    
private:
    bool _checkInheritance(const clang::TagDecl*);
    
    std::map<const clang::TagDecl*, bool> _cache;
};

#endif /* FindTfNoticeSubclassesAnalysisPass_h */
