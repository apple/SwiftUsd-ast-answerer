//
//  CodeGenBase.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/3/24.
//

#include "CodeGen/CodeGenBase.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

std::string swiftSpellingForIntegralType(std::string x) {
    if (x == "bool") { return "CBool"; }
    if (x == "char") { return "CChar"; }
    if (x == "unsigned char") { return "CUnsignedChar"; }
    if (x == "short") { return "CShort"; }
    if (x == "unsigned short") { return "CUnsignedShort"; }
    if (x == "int") { return "CInt"; }
    if (x == "unsigned int") { return "CUnsignedInt"; }
    if (x == "long") { return "CLong"; }
    if (x == "unsigned long") { return "CUnsignedLong"; }
    if (x == "long long") { return "CLongLong"; }
    if (x == "unsigned long long") { return "CUnsignedLongLong"; }
    if (x == "float") { return "CFloat"; }
    if (x == "double") { return "CDouble"; }
    return x;
}


class TypeNamePrinterImpl {
public:
    // MARK: Public interface
    static std::optional<std::string> swiftNameInSwift(const Driver *driver, TypeNamePrinter::Type type) {
        TypeNamePrinterImpl impl(driver,
                                 type,
                                 "." /* namespaceSeparator */,
                                 false /* isCppNameInCpp */,
                                 false /* isDoccRef */,
                                 true /* doesTypedefSubstitutionForTemplates */,
                                 true /* failsOnMissingTypedefForTemplateSubstitution */,
                                 true /* failsOnInvalidNameForCodeGen */,
                                 true /* usesBackticksOnSwiftReservedKeywords */,
                                 false /* isCopy */);
        return impl._result;
    }
    static std::optional<std::string> swiftNameInCpp(const Driver *driver, TypeNamePrinter::Type type) {
        TypeNamePrinterImpl impl(driver,
                                 type,
                                 "::" /* namespaceSeparator */,
                                 false /* isCppNameInCpp */,
                                 false /* isDoccRef */,
                                 true /* doesTypedefSubstitutionForTemplates */,
                                 true /* failsOnMissingTypedefForTemplateSubstitution */,
                                 true /* failsOnInvalidNameForCodeGen */,
                                 false /* usesBackticksOnSwiftReservedKeywords */,
                                 false /* isCopy */);
        return impl._result;
    }
    static std::optional<std::string> doccRef(const Driver *driver, TypeNamePrinter::Type type) {
        TypeNamePrinterImpl impl(driver,
                                 type,
                                 "/" /* namespaceSeparator */,
                                 false /* isCppNameInCpp */,
                                 true /* isDoccRef */,
                                 true /* doesTypedefSubstitutionForTemplates */,
                                 true /* failsOnMissingTypedefForTemplateSubstitution */,
                                 true /* failsOnInvalidNameForCodeGen */,
                                 false /* usesBackticksOnSwiftReservedKeywords */,
                                 false /* isCopy */);
        return impl._result;
    }
    static std::optional<std::string> cppNameInCpp(const Driver *driver, TypeNamePrinter::Type type) {
        TypeNamePrinterImpl impl(driver,
                                 type,
                                 "::" /* namespaceSeparator */,
                                 true /* isCppNameInCpp */,
                                 false /* isDoccRef */,
                                 false /* doesTypedefSubstitutionForTemplates */,
                                 false /* failsOnMissingTypedefForTemplateSubstitution */,
                                 true /* failsOnInvalidNameForCodeGen */,
                                 false /* usesBackticksOnSwiftReservedKeywords */,
                                 false /* isCopy */);
        return impl._result;
    }
    static std::set<std::string> includePathsForSwiftNameInCpp(const Driver *driver, TypeNamePrinter::Type type) {
        TypeNamePrinterImpl impl(driver,
                                 type,
                                 "::" /* namespaceSeparator */,
                                 true /* isCppNameInCpp */,
                                 false /* isDoccRef */,
                                 true /* doesTypedefSubstitutionForTemplates */,
                                 true /* failsOnMissingTypedefForTemplateSubstitution */,
                                 true /* failsOnInvalidNameForCodeGen */,
                                 false /* usesBackticksOnSwiftReservedKeywords */,
                                 false /* isCopy */);
        return impl._includePaths;
    }

private:
    // MARK: Output members
    std::optional<std::string> _result;
    std::set<std::string> _includePaths;
    
