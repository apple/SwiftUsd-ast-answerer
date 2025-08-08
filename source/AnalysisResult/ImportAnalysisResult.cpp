//===----------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer project authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

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

