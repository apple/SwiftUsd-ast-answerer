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

#include "ASTAnalysisPass.h"
#include <fstream>

// MARK: Static members

clang::QualType ASTHelpers::removingRefConst(clang::QualType orig) {
    clang::QualType result = orig;
    
    if (!result->isReferenceType()) {
        return clang::QualType();
    }
    result = result.getNonReferenceType();
    
    if (!result.isConstQualified()) {
        return clang::QualType();
    }
    result.removeLocalConst();
    
    return result;
}
clang::QualType ASTHelpers::getQualType(const clang::TypeDecl* typeDecl) {
    return typeDecl->getTypeForDecl()->getCanonicalTypeUnqualified();
}
std::string ASTHelpers::getAsString(const clang::NamedDecl* namedDecl) {
    if (const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl)) {
        return getQualType(tagDecl).getAsString();
    }
    
    if (const clang::TemplateTypeParmDecl* templateTypeParmDecl = clang::dyn_cast<clang::TemplateTypeParmDecl>(namedDecl)) {
        // Note: don't use the fully qualified name for the template type parameter,
        // because that would return e.g. `template <class PXR_NS::TfRefPtr::T> class PXR_NS::TfRefPtr`,
        // not `template <class T> class PXR_NS::TfRefPtr`
        if (templateTypeParmDecl->wasDeclaredWithTypename()) {
            return "typename " + templateTypeParmDecl->getNameAsString();
        } else {
            return "class " + templateTypeParmDecl->getNameAsString();
        }
    }
    
    if (const clang::ClassTemplateDecl* classTemplateDecl = clang::dyn_cast<clang::ClassTemplateDecl>(namedDecl)) {
        std::vector<std::string> templateArguments;
        for (const clang::NamedDecl* templateArg : *classTemplateDecl->getTemplateParameters()) {
            templateArguments.push_back(ASTHelpers::getAsString(templateArg));
        }
        
        std::stringstream ss;
        ss << "template <";
        for (int i = 0; i < templateArguments.size(); i++) {
            ss << templateArguments[i];
            if (i + 1 < templateArguments.size()) {
                ss << ", ";
            }
        }
        ss << "> ";
        if (classTemplateDecl->getTemplatedDecl()->getTagKind() == clang::TagTypeKind::Class) {
            ss << "class ";
        } else {
            ss << "struct ";
        }
        ss << namedDecl->getQualifiedNameAsString();
        
        std::string finalAnswer = ss.str();
        return finalAnswer;
    } // end if clang::ClassTemplateDecl
    
    if (const clang::FunctionDecl* functionDecl = clang::dyn_cast<clang::FunctionDecl>(namedDecl)) {
        std::vector<std::string> templateArguments;
        std::vector<std::string> prefixModifiers;
        std::string retType;
        std::string qualifiedFName;
        std::vector<std::string> parameters;
        std::vector<std::string> suffixModifiers;
        
        if (const clang::FunctionTemplateDecl* functionTemplateDecl = functionDecl->getDescribedFunctionTemplate()) {
            for (const clang::NamedDecl* templateArg : *functionTemplateDecl->getTemplateParameters()) {
                templateArguments.push_back(ASTHelpers::getAsString(templateArg));
            }
        }
        
        retType = ASTHelpers::getAsString(functionDecl->getReturnType().getTypePtr());
        
        if (const clang::CXXMethodDecl* cxxMethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(namedDecl)) {
            if (const clang::CXXConversionDecl* cxxConversionDecl = clang::dyn_cast<clang::CXXConversionDecl>(namedDecl)) {
                if (cxxConversionDecl->isExplicit()) {
                    prefixModifiers.push_back("explicit");
                }
            }
            
            if (cxxMethodDecl->isStatic()) {
                prefixModifiers.push_back("static");
            }
            if (cxxMethodDecl->isVirtual()) {
                prefixModifiers.push_back("virtual");
            }
            if (cxxMethodDecl->isConst()) {
                suffixModifiers.push_back("const");
            }
            if (cxxMethodDecl->isVolatile()) {
                suffixModifiers.push_back("volatile");
            }
            if (cxxMethodDecl->getRefQualifier() == clang::RQ_RValue) {
                suffixModifiers.push_back("&&");
            }
        }
        
        qualifiedFName = namedDecl->getQualifiedNameAsString();
                
        for (const clang::ParmVarDecl* parmVarDecl : functionDecl->parameters()) {
            std::string parmType = ASTHelpers::getAsString(parmVarDecl->getType().getTypePtr());
            std::string parmName = parmVarDecl->getName().str();
            if (parmName.empty()) {
                parameters.push_back(parmType);
            } else {
                parameters.push_back(parmType + " " + parmName);
            }
        }
        
        std::stringstream ss;
        if (!templateArguments.empty()) {
            ss << "template <";
            for (int i = 0; i < templateArguments.size(); i++) {
                ss << templateArguments[i];
                if (i + 1 < templateArguments.size()) {
                    ss << ", ";
                }
            }
            ss << "> ";
        }
        for (const std::string& s : prefixModifiers) {
            ss << s << " ";
        }
        ss << retType << " ";
        ss << qualifiedFName << "(";
        for (int i = 0; i < parameters.size(); i++) {
            ss << parameters[i];
            if (i + 1 < parameters.size()) {
                ss << ", ";
            }
        }
        ss << ")";
        for (const std::string& s : suffixModifiers) {
            ss << " " << s;
        }
        
        std::string finalAnswer = ss.str();
        return finalAnswer;
    } // end if clang::FunctionDecl
    
    if (const clang::VarDecl* varDecl = clang::dyn_cast<clang::VarDecl>(namedDecl)) {
        std::stringstream ss;
        ss << varDecl->getQualifiedNameAsString() << " " << ASTHelpers::getAsString(varDecl->getType().getTypePtr());
        return ss.str();
    }
    
    return namedDecl->getQualifiedNameAsString();
}
std::string ASTHelpers::getAsString(const clang::Type *type) {
    return clang::QualType(type->getCanonicalTypeUnqualified()).getAsString();
}