    // MARK: Input members
    const Driver* _driver;
    TypeNamePrinter::Type _type;
    std::string _namespaceSeparator;
    bool _isCppNameInCpp;
    bool _isDoccRef;
    bool _doesTypedefSubstitutionForTemplates;
    bool _failsOnMissingTypedefForTemplateSubstitution;
    bool _failsOnInvalidNameForCodeGen;
    bool _usesBackticksOnSwiftReservedKeywords;
    bool _isCopy;
    
    struct _InputOnlyComparator {
        auto inputOnlyTuple(const TypeNamePrinterImpl& x) const {
            return std::make_tuple(x._driver,
                                   x._type,
                                   x._namespaceSeparator,
                                   x._isCppNameInCpp,
                                   x._isDoccRef,
                                   x._doesTypedefSubstitutionForTemplates,
                                   x._failsOnMissingTypedefForTemplateSubstitution,
                                   x._failsOnInvalidNameForCodeGen,
                                   x._usesBackticksOnSwiftReservedKeywords,
                                   x._isCopy);
        }
        
        bool operator() (const TypeNamePrinterImpl& lhs, const TypeNamePrinterImpl& rhs) const {
            return inputOnlyTuple(lhs) < inputOnlyTuple(rhs);
        }
    };
    
private:
    TypeNamePrinterImpl(const Driver* driver,
                        TypeNamePrinter::Type type,
                        std::string namespaceSeparator,
                        bool isCppNameInCpp,
                        bool isDoccRef,
                        bool doesTypedefSubstitutionForTemplates,
                        bool failsOnMissingTypedefForTemplateSubstitution,
                        bool failsOnInvalidNameForCodeGen,
                        bool usesBackticksOnSwiftReservedKeywords,
                        bool isCopy) :
    _driver(driver),
    _type(type),
    _namespaceSeparator(namespaceSeparator),
    _isCppNameInCpp(isCppNameInCpp),
    _isDoccRef(isDoccRef),
    _doesTypedefSubstitutionForTemplates(doesTypedefSubstitutionForTemplates),
    _failsOnMissingTypedefForTemplateSubstitution(failsOnMissingTypedefForTemplateSubstitution),
    _failsOnInvalidNameForCodeGen(failsOnInvalidNameForCodeGen),
    _usesBackticksOnSwiftReservedKeywords(usesBackticksOnSwiftReservedKeywords),
    _isCopy(isCopy)
    {
        static std::set<TypeNamePrinterImpl, _InputOnlyComparator> _memoized;
        if (!_memoized.contains(*this)) {
            this->_run();
            _memoized.insert(*this);
        } else {
            *this = *_memoized.find(*this);
        }
    }
    
    TypeNamePrinterImpl copyWithType(TypeNamePrinter::Type newType) {
        return TypeNamePrinterImpl(_driver,
                                   newType,
                                   _namespaceSeparator,
                                   _isCppNameInCpp,
                                   _isDoccRef,
                                   _doesTypedefSubstitutionForTemplates,
                                   _failsOnMissingTypedefForTemplateSubstitution,
                                   _failsOnInvalidNameForCodeGen,
                                   _usesBackticksOnSwiftReservedKeywords,
                                   true /* isCopy */);
    }
    
