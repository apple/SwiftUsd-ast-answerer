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

#ifndef LinterBase_h
#define LinterBase_h

#include "Util/FileSystemInfo.h"
#include "Util/ClangToolHelper.h"
#include "Util/CMakeParser.h"
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "LinterRunner.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include <memory>
#include <unordered_set>


class DefaultLinterResult {
public:
    DefaultLinterResult() = default;
    void dumpLinterOutput(std::string msg) const;
    bool test(std::string allowDeny, std::string filePath, std::string lineNumber) const;
    bool deserialize(std::string allowDeny, std::string filePath, std::string lineNumber);
    std::string serialize() const;
    
    struct Entry {
        Entry(bool allow, std::string filePath, int lineNumber);
        Entry(const ASTHelpers* helper, bool allow, const clang::Decl* decl);
        Entry(const ASTHelpers* helper, bool allow, const clang::Stmt* stmt);

        
        bool allow;
        std::string filePath;
        int lineNumber;
        
        bool operator==(const Entry&) const;
        
        static std::optional<Entry> deserialize(std::string allowDeny, std::string filePath, std::string lineNumber);
        
        struct HashFunction {
            size_t operator()(const Entry&) const;
        };
        
        struct Comparator {
            bool operator()(const Entry&, const Entry&) const;
        };
    };
    
    void addEntry(Entry);
    
private:
    DefaultLinterResult(const DefaultLinterResult&);
    DefaultLinterResult& operator=(const DefaultLinterResult&);
    
        
    std::unordered_set<Entry, Entry::HashFunction> _entries;
};
std::ostream& operator<<(std::ostream&, DefaultLinterResult::Entry const&);



// Base class of all linters. Inherit with CRTP,
// specifying the LinterResult type if you need to customize it
template <typename _LinterResult = DefaultLinterResult>
class LinterBase: public clang::RecursiveASTVisitor<LinterBase<_LinterResult>>, public ASTHelpers {
public:
    using LinterResult = _LinterResult;
    
    // MARK: Virtual
    // These methods can be used to customize the behavior of visiting the AST.
    // Override one or more Visit methods to analyze that part of the AST
    virtual std::string serializationFileName() const = 0;
    virtual std::string testFileName() const = 0;
    virtual bool VisitCXXMethodDecl(clang::CXXMethodDecl* methodDecl) { return true; }
    virtual bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) { return true; }
    virtual bool VisitFunctionTemplateDecl(clang::FunctionTemplateDecl* functionTemplateDecl) { return true; }
    virtual bool VisitNamedDecl(clang::NamedDecl* namedDecl) { return true; }
    virtual bool VisitRecordDecl(clang::RecordDecl* recordDecl) { return true; }
    virtual bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) { return true; }
    virtual bool VisitTagDecl(clang::TagDecl* tagDecl) { return true; }
    virtual bool VisitTypedefDecl(clang::TypedefDecl* typedefDecl) { return true; }
    virtual bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) { return true; }
    virtual bool VisitTypeAliasDecl(clang::TypeAliasDecl* typeAliasDecl) { return true; }
    virtual bool VisitUsingDecl(clang::UsingDecl* usingDecl) { return true; }
    virtual bool VisitBaseUsingDecl(clang::BaseUsingDecl* baseUsingDecl) { return true; }
    virtual bool VisitNamespaceAliasDecl(clang::NamespaceAliasDecl* namespaceAliasDecl) { return true; }
    virtual bool VisitUsingDirectiveDecl(clang::UsingDirectiveDecl* usingDirectiveDecl) { return true; }
    virtual bool VisitUsingShadowDecl(clang::UsingShadowDecl* usingShadowDecl) { return true; }
    virtual bool VisitTemplateTypeParmDecl(clang::TemplateTypeParmDecl* templateTypeParmDecl) { return true; }
    virtual bool VisitEnumDecl(clang::EnumDecl* enumDecl) { return true; }
    virtual bool VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl) { return true; }
    
    virtual bool VisitTypedefType(clang::TypedefType* typedefType) { return true; }
    virtual bool VisitUsingType(clang::UsingType* usingType) { return true; }
    
    virtual bool VisitType(clang::Type* type) { return true; }
    
    virtual bool VisitCallExpr(clang::CallExpr* callExpr) { return true; }
    virtual bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr* cxxOperatorCallExpr) { return true; }
    
    virtual bool shouldVisitTemplateInstantiations() const { return true; }
    virtual bool shouldVisitImplicitCode() const { return true; }

    virtual bool shouldOnlyVisitDeclsFromUsd() const { return true; }
    
    // MARK: AST Traversal
    bool TraverseDecl(clang::Decl* decl) {
        if (!decl) { return true; }
        
        bool fromUsd = isEarliestDeclLocFromUsd(decl) || (clang::dyn_cast<clang::TypeDecl>(decl) && doesTypeContainUsdTypes(clang::dyn_cast<clang::TypeDecl>(decl)));
        bool shouldVisit = (bool) clang::dyn_cast<clang::TranslationUnitDecl>(decl) ||
                           fromUsd ||
                           !shouldOnlyVisitDeclsFromUsd();
        
        if (shouldVisit) {
            // Visit by calling the base class implementation
            return clang::RecursiveASTVisitor<LinterBase<LinterResult>>::TraverseDecl(decl);
        } else {
            // Continue traversal, but don't visit
            return true;
        }
    }
    
    // MARK: Accessors
    
    const LinterResult* getLinterResult() const {
        return _linterResult.get();
    }
    LinterResult* getLinterResult() {
        return _linterResult.get();
    }
    
    LinterRunner* getLinterRunner() const {
        return _linterRunner;
    }
    
    // MARK: Utility
    
    void dumpLinterOutput() const {
        getLinterResult()->dumpLinterOutput(serializationFileName());
    }
    
    const ASTAnalysisRunner& getASTAnalysisRunner() const override {
        return *(getLinterRunner()->getDriver()->getASTAnalysisRunner());
    }
    
    const FileSystemInfo& getFileSystemInfo() const override {
        return getASTAnalysisRunner().getFileSystemInfo();
    }
    
    // MARK: Serialization
    
    virtual void serialize() const {
        std::cout << "Serializing " << serializationFileName() << std::endl;
        const FileSystemInfo& f = getFileSystemInfo();
        std::filesystem::path filePath = f.getSerializedLinterPath(serializationFileName());

        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream stream(filePath);

        stream << _linterResult->serialize() << std::endl;
        
        stream.close();
    }
    
    virtual bool deserialize() {
        const FileSystemInfo& f = getFileSystemInfo();
        std::filesystem::path filePath = f.getSerializedLinterPath(serializationFileName());
        
        if (!std::filesystem::exists(filePath)) {
            return false;
        }
        
        std::cout << "Deserializing " << serializationFileName() << std::endl;

        std::vector<std::vector<std::string>> loadedFields = TestDataLoader::load(filePath, TestDataLoader::PxrNsReplacement::replace);
        
        for (const std::vector<std::string>& line : loadedFields) {
            if (line.size() != 3) {
                std::cerr << "Expected 3 fields in deserialization, but got " << line.size() << std::endl;
                for (const std::string& x : line) {
                    std::cerr << x << "; ";
                }
                std::cerr << std::endl;
                __builtin_trap();
            }
            
            if (!_linterResult->deserialize(line[0], line[1], line[2])) {
                std::cerr << "Could not deserialize '" << line[0] << "' '" << line[1] << "' '" << line[2] << "'" << std::endl;
                __builtin_trap();
            }
        }
        
        return true;
    }
    
