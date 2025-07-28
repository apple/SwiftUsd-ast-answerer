//
//  TypedefAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/24.
//

#include "AnalysisPass/TypedefAnalysisPass.h"
#include "Util/CMakeParser.h"

TypedefAnalysisPass::TypedefAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<TypedefAnalysisPass, TypedefAnalysisResult>(astAnalysisRunner) {}

std::string TypedefAnalysisPass::serializationFileName() const {
    return "Typedef.txt";
}
std::string TypedefAnalysisPass::testFileName() const {
    return "testTypedef.txt";
}

bool TypedefAnalysisPass::isTypedefInAllowableDeclContext(const clang::TypedefNameDecl* typedefNameDecl) const {
    const clang::DeclContext* declContext = typedefNameDecl->getLexicalDeclContext();
    if (!declContext) {
        return false;
    }
#define DISALLOW(TYPE) \
if (clang::dyn_cast<clang::TYPE>(declContext)) {\
    return false;\
}
    
    DISALLOW(HLSLBufferDecl)
    DISALLOW(LabelDecl)
    DISALLOW(ObjCCompatibleAliasDecl)
    DISALLOW(ObjCContainerDecl)
    DISALLOW(ObjCMethodDecl)
    DISALLOW(ObjCPropertyDecl)
    DISALLOW(TemplateDecl)
    DISALLOW(ClassTemplateSpecializationDecl)
    DISALLOW(UnresolvedUsingIfExistsDecl)
    DISALLOW(UsingPackDecl)
    DISALLOW(UsingShadowDecl)
    DISALLOW(ValueDecl)
    
    if (!clang::dyn_cast<clang::NamedDecl>(declContext)) {
        return false;
    }
    
    return true;
    
#undef DISALLOW
}

bool TypedefAnalysisPass::VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) {
    if (!isEarliestDeclLocFromUsd(typedefNameDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(typedefNameDecl)) {
        return true;
    }
    if (ASTHelpers::isNotVisibleToSwift(typedefNameDecl->getAccess())) {
        return true;
    }
    
    std::string sugaredName = typedefNameDecl->getNameAsString();
    if (sugaredName.size() == 0) {
        return true;
    }
    
    clang::QualType unsugaredType = typedefNameDecl->getUnderlyingType().getCanonicalType();
    const clang::TagDecl* unsugaredTagDecl = unsugaredType->getAsTagDecl();
    if (!unsugaredTagDecl) {
        return true;
    }
    if (!doesTypeContainUsdTypes(unsugaredTagDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(unsugaredTagDecl)) {
        return true;
    }
    if (!isTypedefInAllowableDeclContext(typedefNameDecl)) {
        return true;
    }
    
    const clang::NamedDecl* enclosingNamedDecl = clang::dyn_cast<clang::NamedDecl>(typedefNameDecl->getLexicalDeclContext());
    
    auto it = find(unsugaredTagDecl);
    if (it == end()) {
        insert_or_assign(unsugaredTagDecl, TypedefAnalysisResult());
        it = find(unsugaredTagDecl);
    }
    it->second.insert(enclosingNamedDecl, sugaredName);
    return true;
}

bool TypedefAnalysisPass::comparesEqualWhileTesting(const TypedefAnalysisResult& expected, const TypedefAnalysisResult& actual) const {
    return expected.isSubset(actual);
}


