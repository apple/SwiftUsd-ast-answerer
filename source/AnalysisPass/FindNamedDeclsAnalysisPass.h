//
//  FindNamedDeclsAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef FindNamedDeclsAnalysisPass_h
#define FindNamedDeclsAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

// The first analysis pass. This analysis pass allows future analysis passes to find tags by name, which
// makes testing and deserializing analysis passes much easier. It also makes it easier to write analysis passes and code gen passes,
// by refering to specific C++ types by their string names.
// Unfortunately, this analysis pass has to be run every time, and it takes a bit of time to do so. 
class FindNamedDeclsAnalysisPass final: public ASTAnalysisPass<FindNamedDeclsAnalysisPass, FindNamedDeclsAnalysisResult> {
public:
    using AnalysisResult = FindNamedDeclsAnalysisResult;
    
    FindNamedDeclsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
        
    std::string serializationFileName() const override;
    std::string testFileName() const override;
    void serialize() const override;
    bool deserialize() override;
    bool VisitNamedDecl(clang::NamedDecl* namedDecl) override;
    bool VisitType(clang::Type* type) override;
    
    void test() const override;
    
    const clang::TagDecl* findTagDecl(const std::string& typeName) const;
    const clang::NamedDecl* findNamedDecl(const std::string& name) const;
    const clang::Type* findType(const std::string& name) const;
    const clang::FunctionDecl* findFunctionDecl(const std::string& signature) const;
    
    bool shouldOnlyVisitDeclsFromUsd() const override;
    
private:
    const FindNamedDeclsAnalysisResult::NamedDeclMap& getNamedDeclMap() const;
    FindNamedDeclsAnalysisResult::NamedDeclMap& getNamedDeclMap();
    
    const FindNamedDeclsAnalysisResult::TypeMap& getTypeMap() const;
    FindNamedDeclsAnalysisResult::TypeMap& getTypeMap();
};

#endif /* FindNamedDeclsAnalysisPass_h */
