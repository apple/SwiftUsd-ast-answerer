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

#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>


ImportAnalysisPass::ImportAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<ImportAnalysisPass, ImportAnalysisResult>(astAnalysisRunner) {}

std::string ImportAnalysisPass::serializationFileName() const {
    return "Import.txt";
}

std::string ImportAnalysisPass::testFileName() const {
    return "testImport.txt";
}

bool ImportAnalysisPass::shouldOnlyVisitDeclsFromUsd() const {
    return false;
}

// Swift 6.1 uses the following rules:
// Class templates: All instantiation arguments must be types, but those types do not need to be importable
// Parent scope: If a type is nested in a decl context, every enclosing decl context must be an importable type.
// Access specifier: If a type is private or protected, it is not imported.
// Copyable: Must have accessible copy ctor or move ctor and an accessible dtor
// Non-copyable: Must have accessible move ctor and an accessible dtor but not an accessible copy ctor
// Reference: Must be marked as reference. (May have inaccessible copy/move ctors and inaccessible dtor)
//
// Note that deriving from a private type does not block a type from being imported.
// And, note that we can do this analysis in a single pass without needing to finalize() anything

bool ImportAnalysisPass::VisitTagDecl(clang::TagDecl *tagDecl) {
    onFindPotentialCandidate(tagDecl);
    return true;
}

bool ImportAnalysisPass::VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl) {
    clang::QualType unsugaredType = typedefNameDecl->getUnderlyingType().getCanonicalType();
    if (const clang::TagDecl* unsugaredTagDecl = unsugaredType->getAsTagDecl()) {
        if (isEarliestDeclLocFromUsd(unsugaredTagDecl) || doesTypeContainUsdTypes(unsugaredTagDecl)) {
            onFindPotentialCandidate(unsugaredTagDecl);
        }
    }

    return true;
}

void ImportAnalysisPass::onFindPotentialCandidate(const clang::TagDecl* tagDecl) {
    if (!tagDecl->isThisDeclarationADefinition()) { return; }
        
    if (find(tagDecl) != end()) {
        return;
    }
        
    if (checkSpecialCaseHandling(tagDecl)) { return; }
    if (checkPublicHeader(tagDecl)) { return; }
    if (checkClassTemplateDecl(tagDecl)) { return; }
    if (checkTemplate(tagDecl)) { return; }
    if (checkParentScope(tagDecl)) { return; }
    if (checkAccessSpecifier(tagDecl)) { return; }
    if (checkSharedReferenceType(tagDecl)) { return; }
    if (checkValueType(tagDecl)) { return; }
    if (checkNonCopyableType(tagDecl)) { return; }
    if (checkImmortalReferenceType(tagDecl)) { return; }
    if (checkMissingMoveCtor(tagDecl)) { return; }
    if (checkMissingDtor(tagDecl)) { return; }
    
    // If we haven't gone into a special case, we don't know how to handle it
    insert_or_assign(tagDecl, ImportAnalysisResult::unknown);
    std::cout << ASTHelpers::getAsString(tagDecl) << std::endl;
    __builtin_trap();
    return;
}

// Special casing, to be avoided whenever possible
bool ImportAnalysisPass::checkSpecialCaseHandling(const clang::TagDecl* tagDecl) {
    std::string typeName = ASTHelpers::getAsString(tagDecl);
    if (typeName.starts_with("class " PXR_NS"::VtDictionary::Iterator")) {
        // VtDictionary::Iterator is templated with a std::__map_iterator as its second argument,
        // which ends up not being imported due to instantiation arguments. But,
        // VtDictionary::Iterator's template arguments are used indirectly,
        // in a way that shouldn't block importing. This is the only known
        // case of a Usd templated type named `Iterator`
        insert_or_assign(tagDecl, ImportAnalysisResult::importedAsValue);
        return true;
    }
    
    return false;
}

// Special case for Usd, since we're processing public headers and source files,
// we need to make sure a type is actually visible to Swift
bool ImportAnalysisPass::checkPublicHeader(const clang::TagDecl* tagDecl) {
#warning Is this correct? It says SdfListOp<SdfPayload> is blockedByNonPublicHeaderDefinition when it should't be
    if (doesTypeContainUsdTypes(tagDecl) && !areAllUsdDeclsFromPublicHeaders(tagDecl)) {
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByNonPublicHeaderDefinition);
        return true;
    }
    return false;
}

