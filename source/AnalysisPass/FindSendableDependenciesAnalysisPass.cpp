//
//  FindSendableDependenciesAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/13/24.
//

#include "AnalysisPass/FindSendableDependenciesAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"


FindSendableDependenciesAnalysisPass::FindSendableDependenciesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindSendableDependenciesAnalysisPass, FindSendableDependenciesAnalysisResult>(astAnalysisRunner) {}

std::string FindSendableDependenciesAnalysisPass::serializationFileName() const {
    return "FindSendableDependencies.txt";
}

std::string FindSendableDependenciesAnalysisPass::testFileName() const {
    return "testFindSendableDependencies.txt";
}

bool FindSendableDependenciesAnalysisPass::shouldOnlyVisitDeclsFromUsd() const {
    return false;
}

bool FindSendableDependenciesAnalysisPass::VisitTagDecl(clang::TagDecl *tagDecl) {
    if (!tagDecl->isThisDeclarationADefinition()) {
        return true;
    }
    if (tagDecl->getDescribedTemplate()) {
        return true;
    }
    if (tagDecl->isTemplated()) {
        return true;
    }
    
    const clang::RecordDecl* recordDecl = clang::dyn_cast<clang::RecordDecl>(tagDecl);
    if (!recordDecl) {
        insert_or_assign(tagDecl, FindSendableDependenciesAnalysisResult());
        return true;
    }
    FindSendableDependenciesAnalysisResult analysisResult;
    if (const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl)) {
        for (clang::CXXBaseSpecifier base : cxxRecordDecl->bases()) {
            analysisResult.addInheritanceDependency(base);
        }
    }
    
    for (const clang::FieldDecl* fieldDecl : recordDecl->fields()) {
        analysisResult.addFieldDependency(fieldDecl);
    }
    
    handleSpecialDependencies(recordDecl, analysisResult);
    
    insert_or_assign(tagDecl, analysisResult);
    
    return true;
}

void FindSendableDependenciesAnalysisPass::handleSpecialDependencies(const clang::RecordDecl *recordDecl, FindSendableDependenciesAnalysisResult &analysisResult) {
    std::vector<std::string> specialAvailables = {
        "std::string", "class " PXR_NS"::TfToken", "class " PXR_NS"::GfFrustum"
    };
    std::vector<std::pair<std::string, int>> specialConditionals = {
        {"class std::list<", 1},
        {"class std::set<", 1},
        {"class std::unordered_set<", 1},
        {"class std::vector<", 1},
        {"class std::map<", 2},
        {"class std::multimap<", 2},
        {"class std::unordered_map<", 2},
        {"class std::unordered_multimap<", 2},
        {"class " PXR_NS"::VtArray<", 1},
        {"class " PXR_NS"::TfStaticData<", 1},
    };

    
    
    for (const auto& x : specialAvailables) {
        if (ASTHelpers::getAsString(recordDecl) == x) {
            analysisResult.dependencies = {FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::specialAvailable, "", nullptr)};
            return;
        }
    }
    
    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    auto it = importAnalysisPass->getData().find(recordDecl);
    if (it != importAnalysisPass->getData().end() && it->second.isImportedAsAnyReference()) {
        analysisResult.dependencies.push_back(FindSendableDependenciesAnalysisResult::Dependency(FindSendableDependenciesAnalysisResult::specialImportedAsReference, "", nullptr));
        return;
    }
    
    if (const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl)) {
        const clang::TemplateArgumentList& templateArgumentList = classTemplateSpecializationDecl->getTemplateInstantiationArgs();
                
        for (const auto& p : specialConditionals) {
            if (ASTHelpers::getAsString(classTemplateSpecializationDecl).starts_with(p.first)) {
                bool allConditionalArgumentsAreTypes = true;
                for (int i = 0; i < p.second; i++) {
                    if (templateArgumentList[i].getKind() != clang::TemplateArgument::Type) {
                        allConditionalArgumentsAreTypes = false;
                        break;
                    }
                }
                if (!allConditionalArgumentsAreTypes) {
                    continue;
                }
                
                analysisResult.dependencies = {};
                for (int i = 0; i < p.second; i++) {
                    FindSendableDependenciesAnalysisResult::Dependency dependency(FindSendableDependenciesAnalysisResult::specialConditional, "", templateArgumentList[i].getAsType().getTypePtr());
                    analysisResult.dependencies.push_back(dependency);
                }
                return;
            }
        }
    }
}
