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

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "Driver/Driver.h"
#include "Util/TestDataLoader.h"
#include "Util/FileWriterHelper.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Lookup.h"
#include <regex>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "AnalysisPass/FindNamedDeclsAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/PublicInheritanceAnalysisPass.h"
#include "AnalysisPass/EquatableAnalysisPass.h"
#include "AnalysisPass/HashableAnalysisPass.h"
#include "AnalysisPass/ComparableAnalysisPass.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"
#include "AnalysisPass/FindStaticTokensAnalysisPass.h"
#include "AnalysisPass/FindTfNoticeSubclassesAnalysisPass.h"
#include "AnalysisPass/CustomStringConvertibleAnalysisPass.h"
#include "AnalysisPass/TypedefAnalysisPass.h"
#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include "AnalysisPass/FindSchemasAnalysisPass.h"
#include "AnalysisPass/FindSendableDependenciesAnalysisPass.h"
#include "AnalysisPass/SendableAnalysisPass.h"
#include "AnalysisPass/APINotesAnalysisPass.h"


// MARK: Entry
ASTAnalysisRunner::~ASTAnalysisRunner() {}

ASTAnalysisRunner::ASTAnalysisRunner(const Driver* driver) :
    _driver(driver)
{
    if (_driver->getClangToolHelper()->getASTUnits().size() != 1) {
        std::cerr << "Error! Expected 1 AST unit, but got " << _driver->getClangToolHelper()->getASTUnits().size() << std::endl;
        __builtin_trap();
    }
    _astUnit = _driver->getClangToolHelper()->getASTUnits().front().get();
    _astContext = &_astUnit->getASTContext();
    _sourceManager = &_astUnit->getSourceManager();
    _translationUnitDecl = _astContext->getTranslationUnitDecl();
    
    // Now that we've set up our clang fields,
    // start doing analysis passes. The order we do these in
    // is important, because later passes may assume earlier passes
    // have already completed
    _findNamedDeclsAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindNamedDeclsAnalysisPass>(this);
    _importAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<ImportAnalysisPass>(this);
    _publicInheritanceAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<PublicInheritanceAnalysisPass>(this);
    _equatableAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<EquatableAnalysisPass>(this);
    _hashableAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<HashableAnalysisPass>(this);
    _comparableAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<ComparableAnalysisPass>(this);
    _findEnumsAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindEnumsAnalysisPass>(this);
    _findStaticTokensAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindStaticTokensAnalysisPass>(this);
    _findTfNoticeSubclassesAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindTfNoticeSubclassesAnalysisPass>(this);
    _customStringConvertibleAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<CustomStringConvertibleAnalysisPass>(this);
    _typedefAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<TypedefAnalysisPass>(this);
    _sdfValueTypeNamesMembersAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<SdfValueTypeNamesMembersAnalysisPass>(this);
    _findSchemasAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindSchemasAnalysisPass>(this);
    _findSendableDependenciesAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<FindSendableDependenciesAnalysisPass>(this);
    _sendableAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<SendableAnalysisPass>(this);
    _apiNotesAnalysisPass = ASTAnalysisPassFactory::makeAnalysisPass<APINotesAnalysisPass>(this);
}

// MARK: Accessors
const Driver* ASTAnalysisRunner::getDriver() const {
    return _driver;
}
const FileSystemInfo& ASTAnalysisRunner::getFileSystemInfo() const {
    return *(_driver->getFileSystemInfo());
}

const ClangToolHelper& ASTAnalysisRunner::getClangToolHelper() const {
    return *(_driver->getClangToolHelper());
}

const clang::SourceManager* ASTAnalysisRunner::getSourceManager() const {
    return _sourceManager;
}

const clang::TranslationUnitDecl* ASTAnalysisRunner::getTranslationUnitDecl() const {
    return _translationUnitDecl;
}

const clang::TagDecl* ASTAnalysisRunner::findTagDecl(const std::string &typeName) const {
    return _findNamedDeclsAnalysisPass->findTagDecl(typeName);
}

const clang::NamedDecl* ASTAnalysisRunner::findNamedDecl(const std::string &name) const {
    return _findNamedDeclsAnalysisPass->findNamedDecl(name);
}
const clang::Type* ASTAnalysisRunner::findType(const std::string &name) const {
    return _findNamedDeclsAnalysisPass->findType(name);
}
const clang::FunctionDecl* ASTAnalysisRunner::findFunctionDecl(const std::string& signature) const {
    return _findNamedDeclsAnalysisPass->findFunctionDecl(signature);
}

const FindNamedDeclsAnalysisPass* ASTAnalysisRunner::getFindNamedDeclsAnalysisPass() const {
    return _findNamedDeclsAnalysisPass.get();
}
const ImportAnalysisPass* ASTAnalysisRunner::getImportAnalysisPass() const {
    return _importAnalysisPass.get();
}
const PublicInheritanceAnalysisPass* ASTAnalysisRunner::getPublicInheritanceAnalysisPass() const {
    return _publicInheritanceAnalysisPass.get();
}
const EquatableAnalysisPass* ASTAnalysisRunner::getEquatableAnalysisPass() const {
    return _equatableAnalysisPass.get();
}
const HashableAnalysisPass* ASTAnalysisRunner::getHashableAnalysisPass() const {
    return _hashableAnalysisPass.get();
}
const ComparableAnalysisPass* ASTAnalysisRunner::getComparableAnalysisPass() const {
    return _comparableAnalysisPass.get();
}
const CustomStringConvertibleAnalysisPass* ASTAnalysisRunner::getCustomStringConvertibleAnalysisPass() const {
    return _customStringConvertibleAnalysisPass.get();
}
const TypedefAnalysisPass* ASTAnalysisRunner::getTypedefAnalysisPass() const {
    return _typedefAnalysisPass.get();
}
const SdfValueTypeNamesMembersAnalysisPass* ASTAnalysisRunner::getSdfValueTypeNamesMembersAnalysisPass() const {
    return _sdfValueTypeNamesMembersAnalysisPass.get();
}
const FindSchemasAnalysisPass* ASTAnalysisRunner::getFindSchemasAnalysisPass() const {
    return _findSchemasAnalysisPass.get();
}
const FindEnumsAnalysisPass* ASTAnalysisRunner::getFindEnumsAnalysisPass() const {
    return _findEnumsAnalysisPass.get();
}
const FindStaticTokensAnalysisPass* ASTAnalysisRunner::getFindStaticTokensAnalysisPass() const {
    return _findStaticTokensAnalysisPass.get();
}
const FindTfNoticeSubclassesAnalysisPass* ASTAnalysisRunner::getFindTfNoticeSubclassesAnalysisPass() const {
    return _findTfNoticeSubclassesAnalysisPass.get();
}
const FindSendableDependenciesAnalysisPass* ASTAnalysisRunner::getFindSendableDependenciesAnalysisPass() const {
    return _findSendableDependenciesAnalysisPass.get();
}
const SendableAnalysisPass* ASTAnalysisRunner::getSendableAnalysisPass() const {
    return _sendableAnalysisPass.get();
}
const APINotesAnalysisPass* ASTAnalysisRunner::getAPINotesAnalysisPass() const {
    return _apiNotesAnalysisPass.get();
}