// ClassTemplateDecl special case, for Clang internals
bool ImportAnalysisPass::checkClassTemplateDecl(const clang::TagDecl* tagDecl) {
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
    const clang::ClassTemplateDecl* classTemplateDecl = cxxRecordDecl->getDescribedClassTemplate();
    if (!classTemplateDecl) { return false; }
    // The Clang AST represents class templates as a pair of ClassTemplateDecl and CXXRecordDecl,
    // where the former contains template properties, while the latter contains the template's contents.
    // We can use CXXRecordDecl::getDescribedClassTemplate() to detect if a given CXXRecordDecl is part
    // of a ClassTemplateDecl pair. If the described class template is not null,
    // then this record is a "templated record" (my term, not clang's), and not a concrete type.
    // Note that for instantiations of the class template, getDescribedClassTemplate returns null,
    // and the CXXRecordDecl is also a ClassTemplateSpecializationDecl, which is not the case for templated records
    //
    // Since templated records aren't concrete types, we don't want to further analysis passes
    // to be able to use them.
    insert_or_assign(tagDecl, ImportAnalysisResult::blockedByTemplatedRecordDecl);
    return true;
}

bool ImportAnalysisPass::checkTemplate(const clang::TagDecl* tagDecl) {
    const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(tagDecl);
    if (!classTemplateSpecializationDecl) { return false; }
    const clang::TemplateArgumentList& args = classTemplateSpecializationDecl->getTemplateInstantiationArgs();
    for (const clang::TemplateArgument& arg : args.asArray()) {
        if (arg.getKind() == clang::TemplateArgument::Template) {
            // Templates as arguments is hard to deal with,
            // but we need to let TfWeakPtrFacade<TfWeakPtr, T> be importable when T is importable
            // so that TfWeakPtr<T> is Equatable. So this is probably wrong in general,
            // but it's the least worst way of special casing this
            continue;
        }
        
        if (arg.getKind() != clang::TemplateArgument::Type) {
            insert_or_assign(tagDecl, ImportAnalysisResult::blockedByTemplateInstantiationArgs);
            return true;
        }
        if (arg.getAsType().isNull()) {
            insert_or_assign(tagDecl, ImportAnalysisResult::blockedByTemplateInstantiationArgs);
            return true;
        }
        
        const clang::Type* type = arg.getAsType().getTypePtr();
        if (type->isBuiltinType()) {
            continue;
        }
        
        if (type->isPointerType()) {
#warning Is this correct? Swift will import as Unsafe[MRB]Pointer where it can, and OpaquePointer otherwise, right?
            continue;
        }
        
        const clang::TagDecl* argTagDecl = type->getAsTagDecl();
#warning Is this correct? From v25.02a to v25.05.01, this made `struct std::is_array<int[]>` go from `importedAsValue` to `blockedByTemplateInstantiationArgs`
        if (!argTagDecl) {
            insert_or_assign(tagDecl, ImportAnalysisResult::blockedByTemplateInstantiationArgs);
            return true;
        }
        
        // Do an import pass for the template argument,
        // but don't require that it be importable,
        // because Swift 6 supports templated types with non-imported
        // type template arguments when the template is used through a typedef
        onFindPotentialCandidate(argTagDecl);
    }
    
    return false;
}

bool ImportAnalysisPass::checkParentScope(const clang::TagDecl* tagDecl) {
    const clang::DeclContext* parent = tagDecl->getParent();
    if (!parent) { return false; }
    const clang::TagDecl* parentTag = clang::dyn_cast<clang::TagDecl>(parent);
    if (!parentTag) {
        if (clang::dyn_cast<clang::NamespaceDecl>(parent)) {
            return false;
        }
        
        // We're not in a tag or a namespace, so we must be somewhere weird
        // like a type defined in a function body. 
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByParent);
        return true;
    }
    
    onFindPotentialCandidate(parentTag);
    const auto& parentIt = find(parentTag);
    if (parentIt == end()) {
        std::cerr << ASTHelpers::getAsString(tagDecl) << " can't find parent " << ASTHelpers::getAsString(parentTag) << std::endl;
        __builtin_trap();
    }
    
    if (parentIt->second._kind == ImportAnalysisResult::unknown) {
        std::cerr << ASTHelpers::getAsString(tagDecl) << " needs " << ASTHelpers::getAsString(parentTag) << " to finish analysis first" << std::endl;
        __builtin_trap();
    }
    
    if (!parentIt->second.isImportedSomehow()) {
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByParent);
        return true;
    }
    
    return false;
}

bool ImportAnalysisPass::checkAccessSpecifier(const clang::TagDecl* tagDecl) {
    if (ASTHelpers::isNotVisibleToSwift(tagDecl->getAccess())) {
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByAccess);
        return true;
    }
    return false;
}

