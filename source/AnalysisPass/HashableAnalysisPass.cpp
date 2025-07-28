//
//  HashableAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/HashableAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/EquatableAnalysisPass.h"
#include "Util/CMakeParser.h"
#include <fstream>



HashableAnalysisPass::HashableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<HashableAnalysisPass, HashableAnalysisResult>(astAnalysisRunner) {}

std::string HashableAnalysisPass::serializationFileName() const {
    return "Hashable.txt";
}

std::string HashableAnalysisPass::testFileName() const {
    return "testHashable.txt";
}

// Hashing is complicated!
//
// Usd uses a number of different mechanisms for exposing hashing for Usd types.
// We do our best to expose them, focusing on the most common patterns.

void HashableAnalysisPass::onFindPotentialCandidate(clang::QualType qualType) {
    qualType = ASTHelpers::removingRefConst(qualType);
    if (qualType.isNull()) {
        return;
    }
    const clang::TagDecl* tagDecl = qualType->getAsTagDecl();
    if (!doesTypeContainUsdTypes(tagDecl)) {
        return;
    }
    
    const auto& hashableIt = find(tagDecl);
    if (hashableIt == end()) {
        insert_or_assign(tagDecl, HashableAnalysisResult::unknown);
    }
    
    if (decideResultViaImportIfPossible(tagDecl)) {
        return;
    }
    
    decideResultViaEquatableGivenCandidateExists(tagDecl);
}

void HashableAnalysisPass::finalize(const clang::NamedDecl *namedDecl) {
    const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl);
    if (find(tagDecl)->second._kind != HashableAnalysisResult::unknown) {
        return;
    }
    
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) {
        insert_or_assign(tagDecl, HashableAnalysisResult::blockedByNoCandidate);
        return;
    }
    
    int nCandidates = 0;
    for (clang::CXXBaseSpecifier cxxBaseSpecifier : cxxRecordDecl->bases()) {
        if (const clang::TagDecl* baseTagDecl = cxxBaseSpecifier.getType()->getAsTagDecl()) {
            finalize(baseTagDecl);
            
            if (find(baseTagDecl)->second._kind == HashableAnalysisResult::available) {
                nCandidates += 1;
            }
        }
    }
    
    if (nCandidates == 1) {
        decideResultViaEquatableGivenCandidateExists(tagDecl);
    } else {
        insert_or_assign(tagDecl, HashableAnalysisResult::blockedByNoCandidate);
    }
}

bool HashableAnalysisPass::VisitFunctionDecl(clang::FunctionDecl* functionDecl) {
    if (isEarliestDeclLocFromUsd(functionDecl) && !areAllUsdDeclsFromPublicHeaders(functionDecl)) {
#warning suspect use of declLocFromUsd
        return true;
    }
    
    if (functionDecl->getNameAsString() == "TfHashAppend") {
        if (functionDecl->parameters().size() != 2) {
            return true;
        }
        clang::QualType qualType = functionDecl->parameters()[1]->getType();
        onFindPotentialCandidate(qualType);
        
        return true;
        
    } else if (functionDecl->getNameAsString() == "hash_value") {
        if (functionDecl->parameters().size() != 1) {
            return true;
        }
        
        clang::QualType qualType = functionDecl->parameters()[0]->getType();
        if (qualType.getAsString() == "const half") {
            qualType = functionDecl->getASTContext().getLValueReferenceType(qualType);
        }
        onFindPotentialCandidate(qualType);
        return true;
        
    } else if (functionDecl->getOverloadedOperator() == clang::OO_Call) {
        if (functionDecl->parameters().size() != 1) {
            return true;
        }
        const clang::CXXMethodDecl* cxxMethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(functionDecl);
        if (!cxxMethodDecl) {
            return true;
        }
        if (cxxMethodDecl->getParent() != findTagDecl("class " PXR_NS"::TfHash")) {
            return true;
        }
        clang::QualType qualType = functionDecl->parameters()[0]->getType();
        onFindPotentialCandidate(qualType);
        return true;
        
    } else {
        return true;
    }
}

bool HashableAnalysisPass::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) {
    if (!cxxRecordDecl->isThisDeclarationADefinition()) {
        return true;
    }
    if (!isEarliestDeclLocFromUsd(cxxRecordDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(cxxRecordDecl)) {
        return true;
    }
    
    if (find(cxxRecordDecl) == end()) {
        insert_or_assign(cxxRecordDecl, HashableAnalysisResult::unknown);
    }
    
    if (decideResultViaImportIfPossible(cxxRecordDecl)) {
        return true;
    }
    
    return true;
}

bool HashableAnalysisPass::decideResultViaImportIfPossible(const clang::TagDecl* tagDecl) {
    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    const auto& it = importAnalysisPass->find(tagDecl);
    if (it == importAnalysisPass->end() || !it->second.isImportedSomehow()) {
        insert_or_assign(tagDecl, HashableAnalysisResult::blockedByImport);
        return true;
    }
    if (it->second.isImportedAsNonCopyable()) {
        insert_or_assign(tagDecl, HashableAnalysisResult::noAnalysisBecauseNonCopyable);
        return true;
    }
    
    if (it->second.isImportedAsAnyReference()) {
        // Swift reference types are hashable
        insert_or_assign(tagDecl, HashableAnalysisResult::available);
        return true;
    }
    
    return false;
}

void HashableAnalysisPass::decideResultViaEquatableGivenCandidateExists(const clang::TagDecl* tagDecl) {
    std::function<void(const clang::TagDecl*)> handleTagDecl = [this](const clang::TagDecl* x){
        const EquatableAnalysisPass* equatableAnalysisPass = getASTAnalysisRunner().getEquatableAnalysisPass();
        const auto& equatableIt = equatableAnalysisPass->find(x);
        
        if (equatableIt != equatableAnalysisPass->end() && equatableIt->second.isAvailable()) {
            insert_or_assign(x, HashableAnalysisResult::available);
        } else {
            insert_or_assign(x, HashableAnalysisResult::foundCandidateButBlockedByEquatable);
        }
    };
    
    if (const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(tagDecl)) {
        const clang::ClassTemplateDecl* classTemplateDecl = classTemplateSpecializationDecl->getSpecializedTemplate();
        for (const clang::ClassTemplateSpecializationDecl* specialization : classTemplateDecl->specializations()) {
            handleTagDecl(specialization);
        }
    } else {
        handleTagDecl(tagDecl);
    }
}