bool ASTHelpers::isNotVisibleToSwift(clang::AccessSpecifier accessSpecifier) {
    return accessSpecifier == clang::AS_private || accessSpecifier == clang::AS_protected;
}
std::string ASTHelpers::getCharacterData(const clang::SourceManager* sourceManager, const clang::SourceRange& sourceRange) {
    const char* sourceRangeStart = sourceManager->getCharacterData(sourceRange.getBegin());
    const char* sourceRangeEnd = sourceManager->getCharacterData(sourceRange.getEnd());
    
    return std::string(sourceRangeStart, sourceRangeEnd - sourceRangeStart + 1);
}

std::vector<const clang::CXXRecordDecl*> ASTHelpers::allAccessibleSupertypes(const clang::CXXRecordDecl* cxxRecordDecl) {
    std::vector<const clang::CXXRecordDecl*> result = {cxxRecordDecl};
    // `result` gets longer as this for loop proceeds,
    // but we use an index and check against the size, so that's okay
    for (uint64_t i = 0; i < result.size(); i++) {
        const clang::CXXRecordDecl* current = result[i]->getDefinition();
        if (!current) {
            continue;
        }
        
        for (clang::CXXBaseSpecifier cxxBaseSpecifier : current->bases()) {
            if (ASTHelpers::isNotVisibleToSwift(cxxBaseSpecifier.getAccessSpecifier())) {
                continue;
            }
            const clang::CXXRecordDecl* baseRecord = cxxBaseSpecifier.getType()->getAsCXXRecordDecl();
            if (baseRecord) {
                result.push_back(baseRecord);
            }
        }
    }
    return result;
}

std::vector<const clang::Type*> ASTHelpers::allAccessibleImplicitNoArgConstConversions(const clang::CXXRecordDecl* cxxRecordDecl) {
    std::vector<const clang::Type*> result;
    for (clang::CXXMethodDecl* cxxMethodDecl : cxxRecordDecl->methods()) {
        const clang::CXXConversionDecl* cxxConversionDecl = clang::dyn_cast<clang::CXXConversionDecl>(cxxMethodDecl);
        if (!cxxConversionDecl) {
            continue;
        }
        
        if (cxxConversionDecl->isDeleted() || isNotVisibleToSwift(cxxConversionDecl->getAccess())) {
            continue;
        }
        
        if (cxxConversionDecl->isExplicit() ||
            !cxxConversionDecl->isConst() || 
            cxxConversionDecl->parameters().size() != 0) {
            continue;
        }
        
        clang::QualType qualType = cxxConversionDecl->getConversionType();
        result.push_back(qualType.getTypePtr());
    }
    return result;
}

