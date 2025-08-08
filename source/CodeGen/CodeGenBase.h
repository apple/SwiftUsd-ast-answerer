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

#ifndef CodeGenBase_h
#define CodeGenBase_h

#include "Util/ClangToolHelper.h"
#include "CodeGen/CodeGenRunner.h"
#include "Util/FileWriterHelper.h"
#include "Util/CMakeParser.h"

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/FindNamedDeclsAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/EquatableAnalysisPass.h"
#include "AnalysisPass/HashableAnalysisPass.h"
#include "AnalysisPass/ComparableAnalysisPass.h"
#include "AnalysisPass/CustomStringConvertibleAnalysisPass.h"
#include "AnalysisPass/TypedefAnalysisPass.h"
#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"
#include "AnalysisPass/FindStaticTokensAnalysisPass.h"
#include "AnalysisPass/SendableAnalysisPass.h"
#include "AnalysisPass/APINotesAnalysisPass.h"

#include <algorithm>
#include <variant>

// Formats type names for a particular language. Automatically writes platform guards to open files
class TypeNamePrinter {
public:
    struct Type {
        Type(const clang::NamedDecl* namedDecl) : _impl(namedDecl) {
            if (namedDecl == nullptr) {
                std::cout << "Cannot form TypeNamePrinter:Type from nullptr" << std::endl;
                __builtin_trap();
            }
        }
        bool isNamedDecl() const {
            return std::holds_alternative<const clang::NamedDecl*>(_impl);
        }
        const clang::NamedDecl* getNamedDeclOpt() {
            return isNamedDecl() ? getNamedDecl() : nullptr;
        }
        const clang::NamedDecl* getNamedDecl() const {
            return std::get<const clang::NamedDecl*>(_impl);
        }
        
        Type(clang::QualType qualType) : _impl(qualType) {}
        bool isQualType() const {
            return std::holds_alternative<clang::QualType>(_impl);
        }
        clang::QualType getQualTypeOpt() const {
            return isQualType() ? getQualType() : clang::QualType();
        }
        clang::QualType getQualType() const {
            return std::get<clang::QualType>(_impl);
        }
        
        bool operator<(const Type& other) const{
            return _impl < other._impl;
        }
        
    private:
        std::variant<const clang::NamedDecl*, clang::QualType> _impl;
    };
    
    ~TypeNamePrinter() {
        if (_usedTypeName) {
            _onDtorIfUsedTypeName();
        }
    }
    
    Type getType() { return _type; }
    
    TypeNamePrinter(TypeNamePrinter&& other) :
    _type(other._type),
    _onDtorIfUsedTypeName(other._onDtorIfUsedTypeName),
    _usedTypeName(other._usedTypeName)
    {
        other._onDtorIfUsedTypeName = {};
        other._usedTypeName = false;
    }
    TypeNamePrinter& operator=(TypeNamePrinter&&) = delete;
    
private:
    static std::set<std::string> includePathsForSwiftNameInCpp(const Driver *driver, TypeNamePrinter::Type type);
    
    TypeNamePrinter(TypeNamePrinter::Type type, std::function<void()> onDtorIfUsedTypeName)
    : _type(type), _onDtorIfUsedTypeName(onDtorIfUsedTypeName) {}
    
    static std::optional<std::string> getFeatureFlagGuard(const Driver*, TypeNamePrinter::Type type, std::string fileNameSuffix);
    static std::optional<std::string> getFeatureFlagGuard(std::string includedHeader);
    
    TypeNamePrinter(const TypeNamePrinter&) = delete;
    TypeNamePrinter& operator=(const TypeNamePrinter&) = delete;
    
    TypeNamePrinter::Type _type;
    std::function<void()> _onDtorIfUsedTypeName;
    bool _usedTypeName = false;
    
    template <typename Derived> friend class CodeGenBase;
    friend class TypeNamePrinterImpl;
};

// Languages for TypeNamePrinter
struct SwiftNameInSwift {
private:
    static std::optional<std::string> getTypeNameOpt(const Driver*, TypeNamePrinter::Type type);
    template <typename Derived> friend class CodeGenBase;
};
struct SwiftNameInCpp {
private:
    static std::optional<std::string> getTypeNameOpt(const Driver*, TypeNamePrinter::Type type);
    template <typename Derived> friend class CodeGenBase;
};
struct CppNameInCpp {
private:
    static std::optional<std::string> getTypeNameOpt(const Driver*, TypeNamePrinter::Type type);
    template <typename Derived> friend class CodeGenBase;
};
struct DoccRef {
private:
    static std::optional<std::string> getTypeNameOpt(const Driver*, TypeNamePrinter::Type type);
    template <typename Derived> friend class CodeGenBase;
};


