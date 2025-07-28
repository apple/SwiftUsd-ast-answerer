//
//  ImportAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#include "AnalysisResult/ImportAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

bool ImportAnalysisResult::isImportedSomehow() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return true;
        case importedAsNonCopyable: return true;
        case importedAsSharedReference: return true;
        case importedAsImmortalReference: return true;
    }
}

bool ImportAnalysisResult::isImportedAsValue() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return true;
        case importedAsNonCopyable: return false;
        case importedAsSharedReference: return false;
        case importedAsImmortalReference: return false;
    }
}

bool ImportAnalysisResult::isImportedAsNonCopyable() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return false;
        case importedAsNonCopyable: return true;
        case importedAsSharedReference: return false;
        case importedAsImmortalReference: return false;
    }
}

bool ImportAnalysisResult::isImportedAsAnyReference() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return false;
        case importedAsNonCopyable: return false;
        case importedAsSharedReference: return true;
        case importedAsImmortalReference: return true;
    }
}

bool ImportAnalysisResult::isImportedAsSharedReference() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return false;
        case importedAsNonCopyable: return false;
        case importedAsSharedReference: return true;
        case importedAsImmortalReference: return false;
    }
}

bool ImportAnalysisResult::isImportedAsImmortalReference() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByNonPublicHeaderDefinition: return false;
        case blockedByTemplatedRecordDecl: return false;
        case blockedByTemplateInstantiationArgs: return false;
        case blockedByParent: return false;
        case blockedByAccess: return false;
        case blockedByInaccessibleMove: return false;
        case blockedByInaccessibleDtor: return false;
        case importedAsValue: return false;
        case importedAsNonCopyable: return false;
        case importedAsSharedReference: return false;
        case importedAsImmortalReference: return true;
    }
}

ImportAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: return "unknown";
        case blockedByNonPublicHeaderDefinition: return "blockedByNonPublicHeaderDefinition";
        case blockedByTemplatedRecordDecl: return "blockedByTemplatedRecordDecl";
        case blockedByTemplateInstantiationArgs: return "blockedByTemplateInstantiationArgs";
        case blockedByParent: return "blockedByParent";
        case blockedByAccess: return "blockedByAccess";
        case blockedByInaccessibleMove: return "blockedByInaccessibleMove";
        case blockedByInaccessibleDtor: return "blockedByInaccessibleDtor";
        case importedAsValue: return "importedAsValue";
        case importedAsNonCopyable: return "importedAsNonCopyable";
        case importedAsSharedReference: return "importedAsSharedReference";
        case importedAsImmortalReference: return "importedAsImmortalReference";
        default: return "__errCase";
    }
}

/* static */
std::vector<ImportAnalysisResult::Kind> ImportAnalysisResult::allCases() {
    return {
        unknown,
        blockedByNonPublicHeaderDefinition,
        blockedByTemplatedRecordDecl,
        blockedByTemplateInstantiationArgs,
        blockedByParent,
        blockedByAccess,
        blockedByInaccessibleMove,
        blockedByInaccessibleDtor,
        importedAsValue,
        importedAsNonCopyable,
        importedAsSharedReference,
        importedAsImmortalReference,
    };
}

ImportAnalysisResult::ImportAnalysisResult() : ImportAnalysisResult(unknown) {}

ImportAnalysisResult::ImportAnalysisResult(ImportAnalysisResult::Kind kind) : _kind(kind) {}

/* static */
std::optional<ImportAnalysisResult> ImportAnalysisResult::deserialize(const std::string &data, const ImportAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(ImportAnalysisResult(kind)) == data) {
            return ImportAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const ImportAnalysisResult& obj) {
    return os << std::string(obj);
}

