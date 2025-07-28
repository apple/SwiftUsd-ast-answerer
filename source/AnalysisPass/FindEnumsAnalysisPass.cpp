//
//  FindEnumsAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"


FindEnumsAnalysisPass::FindEnumsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<FindEnumsAnalysisPass, FindEnumsAnalysisResult>(astAnalysisRunner) {}
    
std::string FindEnumsAnalysisPass::serializationFileName() const {
    return "FindEnums.txt";
}
std::string FindEnumsAnalysisPass::testFileName() const {
    return "testFindEnums.txt";
}

bool FindEnumsAnalysisPass::VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl) {
    if (!isEarliestDeclLocFromUsd(enumConstantDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(enumConstantDecl)) {
        return true;
    }
    
    const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(enumConstantDecl->getDeclContext());
            
    if (find(enumDecl) == end()) {
        insert_or_assign(enumDecl, FindEnumsAnalysisResult(enumDecl->isScoped()));
    }
    
    std::string name = enumConstantDecl->getNameAsString();
    llvm::APSInt apsInt = enumConstantDecl->getInitVal();
    int64_t value = apsInt.getExtValue();
    
    find(enumDecl)->second.addCase(name, value);
    
    return true;
}