// Shared reference types (e.g. subclasses of TfRefBase)
bool ImportAnalysisPass::checkSharedReferenceType(const clang::TagDecl* tagDecl) {
    if (const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl)) {
        const clang::CXXRecordDecl* tfRefBase = clang::dyn_cast<clang::CXXRecordDecl>(findTagDecl("class " PXR_NS"::TfRefBase"));
        
        if (ASTHelpers::isEqualToOrDerivedFromClass(cxxRecordDecl, tfRefBase)) {
            // Swift 6.0: Don't import nested types as a reference type, because API notes don't support nested tags
            // Note: This restriction is expected to be lifted in Swift 6.1
#warning Swift 6.1: Support importing nested tags as reference
            const clang::Decl* cxxRecordDeclContext = clang::dyn_cast<clang::Decl>(cxxRecordDecl->getDeclContext());
            const clang::Decl* tfRefBaseContext = clang::dyn_cast<clang::Decl>(tfRefBase->getDeclContext());
            if (cxxRecordDeclContext->getCanonicalDecl() != tfRefBaseContext->getCanonicalDecl()) {
                return false;
            }
            
            // Alright, we can import it as a reference type
            insert_or_assign(tagDecl, ImportAnalysisResult::importedAsSharedReference);
            return true;
        }
    }
    
    return false;
}

bool ImportAnalysisPass::checkValueType(const clang::TagDecl *tagDecl) {
    // Enums are always imported as copyable value types
    if (clang::dyn_cast<clang::EnumDecl>(tagDecl)) {
        insert_or_assign(tagDecl, ImportAnalysisResult::importedAsValue);
        return true;
    }
    
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
            
    if (ASTHelpers::hasSwiftAccessibleCopyCtor(cxxRecordDecl) && ASTHelpers::hasSwiftAccessibleDtor(cxxRecordDecl)) {
        insert_or_assign(tagDecl, ImportAnalysisResult::importedAsValue);
        return true;
    }
    return false;
}

bool ImportAnalysisPass::checkNonCopyableType(const clang::TagDecl *tagDecl) {
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
    
    if (!ASTHelpers::hasSwiftAccessibleCopyCtor(cxxRecordDecl)) {
        if (ASTHelpers::hasSwiftAccessibleMoveCtor(cxxRecordDecl) && ASTHelpers::hasSwiftAccessibleDtor(cxxRecordDecl)) {
            insert_or_assign(tagDecl, ImportAnalysisResult::importedAsNonCopyable);
            return true;
        }
    }
    return false;
}

bool ImportAnalysisPass::checkImmortalReferenceType(const clang::TagDecl *tagDecl) {
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
    
    const clang::ClassTemplateDecl* tfSingleton = clang::dyn_cast<clang::ClassTemplateDecl>(findNamedDecl("template <class T> class " PXR_NS"::TfSingleton"));
    if (!tfSingleton) {
        __builtin_trap();
    }
    
    // Check if tagDecl is the template argument of a specialization of TfSingleton<T>
    bool isTfSingletonSpecialization = false;
    for (const clang::ClassTemplateSpecializationDecl* specialization : tfSingleton->specializations()) {
        // Note: don't require the specialization to be a definition, a declaration is sufficient
        
        const clang::TemplateArgumentList& templateArgs = specialization->getTemplateInstantiationArgs();
        if (templateArgs.size() != 1) {
            __builtin_trap();
        }
        const clang::TemplateArgument& templateArg = templateArgs.asArray()[0];
        if (templateArg.getKind() != clang::TemplateArgument::Type) { continue; }
        
        if (templateArg.getAsType()->getAsTagDecl()->getDefinition() == cxxRecordDecl)  {
            isTfSingletonSpecialization = true;
            break;
        }
    }
    if (!isTfSingletonSpecialization) {
        return false;
    }
    
    // Okay, we have a TfSingleton specialization.
    // Sometimes TfSingleton is specialized with copyable value types.
    // We don't want to import those as reference types, because it is up
    // to the programmer to respect the singleton-ness of the type
    // and not make implicit copies by holding the value by * or &.
    //
    // (Note: the first use of the TfSingleton specialization is sometimes not
    // in a public header (either in a private header or in a .cpp file). That's okay,
    // types often will declare a `static This& GetInstance() const` method, and even
    // if they don't, fixed address types that are privately instantiated with TfSingleton
    // but publicly usable in C++ can still be accessed by using TfSingleton<T>::GetInstance())
    
    if (ASTHelpers::hasSwiftAccessibleCopyCtor(cxxRecordDecl) || ASTHelpers::hasSwiftAccessibleMoveCtor(cxxRecordDecl)) {
        return false;
    } else {
        insert_or_assign(cxxRecordDecl, ImportAnalysisResult::importedAsImmortalReference);
        return true;
    }
}

bool ImportAnalysisPass::checkMissingMoveCtor(const clang::TagDecl *tagDecl) {
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
    
    if (ASTHelpers::hasSwiftAccessibleMoveCtor(cxxRecordDecl)) {
        return false;
    } else {
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByInaccessibleMove);
        return true;
    }
}

bool ImportAnalysisPass::checkMissingDtor(const clang::TagDecl *tagDecl) {
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) { return false; }
    
    if (ASTHelpers::hasSwiftAccessibleDtor(cxxRecordDecl)) {
        return false;
    } else {
        insert_or_assign(tagDecl, ImportAnalysisResult::blockedByInaccessibleDtor);
        return true;
    }
}





