//
//  HashableAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef HashableAnalysisPass_h
#define HashableAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/HashableAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// Analysis for the Swift `Hashable` protocol
class HashableAnalysisPass final: public ASTAnalysisPass<HashableAnalysisPass, HashableAnalysisResult> {
public:
    HashableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    void onFindPotentialCandidate(clang::QualType qualType);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    void finalize(const clang::NamedDecl* namedDecl) override;
    
    bool decideResultViaImportIfPossible(const clang::TagDecl* tagDecl);
    void decideResultViaEquatableGivenCandidateExists(const clang::TagDecl* tagDecl);
};

#endif /* HashableAnalysisPass_h */
