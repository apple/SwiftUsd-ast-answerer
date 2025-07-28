//
//  CustomStringConvertibleAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/30/24.
//

#ifndef CustomStringConvertibleAnalysisPass_h
#define CustomStringConvertibleAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/CustomStringConvertibleAnalysisResult.h"

// A type T is CustomStringConvertible iff there is a function `std::ostream& operator<<(std::ostream& os, const U& obj)` where T is convertible to U.
// CustomStringConvertible is inheritable

class CustomStringConvertibleAnalysisPass final: public ASTAnalysisPass<CustomStringConvertibleAnalysisPass, CustomStringConvertibleAnalysisResult> {
public:
    CustomStringConvertibleAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    bool VisitEnumDecl(clang::EnumDecl* enumDecl) override;
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override;
    bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) override;
    void finalize(const clang::NamedDecl* namedDecl) override;
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
private:
    std::set<clang::QualType> _lessThanLessThanFunctionTypes;
};

#endif /* CustomStringConvertibleAnalysisPass_h */
