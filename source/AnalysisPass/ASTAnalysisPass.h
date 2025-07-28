//
//  ASTAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef ASTAnalysisPass_h
#define ASTAnalysisPass_h

#include "Util/FileSystemInfo.h"
#include "Util/ClangToolHelper.h"
#include "Util/TestDataLoader.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <filesystem>
#include <string>
#include <fstream>

// Helper class that provides a lot of convenient functions for working with the clang AST
struct ASTHelpers {
    static clang::QualType removingRefConst(clang::QualType orig);
    static clang::QualType getQualType(const clang::TypeDecl* typeDecl);
    static std::string getAsString(const clang::NamedDecl* namedDecl);
    static std::string getAsString(const clang::Type* type);
    static bool isNotVisibleToSwift(clang::AccessSpecifier accessSpecifier);
    static std::string getCharacterData(const clang::SourceManager* sourceManager, const clang::SourceRange& sourceRange);
    static std::vector<const clang::CXXRecordDecl*> allAccessibleSupertypes(const clang::CXXRecordDecl* cxxRecordDecl);
    static std::vector<const clang::Type*> allAccessibleImplicitNoArgConstConversions(const clang::CXXRecordDecl* cxxRecordDecl);
    static clang::QualType getInjectedClassNameSpecialization(clang::QualType q);
    static std::vector<clang::QualType> allRecursiveTypesInTag(const clang::TagDecl* tagDecl);
    static bool isEqualToOrDerivedFromClassVisibleToSwift(const clang::CXXRecordDecl* derived, const clang::CXXRecordDecl* base);
    static bool isEqualToOrDerivedFromClass(const clang::CXXRecordDecl* derived, const clang::CXXRecordDecl* base);
    static bool isConstDuplicateSpecializationDecl(const clang::ClassTemplateSpecializationDecl* d);
    
    static bool hasSwiftAccessibleCopyCtor(const clang::CXXRecordDecl* cxxRecordDecl);
    static bool hasSwiftAccessibleMoveCtor(const clang::CXXRecordDecl* cxxRecordDecl);
    static bool hasSwiftAccessibleDtor(const clang::CXXRecordDecl* cxxRecordDecl);

    static clang::SourceLocation getLatestSourceLocation(const clang::Decl* decl);
    
    struct DeclComparator {
        DeclComparator();
        bool operator ()(const clang::Decl* lhs, const clang::Decl* rhs) const;
    };
};


// Base class of all analysis passes. Inherit with CRTP, specifying the AnalysisResult type as well
template <typename Derived, typename AnalysisResult>
class ASTAnalysisPass: public clang::RecursiveASTVisitor<ASTAnalysisPass<Derived, AnalysisResult>> {
public:
    typedef std::map<const clang::NamedDecl*, AnalysisResult, ASTHelpers::DeclComparator> Data;
    
public:
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
    
    virtual bool shouldVisitTemplateInstantiations() const { return true; }
    virtual bool shouldVisitImplicitCode() const { return true; }
    
    // Finalize is automatically called on every NameDecl that has been insert_or_assign'd,
    // at the end of AST traversal. You can use this method to finalize analysis decisions
    // that require more than a single forward pass
    virtual void finalize(const clang::NamedDecl* namedDecl) {}
    virtual bool shouldOnlyVisitDeclsFromUsd() const { return true; }
    
    // Called after the analysis pass finishes testing, whether the AST was traversed or
    // the analysis pass was loaded from disk
    virtual void analysisPassIsFinished() {}

    // MARK: AST Traversal
    bool TraverseDecl(clang::Decl* decl) {
        if (!decl) { return true; }
        
        bool fromUsd = isEarliestDeclLocFromUsd(decl) || (clang::dyn_cast<clang::TypeDecl>(decl) && doesTypeContainUsdTypes(clang::dyn_cast<clang::TypeDecl>(decl)));
        bool shouldVisit = (bool) clang::dyn_cast<clang::TranslationUnitDecl>(decl) ||
                           fromUsd ||
                           !shouldOnlyVisitDeclsFromUsd();
        
        if (shouldVisit) {
            // Visit by calling the base class implementation
            return clang::RecursiveASTVisitor<ASTAnalysisPass<Derived, AnalysisResult>>::TraverseDecl(decl);
        } else {
            // Continue traversal, but don't visit
            return true;
        }
    }