// Base class of all code gen passes. Inherit with CRTP.
// Code gen passes should perform rote, mechanical transformations of analysis passes,
// and not do anything particularly creative or interesting. 
template <class Derived>
class CodeGenBase {
public:
    typedef std::vector<const clang::TagDecl*> Data;
    
protected:
    // MARK: Virtual
    virtual ~CodeGenBase() {}
    virtual std::string fileNamePrefix() const = 0;
    virtual Data preprocess() = 0;
    virtual Data extraSpecialCaseFiltering(const Data& data) const { return data; }
    
    // Override whichever of the `writeFooFile` methods you need. Those (and only those)
    // will be called on your subclass.
    virtual void prepareWriteHeaderFile() {}
    virtual void writeHeaderFile(const Data& data) {}
    virtual void prepareWriteCppFile() {}
    virtual void writeCppFile(const Data& data) {}
    virtual void prepareWriteMmFile() {}
    virtual void writeMmFile(const Data& data) {}
    virtual void prepareWriteSwiftFile() {}
    virtual void writeSwiftFile(const Data& data) {}
    virtual void writeModulemapFile() {}
    virtual void writeAPINotesFile() {}
    virtual std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                                    std::string* outOverview,
                                                                    const Data& processedData) { return {}; }
    
    // MARK: Accessors
public:
    static std::string mangleName(const clang::NamedDecl* _namedDecl) {
        clang::ASTContext& context = _namedDecl->getASTContext();
        clang::ASTNameGenerator astNameGenerator(context);
        std::string result = astNameGenerator.getName(_namedDecl);
        if (result.size()) {
            return result;
        }
        
        // Fallback, try to do it ourselves. Might be imperfect, but hopefully good enough for our purposes
        // of ensuring functions with identical C++ signatures have distinct names
        std::vector<std::string> reversedComponents;
        const clang::NamedDecl* currentNamedDecl = _namedDecl;
        while (currentNamedDecl) {
            std::string toPushBack = currentNamedDecl->getNameAsString();
            if (toPushBack == PXR_NS && currentNamedDecl->getDeclContext()->getDeclKind() == clang::Decl::TranslationUnit) {
                toPushBack = "pxr";
            }
            reversedComponents.push_back(toPushBack);
            const clang::DeclContext* nextDeclContext = currentNamedDecl->getDeclContext();
            currentNamedDecl = nextDeclContext ? clang::dyn_cast<clang::NamedDecl>(nextDeclContext) : nullptr;
        }
        
        if (reversedComponents.empty()) {
            std::cerr << "Error: Couldn't generate mangled name for " << ASTHelpers::getAsString(_namedDecl) << std::endl;
            __builtin_trap();
        }
        
        std::stringstream ss;
        ss << "__ZN";
        for (long i = reversedComponents.size() - 1; i >= 0; i--) {
            if (i == reversedComponents.size() - 1 && reversedComponents[i] == "std") {
                ss << "St";
                continue;
            }
            ss << reversedComponents[i].size() << reversedComponents[i];
        }
        ss << "E";
        return ss.str();
    }
    
