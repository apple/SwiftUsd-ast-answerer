//
//  SdfValueTypeNamesMembersAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"

SdfValueTypeNamesMembersAnalysisPass::SdfValueTypeNamesMembersAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<SdfValueTypeNamesMembersAnalysisPass, SdfValueTypeNamesMembersAnalysisResult>(astAnalysisRunner) {}

std::string SdfValueTypeNamesMembersAnalysisPass::serializationFileName() const {
    return "SdfValueTypeNamesMembers.txt";
}

std::string SdfValueTypeNamesMembersAnalysisPass::testFileName() const {
    return "testSdfValueTypeNamesMembers.txt";
}

bool SdfValueTypeNamesMembersAnalysisPass::VisitTagDecl(clang::TagDecl *tagDecl) {
    // We're looking for the fields on one specific type with a known name,
    // so we don't want to walk the AST, just pull things out that we already know.
    
    const clang::TagDecl* valueTypeNamesTypeTagDecl = findTagDecl("class " PXR_NS"::Sdf_ValueTypeNamesType");
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(valueTypeNamesTypeTagDecl);
    insert_or_assign(valueTypeNamesTypeTagDecl, SdfValueTypeNamesMembersAnalysisResult());
    
    for (const clang::FieldDecl* field : cxxRecordDecl->fields()) {
        if (field->getType().getAsString() == "SdfValueTypeName") {
            find(valueTypeNamesTypeTagDecl)->second.push_back(field->getNameAsString());
        }
    }
    
    // Return false to abort the AST descent,
    // because we're already done.
    return false;
}