clang::QualType ASTHelpers::getInjectedClassNameSpecialization(clang::QualType q) {
    if (q.isNull()) {
        return clang::QualType();
    }
    const clang::CXXRecordDecl* cxxRecordDecl = q->getAsCXXRecordDecl();
    if (!cxxRecordDecl) {
        return clang::QualType();
    }
    
    const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl);
    if (!classTemplateSpecializationDecl) {
        return clang::QualType();
    }
    
    return classTemplateSpecializationDecl->getSpecializedTemplate()->getInjectedClassNameSpecialization();
}


std::vector<clang::QualType> ASTHelpers::allRecursiveTypesInTag(const clang::TagDecl* tagDecl) {
    std::vector<clang::QualType> result;
    result.push_back(tagDecl->getTypeForDecl()->getCanonicalTypeUnqualified());
    
    int64_t i = -1;
    while (i + 1 < (int64_t) result.size()) {
        i += 1;
        if (result[i].isNull()) { continue; }
        const clang::TagDecl* thisTagDecl = result[i]->getAsTagDecl();
        if (!thisTagDecl) { continue; }
        const clang::ClassTemplateSpecializationDecl* specialization = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(thisTagDecl);
        if (!specialization) { continue; }
        for (const clang::TemplateArgument& templateArgument : specialization->getTemplateInstantiationArgs().asArray()) {
            if (templateArgument.getKind() == clang::TemplateArgument::Type && !templateArgument.getAsType().isNull()) {
                result.push_back(templateArgument.getAsType());
            }
        }
    }
    
    return result;
}

bool _isEqualToOrDerivedFromClass(const clang::CXXRecordDecl* derived, const clang::CXXRecordDecl* base, bool visibleToSwift) {
    if (derived == base) {
        return true;
    }
    
    for (const clang::CXXBaseSpecifier& cxxBaseSpecifier : derived->bases()) {
        if (visibleToSwift && ASTHelpers::isNotVisibleToSwift(cxxBaseSpecifier.getAccessSpecifier())) {
            continue;
        }
        if (const clang::CXXRecordDecl* derivedParent = cxxBaseSpecifier.getType()->getAsCXXRecordDecl()) {
            if (_isEqualToOrDerivedFromClass(derivedParent, base, visibleToSwift)) {
                return true;
            }
        }
    }
    return false;
}

bool ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(const clang::CXXRecordDecl* derived, const clang::CXXRecordDecl* base) {
    return _isEqualToOrDerivedFromClass(derived, base, true);
}

bool ASTHelpers::isEqualToOrDerivedFromClass(const clang::CXXRecordDecl* derived, const clang::CXXRecordDecl* base) {
    return _isEqualToOrDerivedFromClass(derived, base, false);
}

bool ASTHelpers::isConstDuplicateSpecializationDecl(const clang::ClassTemplateSpecializationDecl* d) {
    static std::map<const clang::ClassTemplateSpecializationDecl*, bool> memoized;
    if (memoized.find(d) != memoized.end()) {
        return memoized.find(d)->second;
    }
    
    const clang::TemplateArgumentList& argList = d->getTemplateInstantiationArgs();
    for (int i = 0; i < argList.size(); i++) {
        if (argList[i].getKind() != clang::TemplateArgument::Type) {
            // Bailing on non-type arguments isn't correct in general,
            // but Swift doesn't support other kinds of template arguments
            memoized.insert({d, false});
            return false;
        }
    }
    
    for (const clang::ClassTemplateSpecializationDecl* otherSpecialization : d->getSpecializedTemplate()->specializations()) {
        const clang::TemplateArgumentList& otherArgList = otherSpecialization->getTemplateInstantiationArgs();
        if (argList.size() != otherArgList.size()) {
            continue;
        }
        
        bool hadAtLeastOneRemovableConst = false;
        bool allArgumentsMatch = true;
        for (int i = 0; i < argList.size(); i++) {
            clang::TemplateArgument thisArgument = argList[i];
            clang::TemplateArgument otherArgument = otherArgList[i];
            
            if (otherArgument.getKind() != clang::TemplateArgument::Type) {
                memoized.insert({d, false});
                return false;
            }
            clang::QualType thisType = thisArgument.getAsType();
            clang::QualType otherType = otherArgument.getAsType();
            if (thisType.isConstQualified() && !otherType.isConstQualified()) {
                thisType.removeLocalConst();
                if (thisType.getCanonicalType() == otherType.getCanonicalType()) {
                    hadAtLeastOneRemovableConst = true;
                } else {
                    allArgumentsMatch = false;
                    break;
                }
            } else {
                if (thisType.getCanonicalType() != otherType.getCanonicalType()) {
                    allArgumentsMatch = false;
                    break;
                }
            }
        }
        
        if (hadAtLeastOneRemovableConst && allArgumentsMatch) {
            memoized.insert({d, true});
            return true;
        }
    }
    
    memoized.insert({d, false});
    return false;
}