protected:
    const ASTAnalysisRunner& getAstAnalysisRunner() const {
        return _codeGenRunner->getASTAnalysisRunner();
    }
    const FindNamedDeclsAnalysisPass* getFindNamedDeclsAnalysisPass() const {
        return getAstAnalysisRunner().getFindNamedDeclsAnalysisPass();
    }
    const ImportAnalysisPass* getImportAnalysisPass() const {
        return getAstAnalysisRunner().getImportAnalysisPass();
    }
    const PublicInheritanceAnalysisPass* getPublicInheritanceAnalysisPass() const {
        return getAstAnalysisRunner().getPublicInheritanceAnalysisPass();
    }
    const EquatableAnalysisPass* getEquatableAnalysisPass() const {
        return getAstAnalysisRunner().getEquatableAnalysisPass();
    }
    const HashableAnalysisPass* getHashableAnalysisPass() const {
        return getAstAnalysisRunner().getHashableAnalysisPass();
    }
    const ComparableAnalysisPass* getComparableAnalysisPass() const {
        return getAstAnalysisRunner().getComparableAnalysisPass();
    }
    const CustomStringConvertibleAnalysisPass* getCustomStringConvertibleAnalysisPass() const {
        return getAstAnalysisRunner().getCustomStringConvertibleAnalysisPass();
    }
    const TypedefAnalysisPass* getTypedefAnalysisPass() const {
        return getAstAnalysisRunner().getTypedefAnalysisPass();
    }
    const SdfValueTypeNamesMembersAnalysisPass* getSdfValueTypeNamesMembersAnalysisPass() const {
        return getAstAnalysisRunner().getSdfValueTypeNamesMembersAnalysisPass();
    }
    const FindSchemasAnalysisPass* getFindSchemasAnalysisPass() const {
        return getAstAnalysisRunner().getFindSchemasAnalysisPass();
    }
    const FindEnumsAnalysisPass* getFindEnumsAnalysisPass() const {
        return getAstAnalysisRunner().getFindEnumsAnalysisPass();
    }
    const FindStaticTokensAnalysisPass* getFindStaticTokensAnalysisPass() const {
        return getAstAnalysisRunner().getFindStaticTokensAnalysisPass();
    }
    const FindTfNoticeSubclassesAnalysisPass* getFindTfNoticeSubclassesAnalysisPass() const {
        return getAstAnalysisRunner().getFindTfNoticeSubclassesAnalysisPass();
    }
    const SendableAnalysisPass* getSendableAnalysisPass() const {
        return getAstAnalysisRunner().getSendableAnalysisPass();
    }
    const APINotesAnalysisPass* getAPINotesAnalysisPass() const {
        return getAstAnalysisRunner().getAPINotesAnalysisPass();
    }
    const CodeGenRunner* getCodeGenRunner() const {
        return _codeGenRunner;
    }
    // Writes a line into the currently open file, during `writeFooFile`
    void writeLine(const std::string& line) {
        _writer->addLine(line);
    }
    // Writes lines into the currently open file, during `writeFooFile`
    void writeLines(const std::vector<std::string>& lines) {
        _writer->addLines(lines);
    }
    // If hasTypeName<SwiftNameInSwift>(tagDecl) is false, writes `// ' followed by the CppNameInCpp,
    // else does nothing
    void writeThatTagDeclHasNoSwiftNameInSwiftIfNeeded(const clang::TagDecl* tagDecl) {
        if (hasTypeName<SwiftNameInSwift>(tagDecl)) { return; }
        // Bypass using a TypeNamePrinter because we don't want platform guards for this
        std::string name = *CppNameInCpp::getTypeNameOpt(_codeGenRunner->getDriver(), tagDecl);
        writeLine("// " + name);
    }
private:
    // Given this type, if I am going to print its type name, what is the feature flag guard I need?
    std::optional<std::string> _featureFlagGuard(TypeNamePrinter::Type type, std::string fileNameSuffix) const {
        return TypeNamePrinter::getFeatureFlagGuard(_codeGenRunner->getDriver(), type, fileNameSuffix);
    }
    
public:
    // MARK: Name gen
    TypeNamePrinter typeNamePrinter(TypeNamePrinter::Type type) const {
        return TypeNamePrinter(type, [this, type]() {
            std::optional<std::string> guard = _featureFlagGuard(type, _writer->getOpenFileSuffix());
            if (guard) {
                const_cast<Derived*>(static_cast<const Derived*>(this))->writeLine("#endif // " + *guard);
            }
        });
    }
    template <typename Language>
    bool hasTypeName(TypeNamePrinter::Type type) const {
        return Language::getTypeNameOpt(_codeGenRunner->getDriver(), type).has_value();
    }
    // Intentionally don't take a TagDecl because TypeNamePrinter uses RAII to do platform guarding for symbols
    template <typename Language>
    std::string getTypeName(TypeNamePrinter& p) const {
        return *getTypeNameOpt<Language>(p);
    }
    // Intentionally don't take a TagDecl because TypeNamePrinter uses RAII to do platform guarding for symbols
    template <typename Language>
    std::optional<std::string> getTypeNameOpt(TypeNamePrinter& p) const {
        std::optional<std::string> guard = _featureFlagGuard(p._type, _writer->getOpenFileSuffix());
        std::optional<std::string> result = Language::getTypeNameOpt(_codeGenRunner->getDriver(), p._type);
        if (_isWritingFile && guard && result && !p._usedTypeName) {
            p._usedTypeName = true;
            const_cast<Derived*>(static_cast<const Derived*>(this))->writeLine(*guard);
        }
        return result;
    }
    