    bool _isNameComponentValidForCodeGen(const std::string& s) const {
        if (s.size() == 0) {
            // Don't support anonymous namespaces
            return false;
        }
        if (s[0] == '_') {
            // Pixar uses the convention that types with names like `_ClassMap` or `_TokenToTokenMap`
            // are internal implementation details, even when they appear in public headers.
            return false;
        }
        if (s == "pxr_tsl" || s == "rapidjson" || s.starts_with("TsTest_") || s == "tao" || s == "PXR_INTERNAL_NS_pegtl" || s == "pxr_CLI") {
            // Internal sublibraries we shouldn't be messing with
            return false;
        }
        if (s.ends_with("__DebugCodes")) {
            // pxr/base/tf/debug.h enums
            return false;
        }
        
        // Pixar uses the convention that types with names like `Usd_PrimData` or `Vt_ShapeData`
        // are internal implementation details, even when they appear in public headers.
        for (auto library : _driver->getCMakeParser()->getNamesOfPxrLibraries()) {
            library[0] = std::toupper(library[0]);
            if (s.starts_with(library + "_")) {
                return false;
            }
        }
        
        return true;
    }
    
    std::optional<TypedefAnalysisResult::Pair> _getBestTypedefSpelling(const TypedefAnalysisResult& res) const {
        std::set<TypedefAnalysisResult::Pair> typedefSpellings = res.getTypedefSpellings();
        std::vector<TypedefAnalysisResult::Pair> validCandidates;
        
        for (const TypedefAnalysisResult::Pair& pair : typedefSpellings) {
            if (!_isNameComponentValidForCodeGen(pair.second)) {
                continue;
            }
            const ImportAnalysisPass* importAnalysisPass = _driver->getASTAnalysisRunner()->getImportAnalysisPass();
            if (const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(pair.first)) {
                if (importAnalysisPass->find(tagDecl) == importAnalysisPass->end() ||
                    !importAnalysisPass->find(tagDecl)->second.isImportedSomehow()) {
                    continue;
                }
            }
            
            // swiftNameInSwift and swiftNameInCpp are equivalent here
            if (swiftNameInSwift(_driver, pair.first) == std::nullopt) {
                continue;
            }
            
            // These typedefs come from a file that we comment out of the module map due to PRIVATE_HEADER issues
            if (pair.second == "HdStExtCompGpuComputationSharedPtr" ||
                pair.second == "HdStExtCompGpuComputationResourceSharedPtr" ||
                pair.second == "HdBufferArrayRangeSharedPtrVector") {
                continue;
            }
            
            
            
            validCandidates.push_back(pair);
        }
        
        if (validCandidates.size() == 0) {
            return std::nullopt;
        }
        
        // Prefer valid candidates where the outer type is higher up the decl context chain, if there's no required platform guard
        std::sort(validCandidates.begin(), validCandidates.end(), [this](const TypedefAnalysisResult::Pair& a, const TypedefAnalysisResult::Pair& b) {
            bool aRequiresFeatureFlagGuard = TypeNamePrinter::getFeatureFlagGuard(_driver, a.first, "h").has_value();
            bool bRequiresFeatureFlagGuard = TypeNamePrinter::getFeatureFlagGuard(_driver, b.first, "h").has_value();
            
            if (aRequiresFeatureFlagGuard != bRequiresFeatureFlagGuard) {
                return bRequiresFeatureFlagGuard;
            }
            
            int aNum = _getNumberOfContainingDeclContexts(a.first);
            int bNum = _getNumberOfContainingDeclContexts(b.first);
            if (aNum < bNum) {
                return true;
            } else if (aNum > bNum) {
                return false;
            } else {
                return ASTHelpers::DeclComparator()(a.first, b.first);
            }
        });
        
        return validCandidates.front();
    }
    
    int _getNumberOfContainingDeclContexts(const clang::Decl* decl) const {
        int result = 0;
        while (decl) {
            if (!decl->getDeclContext()) {
                break;
            }
            decl = clang::dyn_cast<clang::Decl>(decl->getDeclContext());
            result += 1;
        }
        return result;
    }