protected:
    // MARK: Linting and Testing
    LinterBase(LinterRunner* linterRunner) :
        _linterRunner(linterRunner),
        _linterResult(std::make_unique<LinterResult>())
    {
    }
    
    void lint() {
        std::cout << "Linting " << serializationFileName() << std::endl;
        TraverseDecl((clang::TranslationUnitDecl*)getASTAnalysisRunner().getTranslationUnitDecl());
    }
    
    virtual void test() const {
        std::cout << "Testing " << serializationFileName() << std::endl;
        std::vector<std::vector<std::string>> expected = TestDataLoader::load(getFileSystemInfo(), testFileName(), TestDataLoader::PxrNsReplacement::dontReplace);
        
        uint64_t nFailures = 0;
        bool hadFailures = false;
        for (const std::vector<std::string>& line : expected) {
            if (line.size() != 3) {
                std::cerr << "Expected 3 fields in testing, but got " << line.size() << std::endl;
                for (const std::string& x : line) {
                    std::cerr << x << "; ";
                }
                std::cerr << std::endl;
                __builtin_trap();
            }
            
            if (!_linterResult->test(line[0], line[1], line[2])) {
                std::cout << "Linter test failure: " << line[0] << "; " << line[1] << "; " << line[2] << std::endl;
                nFailures += 1;
                hadFailures = true;
            }
        }
        
        if (hadFailures) {
            std::cerr << serializationFileName() << " had " << nFailures << " failures " << std::endl;
        } else {
            std::cout << serializationFileName() << " passed" << std::endl;
        }
    }

private:
    friend class LinterFactory;
    
    LinterRunner* _linterRunner;
    std::unique_ptr<LinterResult> _linterResult;
};

class LinterFactory {
public:
    template <typename T>
    static std::unique_ptr<T> makeLinter(LinterRunner* linterRunner) {
        std::unique_ptr<T> result = std::make_unique<T>(linterRunner);
        if (!result->deserialize()) {
            result->lint();
            result->serialize();
        }
        result->test();
        result->dumpLinterOutput();
        return result;
    }
};

#endif /* LinterBase_h */