public:
    
    // MARK: Utility
    bool isDeclLocFromUsd(const clang::Decl* decl) const {
        return getFindNamedDeclsAnalysisPass()->isDeclLocFromUsd(decl);
    }
        
    bool doesTypeContainUsdTypes(const clang::TagDecl* tagDecl) const {
        return getFindNamedDeclsAnalysisPass()->doesTypeContainUsdTypes(tagDecl);
    }
    
    bool isImportedAsAnyReference(const clang::TagDecl* tagDecl) const {
        const auto& it = getImportAnalysisPass()->find(tagDecl);
        if (it == getImportAnalysisPass()->end()) {
            return false;
        }
        return it->second.isImportedAsAnyReference();
    }
    
private:
    void writeIncludeLines(const Data& data) {
        std::set<std::string> filePaths;
        for (const clang::TagDecl* tagDecl : data) {
            std::set<std::string> pathsToAdd = getUsdIncludePathsForSpelling(tagDecl);
            filePaths.merge(pathsToAdd);
        }
        
        std::vector sortedFilePaths(filePaths.begin(), filePaths.end());
        std::sort(sortedFilePaths.begin(), sortedFilePaths.end());
        
        for (const auto& f : sortedFilePaths) {
            auto guard = TypeNamePrinter::getFeatureFlagGuard(f);
            if (guard) {
                writeLine(*guard);
            }
            writeLine("#include \"" + f + "\"");
            if (guard) {
                writeLine("#endif // " + *guard);
            }
        }
        writeLine("");
    }
    
    void writeDocCFileImpl(const Data& data) {
        std::string title;
        std::string overview;
        std::vector<std::pair<std::string, Data>> sections = writeDocCFile(&title,
                                                                           &overview,
                                                                           data);

        _writer->addLines({
            "# " + title,
            "",
            "@Comment { This file was generated by ast-answerer. Do not edit! }",
            "",
            "## Overview",
            "",
            overview,
            "",
        });
        
        for (const auto& section : sections) {
            std::string topic = "";
            for (const auto& it : section.second) {
                auto printer = typeNamePrinter(it);
                if (!hasTypeName<DoccRef>(it)) { continue; }
                std::string ref = getTypeName<DoccRef>(printer);
                std::string swiftName = getTypeName<SwiftNameInSwift>(printer);
                
                std::string newTopic = getTypedefAnalysisPass()->getUsdLibraryForDecl(it) + " types";
                std::transform(newTopic.begin(), newTopic.begin() + 1, newTopic.begin(), [](auto c) { return std::toupper(c); });
                
                if (newTopic == "Vt types" && ref.find("VtArray") != std::string::npos) {
                    newTopic = "VtArray specializations";
                }
                
                if (topic == "" || newTopic != topic) {
                    _writer->addLine("");
                    _writer->addLine("### " + section.first + " " + newTopic);
                    topic = newTopic;
                }
                
                if (std::is_same_v<Derived, SendableCodeGen>) {
                    // Sendable does so many links that it slows down DocC.
                    // Additionally, a lot of the links are broken because Sendable
                    // conformances don't generate symbol graph entires on their own.
                    // So don't link, just put the list element in single-backticks for code voice
                    _writer->addLine("- `" + swiftName + "`");
                } else {
                    _writer->addLine("- ``" + ref + "``");
                }
            }
        }
    }
    
    std::set<std::string> getUsdIncludePathsForSpelling(const clang::TagDecl* tagDecl) const {
        return TypeNamePrinter::includePathsForSwiftNameInCpp(_codeGenRunner->getDriver(), tagDecl);
    }
        
protected:
    CodeGenBase(const CodeGenRunner* codeGenRunner) : _codeGenRunner(codeGenRunner) {}
    
    void setWritesPrologue(bool newValue) {
        _writer->setWritesPrologue(newValue);
    }
    