bool ASTHelpers::isTemplatedOrNestedWithinTemplate(const clang::Decl* decl) {
    if (clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {
        return true;
    }
    
    const clang::DeclContext* declContext = decl->getDeclContext();
    if (!declContext) { return false; }
    const clang::Decl* declContextDecl = clang::dyn_cast<clang::Decl>(declContext);
    return isTemplatedOrNestedWithinTemplate(declContextDecl);
}


bool ASTHelpers::hasSwiftAccessibleCopyCtor(const clang::CXXRecordDecl* cxxRecordDecl) {
    for (const clang::CXXConstructorDecl* cxxConstructorDecl : cxxRecordDecl->ctors()) {
        if (!cxxConstructorDecl->isCopyConstructor()) { continue; }
        if (cxxConstructorDecl->isDeleted() ||
            cxxConstructorDecl->isIneligibleOrNotSelected() ||
            ASTHelpers::isNotVisibleToSwift(cxxConstructorDecl->getAccess())) {
            continue;
        }
        return true;
    }
    return cxxRecordDecl->needsImplicitCopyConstructor() && !cxxRecordDecl->defaultedCopyConstructorIsDeleted();
}
bool ASTHelpers::hasSwiftAccessibleMoveCtor(const clang::CXXRecordDecl* cxxRecordDecl) {
    // Is there an explicitly declared accessible moved constructor?
    for (const clang::CXXConstructorDecl* cxxConstructorDecl : cxxRecordDecl->ctors()) {
        if (!cxxConstructorDecl->isMoveConstructor()) { continue; }
        if (cxxConstructorDecl->isDeleted() ||
            cxxConstructorDecl->isIneligibleOrNotSelected() ||
            ASTHelpers::isNotVisibleToSwift(cxxConstructorDecl->getAccess())) {
            continue;
        }
        return true;
    }
    
    return cxxRecordDecl->needsImplicitMoveConstructor() && !cxxRecordDecl->defaultedMoveConstructorIsDeleted();
}
bool ASTHelpers::hasSwiftAccessibleDtor(const clang::CXXRecordDecl* cxxRecordDecl) {
    if (const clang::CXXDestructorDecl* cxxDestructorDecl = cxxRecordDecl->getDestructor()) {
        if (cxxDestructorDecl->isDeleted() ||
            cxxDestructorDecl->isIneligibleOrNotSelected() ||
            ASTHelpers::isNotVisibleToSwift(cxxDestructorDecl->getAccess())) {
            return false;
        }
    } else if (cxxRecordDecl->defaultedDestructorIsDeleted()) {
        return false;
    }
    return true;
}

clang::SourceLocation _getLatestSourceLocationImpl(const clang::Decl* _decl) {
    std::vector<const clang::Decl*> toProcess = {_decl};
    std::vector<clang::SourceLocation> locations;
    
    auto onQualType = [&](clang::QualType qualType){
        if (const clang::TagDecl* tagDecl = qualType->getAsTagDecl()) {
            toProcess.push_back(tagDecl);
        }
    };
    
    std::function<void(clang::TemplateArgument arg)> onTemplateArg = [&](clang::TemplateArgument arg) {
        switch (arg.getKind()) {
            case clang::TemplateArgument::Null: break;
            case clang::TemplateArgument::Type:
                onQualType(arg.getAsType());
                break;
            case clang::TemplateArgument::Declaration: // fallthrough
            case clang::TemplateArgument::NullPtr: {
                clang::ValueDecl* valueDecl = arg.getAsDecl();
                onQualType(valueDecl->getType());
                break;
            };
            case clang::TemplateArgument::Integral: break;
            case clang::TemplateArgument::StructuralValue: break;
            case clang::TemplateArgument::Template: // fallthrough
            case clang::TemplateArgument::TemplateExpansion: {
                clang::TemplateName templateName = arg.getAsTemplateOrTemplatePattern();
                if (const clang::TemplateDecl* templateDecl = templateName.getAsTemplateDecl()) {
                    toProcess.push_back(templateDecl);
                }
                break;
            };
            case clang::TemplateArgument::Expression: break;
            case clang::TemplateArgument::Pack: {
                for (const auto& packArg : arg.getPackAsArray()) {
                    onTemplateArg(packArg);
                }
                break;
            };
        }
    };
    
    for (int i = 0; i < toProcess.size(); i++) {
        // Using getExpansionLoc is _very_ important, because we care about where a macro expands to,
        // and "expansion locations represent where the location is in the user's view",
        // per https://clang.llvm.org/doxygen/classclang_1_1SourceManager.html#details

        const clang::SourceManager& sourceManager = _decl->getASTContext().getSourceManager();
        const clang::Decl* decl = toProcess[i];
        locations.push_back(sourceManager.getExpansionLoc(decl->getLocation()));
        
        if (const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {
            
            const auto& templateArgs = classTemplateSpecializationDecl->getTemplateInstantiationArgs();
            for (const auto& arg : templateArgs.asArray()) {
                onTemplateArg(arg);
            }
        }
    }
    
    std::sort(locations.begin(), locations.end());
    return locations.back();
}

clang::SourceLocation ASTHelpers::getLatestSourceLocation(const clang::Decl* decl) {
    static std::unordered_map<const clang::Decl*, clang::SourceLocation> memo;
    if (memo.find(decl) == memo.end()) {
        memo.insert({decl, _getLatestSourceLocationImpl(decl)});
    }
    return memo.find(decl)->second;
}


ASTHelpers::DeclComparator::DeclComparator() {}
bool ASTHelpers::DeclComparator::operator ()(const clang::Decl* lhs, const clang::Decl* rhs) const {
    if (lhs == nullptr) {
        return false;
    }
    if (rhs == nullptr) {
        return true;
    }
    
    clang::SourceLocation lhsSourceLoc = ASTHelpers::getLatestSourceLocation(lhs);
    clang::SourceLocation rhsSourceLoc = ASTHelpers::getLatestSourceLocation(rhs);
    
    if (lhsSourceLoc == rhsSourceLoc) {
        // Important! Don't let templates be equal
        if (const clang::NamedDecl* lNamed = clang::dyn_cast<clang::NamedDecl>(lhs)) {
            if (const clang::NamedDecl* rNamed = clang::dyn_cast<clang::NamedDecl>(rhs)) {
                // Try to provide a stable ordering based on sorting of names
                
                std::string lString = ASTHelpers::getAsString(lNamed);
                std::string rString = ASTHelpers::getAsString(rNamed);
                
                if (lString != rString) {
                    return lString < rString;
                }
            }
        }
        
        // Fallback to pointer ordering, which isn't stable
        return lhs < rhs;
    } else {
        return lhsSourceLoc < rhsSourceLoc;
    }
}


// MARK: Instance members

std::string ASTHelpers::makePathRelativeForUsd(const std::string& p) const {
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

std::string ASTHelpers::makePathIncludeForUsd(const std::string& p) const {
    static std::vector<std::filesystem::path> publicHeaders = getFileSystemInfo().getListOfPublicHeaders();
    for (const auto& h : publicHeaders) {
        if (h.string() == p) {
            return makePathRelativeForUsd(p);
        }
    }
    
    return "";
}

// Given `TfRefPtr<UsdStage>`, returns `pxr/base/tf/refPtr.h`,
// given `shared_ptr<HdTask>`, returns a stl header for `<memory.h>`.
// Compare with latestDeclLocFilePath.
std::string ASTHelpers::earliestDeclLocFilePath(const clang::Decl* decl) const {
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
std::string ASTHelpers::latestDeclLocFilePath(const clang::Decl* decl) const {
    if (!decl) { return ""; }
    
    const clang::SourceManager& sourceManager = decl->getASTContext().getSourceManager();
    clang::SourceLocation loc = ASTHelpers::getLatestSourceLocation(decl);
    std::string filePath = sourceManager.getFilename(loc).str();
    
    return filePath;
}

std::string ASTHelpers::stmtFilePath(const clang::Stmt* stmt) const {
    if (!stmt) { return ""; }
    
    const clang::SourceManager& sourceManager = *getASTAnalysisRunner().getSourceManager();
    clang::SourceLocation loc = stmt->getBeginLoc();
    loc = sourceManager.getExpansionLoc(loc);
    std::string filePath = sourceManager.getFilename(loc).str();
    
    return filePath;
}

unsigned ASTHelpers::expansionLineNumber(const clang::Decl* decl) const {
    if (!decl) { return 0; }
    
    const clang::SourceManager& sourceManager = decl->getASTContext().getSourceManager();
    clang::SourceLocation loc = ASTHelpers::getLatestSourceLocation(decl);
    unsigned lineNumber = sourceManager.getExpansionLineNumber(loc);
    
    return lineNumber;
}

unsigned ASTHelpers::expansionLineNumber(const clang::Stmt* stmt) const {
    if (!stmt) { return 0; }
    
    const clang::SourceManager& sourceManager = *getASTAnalysisRunner().getSourceManager();
    clang::SourceLocation loc = stmt->getBeginLoc();
    unsigned lineNumber = sourceManager.getExpansionLineNumber(loc);
    
    return lineNumber;
}


std::string ASTHelpers::headerPathForUsdType(const clang::Decl* decl) const {
    return makePathRelativeForUsd(latestDeclLocFilePath(decl));
}

std::string ASTHelpers::makePathIncludeForUsd(const clang::Decl* decl) const {
    return makePathIncludeForUsd(latestDeclLocFilePath(decl));
}

bool ASTHelpers::isEarliestDeclLocFromUsd(const clang::Decl* decl) const {
    return makePathRelativeForUsd(earliestDeclLocFilePath(decl)) != "";
}

std::string ASTHelpers::getUsdLibraryForDecl(const clang::Decl* decl) const {
    std::string relative = makePathRelativeForUsd(latestDeclLocFilePath(decl));
    if (relative == "") {
        return "";
    }
    
    // pxr/foo/result/bar.h
    std::vector<std::string> components = splitStringByRegex(relative, std::regex("/"));
    if (components.size() < 3) { return ""; }
    return components[2];
}

bool ASTHelpers::isFromUsdLibrary(const clang::Decl* decl, std::string lib) const {
    return lib == getUsdLibraryForDecl(decl);
}

bool ASTHelpers::isFromUsdLibraryStrictlyBefore(const clang::Decl* decl, std::string lib) const {
    std::string declLib = getUsdLibraryForDecl(decl);
    
    if (lib == declLib) {
        // Same library isn't strictly before
        return false;
    }
    
    std::vector<std::string> allLibs = getASTAnalysisRunner().getDriver()->getCMakeParser()->getNamesOfPxrLibraries();
    for (const auto& someLib : allLibs) {
        if (someLib == declLib) {
            // Got to the decl's lib before the argument lib
            return true;
        }
        if (someLib == lib) {
            // Got to the argument lib before the decl's lib
            return false;
        }
    }
    
    std::cout << "WARNING: '" << lib << "' not contained in CMakeParser.getNamesOfPxrLibraries()" << std::endl;
    return false;
}

bool ASTHelpers::doesTypeContainUsdTypes(const clang::TypeDecl* typeDecl) const {
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

bool ASTHelpers::areAllUsdDeclsFromPublicHeaders(const clang::Decl* decl) const {
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
const clang::TagDecl* ASTHelpers::findTagDecl(const std::string& typeName) const {
    return getASTAnalysisRunner().findTagDecl(typeName);
}

// Finds a NamedDecl with the given name, querying the FindNamedDeclsAnalysisPass
const clang::NamedDecl* ASTHelpers::findNamedDecl(const std::string& name) const {
    return getASTAnalysisRunner().findNamedDecl(name);
}

// Finds a Type with the given name, querying the FindNamedDeclsAnalysisPass
const clang::Type* ASTHelpers::findType(const std::string& name) const {
    return getASTAnalysisRunner().findType(name);
}

// Finds a FunctionDecl with the given name, querying the FindNamedDeclsAnalysisPass
const clang::FunctionDecl* ASTHelpers::findFunctionDecl(const std::string& signature) const {
    return getASTAnalysisRunner().findFunctionDecl(signature);
}
