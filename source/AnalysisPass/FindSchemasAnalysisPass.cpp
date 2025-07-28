//
//  FindSchemasAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#include "AnalysisPass/FindSchemasAnalysisPass.h"


FindSchemasAnalysisPass::FindSchemasAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindSchemasAnalysisPass, FindSchemasAnalysisResult>(astAnalysisRunner) {
    
}

std::string FindSchemasAnalysisPass::serializationFileName() const {
    return "FindSchemas.txt";
}

std::string FindSchemasAnalysisPass::testFileName() const {
    return "testFindSchemas.txt";
}

bool FindSchemasAnalysisPass::VisitCXXRecordDecl(clang::CXXRecordDecl *cxxRecordDecl) {
    const clang::TagDecl* tagDecl = findTagDecl("class " PXR_NS"::UsdSchemaBase");
    const clang::CXXRecordDecl* usdSchemaBase = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    
    if (cxxRecordDecl->isThisDeclarationADefinition()) {
        if (ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(cxxRecordDecl, usdSchemaBase)) {
            insert_or_assign(cxxRecordDecl, FindSchemasAnalysisResult());
        }
    }
    return true;
}