private:
    Data specialCaseFiltering(const Data& data) const {
        Data result;
        for (const auto& x : data) {
            auto printer = typeNamePrinter(x);
            auto swiftName = getTypeNameOpt<SwiftNameInSwift>(printer);
            
            // rdar://153206529 (Compiler crash when conforming type with copy-ctor with default second parameter to Equatable)
            // After https://github.com/swiftlang/swift/pull/79325, default arg copy constructors will make types ~Copyable,
            // in which case this type couldn't be Equatable anyways
            if (swiftName == "pxr.HdMeshTopology") {
                continue;
            }
            result.push_back(x);
        }
        return result;
    }
    
    // Swift treats Foo<Bar> and Foo<const Bar> the same, so we can end up with duplicate symbols
    // if we don't deduplicate consts that occur within templates specializations
    void constDeduplicate(Data& data) const {
        auto it = data.begin();
        while (it != data.end()) {
            bool advance = true;
            if (const clang::ClassTemplateSpecializationDecl* specialization = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(*it)) {
                if (ASTHelpers::isConstDuplicateSpecializationDecl(specialization)) {
                    it = data.erase(it);
                    advance = false;
                }
            }
            if (advance) {
                ++it;
            }
        }
    }
    
    // We can't call virtual methods until after the Derived ctor finishes,
    // which is after the Base ctor finishes, so use a factory pattern
    // to make sure this is always called after construction completely finishes
    void derivedCtorFinished() {
        const FileSystemInfo& fileSystemInfo = _codeGenRunner->getFileSystemInfo();
        _writer = std::make_unique<FileWriterHelper>(fileSystemInfo.getGeneratedCodeDirectory(), fileNamePrefix());
        
        Data data = preprocess();
        data = specialCaseFiltering(data);
        data = extraSpecialCaseFiltering(data);
        std::sort(data.begin(), data.end(), ASTHelpers::DeclComparator());
        constDeduplicate(data);
        
        _isWritingFile = true;
        // For each of the possible `writeFooFile` methods,
        // we only want to do the preamble if the Derived class
        // actually implemented them
        if (&Derived::writeHeaderFile != &CodeGenBase::writeHeaderFile) {
            prepareWriteHeaderFile();
            std::string upperFileNamePrefix = fileNamePrefix();
            std::transform(upperFileNamePrefix.begin(), upperFileNamePrefix.end(), upperFileNamePrefix.begin(),
                           [](unsigned char c){ return std::toupper(c); });
            _writer->openHeaderFile("SWIFTUSD_GENERATED_" + upperFileNamePrefix + "_H");
            writeIncludeLines(data);
            writeHeaderFile(data);
            _writer->closeFile();
        }
        
        if (&Derived::writeCppFile != &CodeGenBase::writeCppFile) {
            prepareWriteCppFile();
            _writer->openCppFile();
            writeCppFile(data);
            _writer->closeFile();
        }
        
        if (&Derived::writeMmFile != &CodeGenBase::writeMmFile) {
            prepareWriteMmFile();
            _writer->openMmFile();
            writeMmFile(data);
            _writer->closeFile();
        }
        
        if (&Derived::writeSwiftFile != &CodeGenBase::writeSwiftFile) {
            prepareWriteSwiftFile();
            _writer->openSwiftFile();
            writeSwiftFile(data);
            _writer->closeFile();
        }
        
        if (&Derived::writeModulemapFile != &CodeGenBase::writeModulemapFile) {
            _writer->openModulemapFile();
            writeModulemapFile();
            _writer->closeFile();
        }
        
        if (&Derived::writeAPINotesFile != &CodeGenBase::writeAPINotesFile) {
            _writer->openAPINotesFile();
            writeAPINotesFile();
            _writer->closeFile();
        }
        
        if (&Derived::writeDocCFile != &CodeGenBase::writeDocCFile) {
            _writer->openDocCFile();
            writeDocCFileImpl(data);
            _writer->closeFile();
        }
    }
    friend class CodeGenFactory;
    friend class ReferenceTypeConformanceCodeGen;
    
    // MARK: Fields
private:
    std::unique_ptr<FileWriterHelper> _writer;
    const CodeGenRunner* _codeGenRunner;
    bool _isWritingFile = false;
};

class CodeGenFactory {
public:
    template <typename T>
    static std::unique_ptr<T> makeCodeGen(const CodeGenRunner* codeGenRunner) {
        std::unique_ptr<T> result = std::make_unique<T>(codeGenRunner);
        result->derivedCtorFinished();
        return result;
    }
};

#endif /* CodeGenBase_h */
