// ===-------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer authors. All Rights Reserved. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at: 
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.     
// 
// SPDX-License-Identifier: Apache-2.0
// ===-------------------------------------------------------------------===//

#ifndef ASTAnalysisRunner_h
#define ASTAnalysisRunner_h

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "Driver/Driver.h"
#include "Util/ClangToolHelper.h"

#include <memory>
#include <set>

class FindNamedDeclsAnalysisPass;
class ImportAnalysisPass;
class PublicInheritanceAnalysisPass;
class EquatableAnalysisPass;
class HashableAnalysisPass;
class ComparableAnalysisPass;
class FindEnumsAnalysisPass;
class FindStaticTokensAnalysisPass;
class FindTfNoticeSubclassesAnalysisPass;
class CustomStringConvertibleAnalysisPass;
class TypedefAnalysisPass;
class SdfValueTypeNamesMembersAnalysisPass;
class FindSchemasAnalysisPass;
class FindSendableDependenciesAnalysisPass;
class SendableAnalysisPass;
class APINotesAnalysisPass;

// Owns and coordinates running different AST analysis passes
class ASTAnalysisRunner {
public:
    // MARK: Entry
    ASTAnalysisRunner(const Driver* driver);
    ~ASTAnalysisRunner();
        
public:
    // MARK: Accessors
    const Driver* getDriver() const;
    const FileSystemInfo& getFileSystemInfo() const;
    const ClangToolHelper& getClangToolHelper() const;
    const clang::SourceManager* getSourceManager() const;
    const clang::TranslationUnitDecl* getTranslationUnitDecl() const;
    
    const clang::TagDecl* findTagDecl(const std::string& typeName) const;
    const clang::NamedDecl* findNamedDecl(const std::string& name) const;
    const clang::Type* findType(const std::string& name) const;
    const clang::FunctionDecl* findFunctionDecl(const std::string& signature) const;
    
    const FindNamedDeclsAnalysisPass* getFindNamedDeclsAnalysisPass() const;
    const ImportAnalysisPass* getImportAnalysisPass() const;
    const PublicInheritanceAnalysisPass* getPublicInheritanceAnalysisPass() const;
    const EquatableAnalysisPass* getEquatableAnalysisPass() const;
    const HashableAnalysisPass* getHashableAnalysisPass() const;
    const ComparableAnalysisPass* getComparableAnalysisPass() const;
    const FindEnumsAnalysisPass* getFindEnumsAnalysisPass() const;
    const FindStaticTokensAnalysisPass* getFindStaticTokensAnalysisPass() const;
    const FindTfNoticeSubclassesAnalysisPass* getFindTfNoticeSubclassesAnalysisPass() const;
    const CustomStringConvertibleAnalysisPass* getCustomStringConvertibleAnalysisPass() const;
    const TypedefAnalysisPass* getTypedefAnalysisPass() const;
    const SdfValueTypeNamesMembersAnalysisPass* getSdfValueTypeNamesMembersAnalysisPass() const;
    const FindSchemasAnalysisPass* getFindSchemasAnalysisPass() const;
    const FindSendableDependenciesAnalysisPass* getFindSendableDependenciesAnalysisPass() const;
    const SendableAnalysisPass* getSendableAnalysisPass() const;
    const APINotesAnalysisPass* getAPINotesAnalysisPass() const;

private:
    // MARK: Fields
    const Driver* _driver;
    
    const clang::ASTUnit* _astUnit;
    const clang::ASTContext* _astContext;
    const clang::SourceManager* _sourceManager;
    const clang::TranslationUnitDecl* _translationUnitDecl;
    
    std::unique_ptr<FindNamedDeclsAnalysisPass> _findNamedDeclsAnalysisPass;
    std::unique_ptr<ImportAnalysisPass> _importAnalysisPass;
    std::unique_ptr<PublicInheritanceAnalysisPass> _publicInheritanceAnalysisPass;
    std::unique_ptr<EquatableAnalysisPass> _equatableAnalysisPass;
    std::unique_ptr<HashableAnalysisPass> _hashableAnalysisPass;
    std::unique_ptr<ComparableAnalysisPass> _comparableAnalysisPass;
    std::unique_ptr<FindEnumsAnalysisPass> _findEnumsAnalysisPass;
    std::unique_ptr<FindStaticTokensAnalysisPass> _findStaticTokensAnalysisPass;
    std::unique_ptr<FindTfNoticeSubclassesAnalysisPass> _findTfNoticeSubclassesAnalysisPass;
    std::unique_ptr<CustomStringConvertibleAnalysisPass> _customStringConvertibleAnalysisPass;
    std::unique_ptr<TypedefAnalysisPass> _typedefAnalysisPass;
    std::unique_ptr<SdfValueTypeNamesMembersAnalysisPass> _sdfValueTypeNamesMembersAnalysisPass;
    std::unique_ptr<FindSchemasAnalysisPass> _findSchemasAnalysisPass;
    std::unique_ptr<FindSendableDependenciesAnalysisPass> _findSendableDependenciesAnalysisPass;
    std::unique_ptr<SendableAnalysisPass> _sendableAnalysisPass;
    std::unique_ptr<APINotesAnalysisPass> _apiNotesAnalysisPass;
};


#endif /* ASTAnalysisRunner_h */