    // MARK: Accessors
private:
    const clang::NamedDecl* _prepareForDataAccess(const clang::NamedDecl* namedDecl) const {
        if (!namedDecl) {
            return namedDecl;
        }
        
        if (const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl)) {
            if (tagDecl->getDefinition()) {
                namedDecl = tagDecl->getDefinition();
            }
        }
        
        return namedDecl;
    }
public:
    // Use the find, end, and insert_or_assign methods like a stl container
    
    Data::const_iterator find(const clang::NamedDecl* namedDecl) const {
        const clang::NamedDecl* x = _prepareForDataAccess(namedDecl);
        return _data.find(x);
    }
    Data::iterator find(const clang::NamedDecl* namedDecl) {
        const clang::NamedDecl* x = _prepareForDataAccess(namedDecl);
        return _data.find(x);
    }
    Data::const_iterator end() const {
        return _data.end();
    }
    Data::iterator end() {
        return _data.end();
    }
    template <typename T>
    void insert_or_assign(const clang::NamedDecl* namedDecl, const T& value) {
        const clang::NamedDecl* x = _prepareForDataAccess(namedDecl);
        _data.insert_or_assign(x, AnalysisResult(value));
    }
    
    void insert_or_assign(const clang::NamedDecl* namedDecl, const AnalysisResult& analysisResult) {
        const clang::NamedDecl* x = _prepareForDataAccess(namedDecl);
        _data.insert_or_assign(x, analysisResult);
    }
    
    const ASTAnalysisRunner& getASTAnalysisRunner() const {
        return *_astAnalysisRunner;
    }
    
    const FileSystemInfo& getFileSystemInfo() const {
        return getASTAnalysisRunner().getFileSystemInfo();
    }
    
    // Returns the underlying map of TagDecls to AnalysisResults
    const Data& getData() const {
        return _data;
    }
    
    std::string makePathRelativeForUsd(const std::string& p) const {
        const FileSystemInfo& f = getFileSystemInfo();
        std::vector<std::string> prefixes = {
            f.usdSourceRepoPath.string(),
            f.usdInstalledHeaderPath.string(),
        };
        
        for (const auto& prefix : prefixes) {
            if (p.starts_with(prefix)) {
                return p.substr(prefix.size() + 1);
            }
        }
        
        return "";
    }
    
    std::string makePathIncludeForUsd(const std::string& p) const {
        static std::vector<std::filesystem::path> publicHeaders = getFileSystemInfo().getListOfPublicHeaders();
        for (const auto& h : publicHeaders) {
            if (h.string() == p) {
                return makePathRelativeForUsd(p);
            }
        }
        
        return "";
    }
    
    
    // MARK: Utility
    
    // Given `TfRefPtr<UsdStage>`, returns `pxr/base/tf/refPtr.h`,
    // given `shared_ptr<HdTask>`, returns a stl header for `<memory.h>`.
    // Compare with latestDeclLocFilePath.
    std::string earliestDeclLocFilePath(const clang::Decl* decl) const {
        if (!decl) { return ""; }
        
        const clang::SourceManager& sourceManager = decl->getASTContext().getSourceManager();
        // Using getExpansionLoc is _very_ important, because we care about where a macro expands to,
        // and "expansion locations represent where the location is in the user's view"
        clang::SourceLocation spellingLoc = sourceManager.getExpansionLoc(decl->getLocation());
        
        std::string filePath = sourceManager.getFilename(spellingLoc).str();
        return filePath;
    }
    
    // Given `TfRefPtr<UsdStage>`, returns `pxr/usd/usd/stage.h`,
    // given `shared_ptr<HdTask>`, returns `pxr/imaging/hd/task.h`.
    // Compare with earliestDeclLocFilePath.
    std::string latestDeclLocFilePath(const clang::Decl* decl) const {
        if (!decl) { return ""; }
        
        const clang::SourceManager& sourceManager = decl->getASTContext().getSourceManager();
        clang::SourceLocation loc = ASTHelpers::getLatestSourceLocation(decl);
        std::string filePath = sourceManager.getFilename(loc).str();
        
        return filePath;
    }
    
    std::string headerPathForUsdType(const clang::Decl* decl) const {
        return makePathRelativeForUsd(latestDeclLocFilePath(decl));
    }
    
    std::string makePathIncludeForUsd(const clang::Decl* decl) const {
        return makePathIncludeForUsd(latestDeclLocFilePath(decl));
    }
    
    bool isEarliestDeclLocFromUsd(const clang::Decl* decl) const {
        return makePathRelativeForUsd(earliestDeclLocFilePath(decl)) != "";
    }
    
    std::string getUsdLibraryForDecl(const clang::Decl* decl) const {
        std::string relative = makePathRelativeForUsd(latestDeclLocFilePath(decl));
        if (relative == "") {
            return "";
        }
        // pxr/foo/result/bar.h
        return splitStringByRegex(relative, std::regex("/"))[2];
    }
    
    bool doesTypeContainUsdTypes(const clang::TypeDecl* typeDecl) const {
        if (!typeDecl) {
            return false;
        }
        if (isEarliestDeclLocFromUsd(typeDecl)) {
            return true;
        }
        const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(typeDecl);
        if (!tagDecl) {
            return false;
        }
        
        for (clang::QualType qualType : ASTHelpers::allRecursiveTypesInTag(tagDecl)) {
            if (isEarliestDeclLocFromUsd(qualType->getAsTagDecl())) {
                return true;
            }
        }
        return false;
    }
    
    bool areAllUsdDeclsFromPublicHeaders(const clang::Decl* decl) const {
        const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(decl);
        if (!tagDecl) {
            return makePathIncludeForUsd(decl) != "";
        }
        
        for (clang::QualType qualType : ASTHelpers::allRecursiveTypesInTag(tagDecl)) {
            if (isEarliestDeclLocFromUsd(qualType->getAsTagDecl())) {
                if (makePathIncludeForUsd(qualType->getAsTagDecl()) == "") {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // Finds a TagDecl with the given name, querying the FindNamedDeclsAnalysisPass
    const clang::TagDecl* findTagDecl(const std::string& typeName) const {
        return getASTAnalysisRunner().findTagDecl(typeName);
    }
    
    // Finds a NamedDecl with the given name, querying the FindNamedDeclsAnalysisPass
    const clang::NamedDecl* findNamedDecl(const std::string& name) const {
        return getASTAnalysisRunner().findNamedDecl(name);
    }
    
    // Finds a Type with the given name, querying the FindNamedDeclsAnalysisPass
    const clang::Type* findType(const std::string& name) const {
        return getASTAnalysisRunner().findType(name);
    }
    
    // Finds a FunctionDecl with the given name, querying the FindNamedDeclsAnalysisPass
    const clang::FunctionDecl* findFunctionDecl(const std::string& signature) const {
        return getASTAnalysisRunner().findFunctionDecl(signature);
    }

    // MARK: Serialization
    virtual void serialize() const {
        std::cout << "Serializing " << serializationFileName() << std::endl;
        
        const FileSystemInfo& f = getFileSystemInfo();
        std::filesystem::path filePath = f.getSerializedAnalysisPath(serializationFileName());
        
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream stream(filePath);
        
        // Important! Don't do PXR_NS replacement, because
        // some types include `PXR_NS` as part of a token
        for (const auto& it : _data) {
            stream << ASTHelpers::getAsString(it.first) << "; ";
            stream << it.second << ";" << std::endl;
        }
        
        stream.close();
    }
    virtual bool deserialize() {
        const FileSystemInfo& f = getFileSystemInfo();
        std::filesystem::path filePath = f.getSerializedAnalysisPath(serializationFileName());
        
        if (!std::filesystem::exists(filePath)) {
            return false;
        }
        
        std::cout << "Deserializing " << serializationFileName() << std::endl;
        
        // Important! Don't do PXR_NS replacement on the serialized result data, because
        // some types include `PXR_NS` as part of a token
        std::vector<std::vector<std::string>> loadedFields = TestDataLoader::load(filePath, TestDataLoader::PxrNsReplacement::dontReplace);
        
        for (const std::vector<std::string>& line : loadedFields) {
            if (line.size() != 2) {
                std::cerr << "Expected 2 fields in deserialization, but got " << line.size() << std::endl;
                for (const std::string& x : line) {
                    std::cerr << x << "; ";
                }
                std::cerr << std::endl;
                __builtin_trap();
            }
            
            std::string declName = line[0];
            std::string data = line[1];
            
            const clang::NamedDecl* namedDecl = findNamedDecl(declName);
            if (!namedDecl) {
                std::cerr << "Could not find " << declName << " while deserializing" << std::endl;
                __builtin_trap();
            }
            
            std::optional<AnalysisResult> analysisResult = AnalysisResult::deserialize(data, static_cast<Derived*>(this));
            if (!analysisResult) {
                std::cerr << "Could not deserialize " << data << std::endl;
                __builtin_trap();
            }
            
            insert_or_assign(namedDecl, *analysisResult);
        }
        
        return true;
    }
    
protected:
    // MARK: Analysis and Testing
    ASTAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
    _astAnalysisRunner(astAnalysisRunner),
    _data({})
    {
    }
    
private:
    void analyze() {
        std::cout << "Analyzing " << serializationFileName() << std::endl;
        TraverseDecl((clang::TranslationUnitDecl*) getASTAnalysisRunner().getTranslationUnitDecl());
        for (const auto& it : _data) {
            finalize(it.first);
        }
    }
    
protected:
    // Prefer overriding comparesEqualWhileTesting().
    // Only override the test() function if really needed. The default
    // behavior ensures that testing occurs automatically after every analysis pass finishes
    virtual void test() const {
        std::cout << "Testing " << serializationFileName() << std::endl;
        // Let testing do PXR_NS replacement, because we don't usually write tests using types
        // that contain `PXR_NS` as part of a token
        std::vector<std::pair<std::string, std::string>> expected = TestDataLoader::loadTwoFields(getFileSystemInfo(), testFileName(), TestDataLoader::PxrNsReplacement::replace);
        
        uint64_t nFailures = 0;
        bool hadFailures = false;
        for (const std::pair<std::string, std::string>& line : expected) {
            const clang::NamedDecl* namedDecl = getASTAnalysisRunner().findNamedDecl(line.first);
            if (!namedDecl) {
                std::cerr << "No named decl matches " << line.first << std::endl;
                nFailures += 1;
                hadFailures = true;
                continue;
            }
            
            const auto& it = find(namedDecl);
            if (it == end()) {
                std::cerr << "No analysis for " << line.first << std::endl;
                nFailures += 1;
                hadFailures = true;
                continue;
            }
            
            std::optional<AnalysisResult> expected = AnalysisResult::deserialize(line.second, static_cast<const Derived*>(this));
            if (!expected) {
                std::cerr << "Could not deserialize " << line.second;
                std::cerr << " for " << line.first << std::endl;
                nFailures += 1;
                hadFailures = true;
                continue;
            }
            
            if (!comparesEqualWhileTesting(*expected, it->second)) {
                std::cerr << "For '" << line.first << "': Expected '" << *expected;
                std::cerr << "', but got '" << it->second << "'" << std::endl;
                nFailures += 1;
                hadFailures = true;
                continue;
            }
        }
        
        if (hadFailures) {
            std::cerr << serializationFileName() << " had " << nFailures << " failures" << std::endl;
            __builtin_trap();
        }
        
        std::cout << serializationFileName() << " passed" << std::endl;
    }
public:
    // Comparison function that tests whether an analysis result that was loaded from a test file
    // and an analysis result from running an analysis pass match, i.e. whether the test passes or fails.
    // Defaults to equality of the string representations of the arguments.
    // Prefer overriding this method to test()
    virtual bool comparesEqualWhileTesting(const AnalysisResult& expected, const AnalysisResult& actual) const {
        return std::string(expected) == std::string(actual);
    }
    
private:
    friend class ASTAnalysisPassFactory;
    
    ASTAnalysisRunner* _astAnalysisRunner;
    Data _data;
};

class ASTAnalysisPassFactory {
public:
    template <typename T>
    static std::unique_ptr<T> makeAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) {
        std::unique_ptr<T> result = std::make_unique<T>(astAnalysisRunner);
        if (!result->deserialize()) {
            result->analyze();
            result->serialize();
        }
        result->test();
        result->analysisPassIsFinished();
        return result;
    }
};



#endif /* ASTAnalysisPass_h */
