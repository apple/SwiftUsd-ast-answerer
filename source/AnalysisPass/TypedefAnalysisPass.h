//
//  TypedefAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/24.
//

#ifndef TypedefAnalysisPass_h
#define TypedefAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/TypedefAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

// Analysis pass that finds a subset of typedefs of C++ tags.
// Used by code gen, because Swift-Cxx interop does not currently support specializing
// template classes when used as a type. 
class TypedefAnalysisPass final: public ASTAnalysisPass<TypedefAnalysisPass, TypedefAnalysisResult> {
public:
    TypedefAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    
    bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) override;
    bool isTypedefInAllowableDeclContext(const clang::TypedefNameDecl* typedefNameDecl) const;
    
    bool comparesEqualWhileTesting(const TypedefAnalysisResult& expected, const TypedefAnalysisResult& actual) const override;
};

#endif /* TypedefAnalysisPass_h */