    void _tryAddIncludePathForDecl(const clang::Decl* decl) {
        if (!decl) { return; }
        const TypedefAnalysisPass* typedefAnalysisPass = _driver->getASTAnalysisRunner()->getTypedefAnalysisPass();
        std::string p = typedefAnalysisPass->makePathIncludeForUsd(decl);
        if (p != "") {
            _includePaths.insert(p);
        }
    
        // Special case for TfRefPtr and TfWeakPtr, we can run into trouble using pointers to incomplete types occasionally
        if (const clang::ClassTemplateSpecializationDecl* specialization = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {
            const clang::ClassTemplateDecl* templateDecl = specialization->getSpecializedTemplate();
    
            if (templateDecl->getNameAsString() == "TfRefPtr" || templateDecl->getNameAsString() == "TfWeakPtr") {
                const clang::TemplateArgument& x = specialization->getTemplateInstantiationArgs()[0];
                if (x.getKind() == clang::TemplateArgument::ArgKind::Type) {
                    _tryAddIncludePathForDecl(x.getAsType()->getAsTagDecl());
                }
            }
        }
    }
    
    void _run() {
        std::vector<std::string> reversedComponents;
        const TypedefAnalysisPass* typedefAnalysisPass = _driver->getASTAnalysisRunner()->getTypedefAnalysisPass();
        
        struct ConstRefStarWrapper {
            bool hasOuterConst = false;
            bool hasOuterRef = false;
            bool hasOuterStar = false;
        };
        std::vector<ConstRefStarWrapper> constRefStarWrappers;
        
        // Work outwards, starting at the deepest NamedDecl, out towards the TranslationUnitDecl
        const clang::NamedDecl* currentNamedDecl = _type.getNamedDeclOpt();
        clang::QualType qualType = _type.getQualTypeOpt();
        
        while (!currentNamedDecl) {
            // We must have a QualType. If it's something like `const pxr::Foo&`,
            // then we want to print `pxr::Foo`, then add back the const ref later
            ConstRefStarWrapper currentWrapper;
            if (qualType->isReferenceType()) {
                currentWrapper.hasOuterRef = true;
                qualType = qualType.getNonReferenceType();
            }
            if (qualType->isPointerType()) {
                currentWrapper.hasOuterStar = true;
                qualType = qualType->getPointeeType();
            }
            if (qualType.isConstQualified()) {
                currentWrapper.hasOuterConst = true;
                qualType.removeLocalConst();
            }
                        
            currentNamedDecl = qualType->getAsTagDecl();

            // If we have multiple layers of */&, getAsRecordDecl() just returned null
            // If we didn't apply anything but getAsRecordDecl() is null, we must have
            // a non-tag type

            if (currentWrapper.hasOuterConst || currentWrapper.hasOuterRef || currentWrapper.hasOuterStar) {
                constRefStarWrappers.push_back(currentWrapper);
            } else {
                break;
            }
        }
        
        if (!currentNamedDecl) {
            // We must have a QualType to a non-declared type, i.e. a builtin.
            std::string tmp = qualType.getAsString();
            if (tmp == "_Bool") { tmp = "bool"; }
            
            if (_usesBackticksOnSwiftReservedKeywords || _isDoccRef) {
                tmp = swiftSpellingForIntegralType(tmp);
            }
            reversedComponents.push_back(tmp);
        }
        
        if (currentNamedDecl == _driver->getASTAnalysisRunner()->findNamedDecl("std::string")) {
            currentNamedDecl = nullptr;
            reversedComponents.push_back("string");
            reversedComponents.push_back("std");
        }
        
        while (currentNamedDecl) {
            _tryAddIncludePathForDecl(currentNamedDecl);
            std::string toPushBack = currentNamedDecl->getNameAsString();
            
            // Special case template specializations
            if (const clang::ClassTemplateSpecializationDecl* specialization = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(currentNamedDecl)) {
                
                if (_doesTypedefSubstitutionForTemplates) {
                    // Try to find a substitution.
                    const auto& it = typedefAnalysisPass->find(specialization);
                    std::optional<TypedefAnalysisResult::Pair> typedefSubstitution = std::nullopt;
                    if (it != typedefAnalysisPass->end()) {
                        typedefSubstitution = _getBestTypedefSpelling(it->second);
                    }
                    
                    if (typedefSubstitution) {
                        if (!_isDoccRef) {
                            // We found a substitution, so use it
                            currentNamedDecl = typedefSubstitution->first;
                            reversedComponents.push_back(typedefSubstitution->second);
                            _tryAddIncludePathForDecl(typedefSubstitution->first);
                            
                            std::string typedefStringForFindingTypedef = typedefSubstitution->first->getQualifiedNameAsString() + "::" + typedefSubstitution->second;
                            if (const clang::NamedDecl* foundNamedDecl = typedefAnalysisPass->findNamedDecl(typedefStringForFindingTypedef)) {
                                if (clang::dyn_cast<clang::TypedefNameDecl>(foundNamedDecl)) {
                                    _tryAddIncludePathForDecl(foundNamedDecl);
                                } else {
                                    std::cerr << "Error! Search found non typedef name decl " << typedefStringForFindingTypedef << std::endl;
                                    __builtin_trap();
                                }
                            } else {
                                std::cerr << "Error! Couldn't find named decl " << typedefStringForFindingTypedef << std::endl;
                                __builtin_trap();
                            }
                            
                            continue;
                        } else {
                            // DocC refs require a valid typedef substitution,
                            // because we don't want to refs to succeed where
                            // Swift type names would fail. But,
                            // DocC refs don't _do_ typedef substitution,
                            // so fallthrough and pretend like we never wanted
                            // typedef substitution
                        }
                        
                    } else if (_failsOnMissingTypedefForTemplateSubstitution) {
                        return;
                        
                    } else {
                        // pass. We'll just handle this not via substitution
                    }
                }
                
                // Not handling via a substitution
                toPushBack += "<";
                const clang::TemplateArgumentList& args = specialization->getTemplateInstantiationArgs();
                for (int i = 0; i < args.size(); i++) {
                    const auto& arg = args[i];
                    
                    // For each arg, print it into `toPushBack`
                    if (arg.getKind() == clang::TemplateArgument::Type) {
                        clang::QualType qualType = arg.getAsType();
                        
                        TypeNamePrinterImpl recurse = this->copyWithType(qualType);
                        if (recurse._result == std::nullopt) {
                            return;
                        }
                        _includePaths.merge(recurse._includePaths);
                        toPushBack += *recurse._result;
                    } else {
                        // This template argument isn't a type, so it could be a template, a value, etc.
                        // Just have llvm print it for us.
                        std::string temp;
                        llvm::raw_string_ostream ss(temp);
                        arg.print(clang::PrintingPolicy(clang::LangOptions()), ss, true);
                        toPushBack += temp;
                    }
                    
                    if (i + 1 < args.size()) {
                        toPushBack += ", ";
                    }
                }
                
                toPushBack += ">";
            } // end ClassTemplateSpecializationDecl
            
            // Always substitute the pixar namespace for `pxr`
            if (toPushBack == PXR_NS && currentNamedDecl->getDeclContext()->getDeclKind() == clang::Decl::TranslationUnit) {
                toPushBack = "pxr";
            }
            if (toPushBack == "Type" && _usesBackticksOnSwiftReservedKeywords) {
                toPushBack = "`Type`";
            }
            
            if (_failsOnInvalidNameForCodeGen) {
                // Never do code gen for types in tbb, or specializations thereof
                if (toPushBack == "tbb" && currentNamedDecl->getDeclContext()->getDeclKind() == clang::Decl::TranslationUnit) {
                    return;
                }
                
#warning suspect use of declLocFromUsd
                if (_driver->getASTAnalysisRunner()->getTypedefAnalysisPass()->isEarliestDeclLocFromUsd(currentNamedDecl) && !_isNameComponentValidForCodeGen(toPushBack)) {
                    return;
                }
            }
            
            // Alright, we're at the tail end of the while loop for this NamedDecl
            reversedComponents.push_back(toPushBack);
            // Repeat on the declContext
            const clang::DeclContext* nextDeclContext = currentNamedDecl->getDeclContext();
            if (!nextDeclContext) {
                break;
            }
            currentNamedDecl = clang::dyn_cast<clang::NamedDecl>(nextDeclContext);
            
            
        } // end while(namedDecl)
        
        if (_isDoccRef && !_isCopy) {
            reversedComponents.push_back("C++");
            reversedComponents.push_back("OpenUSD");
        }
        
        if (_isDoccRef && _isCopy) {
            // When DocC encounters a specialized template, it spells out every template argument, including nested template arguments,
            // and every template argument is listed as its fully qualified name, but using `.` instead of `/` to separate components.
            // So, if we're in DocC and we're a recursing copy, our caller must be a template and we're its argument.
            _namespaceSeparator = ".";
        }
        
        if (_usesBackticksOnSwiftReservedKeywords) {
            // pxr::UsdZipFile::Iterator is renamed to pxr.UsdZipFile.pxrIterator in Swift
            if (reversedComponents == std::vector<std::string>({"Iterator", "UsdZipFile", "pxr"})) {
                reversedComponents[0] = "pxrIterator";
            }
        }
        
        // Clean up, just join the reversedComponents.
        std::stringstream ss;
        for (long i = reversedComponents.size() - 1; i >= 0; i--) {
            ss << reversedComponents[i];
            if (i != 0) {
                ss << _namespaceSeparator;
            }
        }
        _result = ss.str();
        
        if (_isCppNameInCpp) {
            if (_result->starts_with("std::__1")) {
                _result = "std" + _result->substr(std::string("std::__1").size());
            }
        }
        
        for (int i = ((int)constRefStarWrappers.size()) - 1; i >= 0; i--) {
            ConstRefStarWrapper x = constRefStarWrappers[i];
            
            if (!_isDoccRef) {
                if (_usesBackticksOnSwiftReservedKeywords) {
                    if (x.hasOuterRef || x.hasOuterStar) {
                        _result = std::string("Unsafe") + (x.hasOuterConst ? "Mutable" : "") + "Pointer<" + *_result + ">";
                    }
                } else {
                    if (x.hasOuterConst) {
                        _result = *_result + " const";
                    }
                    if (x.hasOuterRef) {
                        _result = *_result + " &";
                    }
                    if (x.hasOuterStar) {
                        _result = *_result + " *";
                    }
                }
            }
        }
        
        if (_isDoccRef) {
            std::string toReplace = "OpenUSD/C++/std/__1";
            if (_result->starts_with(toReplace)) {
                _result = "OpenUSD/C++/std" + _result->substr(toReplace.size());
            }
        }
        
        if (_isDoccRef && _result == "std.string") {
            // DocC uses the full templated declaration for std.string
            _result = "std.__1.basic_string<CChar, std.__1.char_traits<CChar>, std.__1.allocator<CChar>>";
        }
    }
};

std::vector<std::string> _featureFlagGuardSet(std::string filePath, std::string openFileSuffix) {
    bool isLangCpp = openFileSuffix == "h" || openFileSuffix == "cpp" || openFileSuffix == "mm";
    bool isLangSwift = openFileSuffix == "swift";
    bool isLangNoGuard = openFileSuffix == "modulemap" || openFileSuffix == "apinotes" || openFileSuffix == "md" || openFileSuffix == "";
    
    if (isLangNoGuard) { return {}; }
    
    if (!isLangCpp && !isLangSwift && !isLangNoGuard) {
        std::cerr << "Unknown openFileSuffix " << openFileSuffix << std::endl;
        __builtin_trap();
    }
        
    std::vector<std::string> result;
    
    if (filePath.starts_with("pxr/usd/usdMtlx")) {
        result.push_back(isLangSwift ? "canImport(SwiftUsd_PXR_ENABLE_MATERIALX_SUPPORT)" : "SwiftUsd_PXR_ENABLE_MATERIALX_SUPPORT");
    }
    if (filePath.starts_with("pxr/imaging")) {
        result.push_back(isLangSwift ? "canImport(SwiftUsd_PXR_ENABLE_IMAGING_SUPPORT)" : "SwiftUsd_PXR_ENABLE_IMAGING_SUPPORT");
    }
    if (filePath == "pxr/imaging/garch/glPlatformContextDarwin.h") {
        result.push_back(isLangSwift ? "canImport(Darwin)" : "defined(ARCH_OS_DARWIN)");
    }
    if (filePath == "pxr/imaging/garch/glPlatformContext.h" || filePath == "pxr/imaging/glf/glRawContext.h") {
        // pxr/imaging/garch/glPlatformContextGLX.h pulls in X11 headers, which `#define`
        // normal identifiers like `Always` and `Bool` and `KeyRelease` to be macros.
        // This causes lots of downstream errors because the preprocessor then replaces
        // those identifiers with integer literals or types.
        //
        // So, pxr/imaging/garch/glPlatformContextGLX.h is excluded from the modulemap.
        // On Linux, pxr/imaging/garch/glPlatformContext.h and pxr/imaging/glf/glRawContext.h
        // end up directly or indirectly including glPlatformContextGLX.h, so
        // on Linux those headers are also excluded from the modulemap. This means that
        // any types defined in these headers won't be visible to Swift or C++
        // on Linux
        result.push_back(isLangSwift ? "!os(Linux)" : "!defined(ARCH_OS_LINUX)");
    }
    if (filePath.starts_with("pxr/imaging/hgiMetal")) {
        result.push_back(isLangSwift ? "canImport(Metal)" : "__has_include(<Metal/Metal.h>)");
    }
    if (filePath.starts_with("pxr/usdImaging")) {
        result.push_back(isLangSwift ? "canImport(SwiftUsd_PXR_ENABLE_USD_IMAGING_SUPPORT)" : "SwiftUsd_PXR_ENABLE_USD_IMAGING_SUPPORT");
    }
    
    return result;
}

std::optional<std::string> _featureFlagGuard(std::vector<std::string> filePaths, std::string openFileSuffix) {
    std::vector<std::string> featureFlagGuards;
    std::set<std::string> usedFeatureFlagGuards;
    
    for (const auto& filePath : filePaths) {
        for (const auto& guard : _featureFlagGuardSet(filePath, openFileSuffix)) {
            if (!usedFeatureFlagGuards.contains(guard)) {
                featureFlagGuards.push_back(guard);
                usedFeatureFlagGuards.insert(guard);
            }
        }
    }
    
    if (featureFlagGuards.size()) {
        std::stringstream ss;
        ss << "#if ";
        for (int i = 0; i < featureFlagGuards.size(); i++) {
            ss << featureFlagGuards[i];
            if (i + 1 < featureFlagGuards.size()) {
                ss << " && ";
            }
        }
        return ss.str();
    }
    
    return std::nullopt;

}

// Given this named decl, if I am going to print its type name, what is the feature flag guard I need?
std::optional<std::string> TypeNamePrinter::getFeatureFlagGuard(const Driver* driver, TypeNamePrinter::Type type, std::string openFileSuffix) {
    auto includePaths = includePathsForSwiftNameInCpp(driver, type);
    return _featureFlagGuard(std::vector(includePaths.begin(), includePaths.end()), openFileSuffix);
}

std::optional<std::string> TypeNamePrinter::getFeatureFlagGuard(std::string includedHeader) {
    return _featureFlagGuard({includedHeader}, "h");
}

std::optional<std::string> SwiftNameInSwift::getTypeNameOpt(const Driver *driver, TypeNamePrinter::Type type) {
    return TypeNamePrinterImpl::swiftNameInSwift(driver, type);
}

std::optional<std::string> SwiftNameInCpp::getTypeNameOpt(const Driver *driver, TypeNamePrinter::Type type) {
    return TypeNamePrinterImpl::swiftNameInCpp(driver, type);
}

std::optional<std::string> CppNameInCpp::getTypeNameOpt(const Driver *driver, TypeNamePrinter::Type type) {
    return TypeNamePrinterImpl::cppNameInCpp(driver, type);
}

std::optional<std::string> DoccRef::getTypeNameOpt(const Driver *driver, TypeNamePrinter::Type type) {
    return TypeNamePrinterImpl::doccRef(driver, type);
}

std::set<std::string> TypeNamePrinter::includePathsForSwiftNameInCpp(const Driver *driver, TypeNamePrinter::Type type) {
    return TypeNamePrinterImpl::includePathsForSwiftNameInCpp(driver, type);
}
