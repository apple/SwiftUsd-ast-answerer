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

#include "SwiftSubclassCxxCodeGenHelpers.h"

// MARK: TypesHelper

TypesHelper::TypesHelper(const SwiftSubclassCxxCodeGen* codeGen) : _codeGen(codeGen) {}

/* static */ bool TypesHelper::isConstRef(clang::QualType q) {
    if (q == clang::QualType()) { return false; }
    if (!q->isLValueReferenceType()) { return false; }
    if (!q.getNonReferenceType().isConstQualified()) { return false; }
    return true;
}

/* static */ clang::QualType TypesHelper::removingConstRefIfPossible(clang::QualType q) {
    return ASTHelpers::removingRefConst(q);
}

/* static */ clang::QualType TypesHelper::convertingConstRefToConstStar(clang::QualType q, const clang::FunctionDecl* f) {
    clang::QualType orig = q;
    q = ASTHelpers::removingRefConst(q);
    if (orig == q) { return orig; }
    return f->getASTContext().getPointerType(q.withConst());
}

bool TypesHelper::isValueTypeIndirection(clang::QualType q) const {
    return _removeValueTypeIndirectionIfPossible(q);
}

clang::QualType TypesHelper::removingValueTypeIndirectionIfPossible(clang::QualType q) const {
    _removeValueTypeIndirectionIfPossible(q);
    return q;
}

bool TypesHelper::doesMethodGetSwiftReturnIndirectionAllocation(const clang::CXXMethodDecl* method) const {
    if (method->getAccess() == clang::AS_private) { return false; }
    if (!method->isVirtual()) { return false; }
    return isValueTypeIndirection(method->getReturnType());
}

clang::QualType TypesHelper::getSwiftReturnIndirectionAllocationType(const clang::CXXMethodDecl* method) const {
    if (!doesMethodGetSwiftReturnIndirectionAllocation(method)) { return method->getReturnType(); }
    clang::QualType q = method->getReturnType();
    
#warning this might be wrong, do we want allocation for all indirection, including (C++ double, Swift single + implicit) indirection to FRTs?
    q = removingValueTypeIndirectionIfPossible(q);
    // Swift allocates an UnsafeMutablePointer to the value, regardless of its kind of value type indirection
    q.removeLocalConst();
    q = method->getASTContext().getPointerType(q);
    return q;
}
std::string TypesHelper::convertSwiftReturnIndirectionAllocationCxxFpToCxxVirtual(const clang::CXXMethodDecl* method, std::string expr) const {
    clang::QualType desiredRet = method->getReturnType();
    clang::QualType currentExpr = getSwiftReturnIndirectionAllocationType(method);
    
    // currentExpr is always non-const pointer
    // desiredRet is either const or non-const and pointer or ref
    // we can implicitly cast non-const to const as needed, so we just need to deal with pointer or ref
    if (desiredRet->isLValueReferenceType()) {
        return "*"+expr;
    } else {
        return expr;
    }
}

bool TypesHelper::isImplicitlyIndirectFRT(clang::QualType q, bool* isShared) const {
    if (q->isLValueReferenceType()) {
        q = q.getNonReferenceType();
    } else if (q->isPointerType()) {
        q = q->getPointeeType();
    } else {
        return false;
    }
    
    if (q.isConstQualified()) {
        q.removeLocalConst();
    }

    const clang::CXXRecordDecl* cxxRecord = q->getAsCXXRecordDecl();
    if (!cxxRecord) { return false; }
    const auto& it = _codeGen->getImportAnalysisPass()->find(cxxRecord);
    if (it == _codeGen->getImportAnalysisPass()->end()) { return false; }
    if (isShared) {
        *isShared = it->second.isImportedAsSharedReference();
    }
    return it->second.isImportedAsAnyReference();
}

bool TypesHelper::_removeValueTypeIndirectionIfPossible(clang::QualType &q) const {
    clang::QualType original = q;
    if (q == clang::QualType()) {
        q = original;
        return false;
    }
    
    // Value type indirections are either pointers or references to value types
    if (q->isLValueReferenceType()) {
        q = q.getNonReferenceType();
    } else if (q->isPointerType()) {
        q = q->getPointeeType();
    } else {
        q = original;
        return false;
    }
    
    if (q->isFundamentalType()) {
        // Fundamental types are void, std::nullptr_t, and arithmetic types
        // (bool, char, signed int, unsigned int, float, etc),
        // which are always value types
        return true;
    }
    
    // Compound types can be references, pointers, pointer-to-member, array, function, enum, and class types.
    // Enum and class have tags, but the rest don't, and they're all imported as values. (Probably?)
    const clang::TagDecl* tagDecl = q->getAsTagDecl();
    if (!tagDecl) {
        return true;
    }
    
    const auto& it = _codeGen->getImportAnalysisPass()->find(tagDecl);
    if (it == _codeGen->getImportAnalysisPass()->end()) {
        std::cerr << "Error! Unable to determine value type indirection of " << original.getAsString() << ", no import for " << ASTHelpers::getAsString(tagDecl) << std::endl;
        __builtin_trap();
    }
    
    bool result = it->second.isImportedAsValue();
    if (!result) {
        q = original;
    }
    return result;
}

// MARK: NamesHelper

NamesHelper::TypeNames::TypeNames(SwiftSubclassCxxCodeGen* codeGen) : _codeGen(codeGen) {}

std::string NamesHelper::TypeNames::overlaySwiftEnum = "__OverlaySwift";

std::string NamesHelper::TypeNames::fieldCppNameInCpp(const clang::FieldDecl* field, bool& succeeded) const {
    auto printer = _codeGen->typeNamePrinter(field->getType());
    std::optional<std::string> result = _codeGen->getTypeNameOpt<CppNameInCpp>(printer);
    succeeded = result.has_value();
    return result.value_or("$ERR$fieldCppNameInCpp$"+field->getType().getAsString()+"`$");
}

std::string NamesHelper::TypeNames::fieldSwiftNameInSwift(const clang::FieldDecl* field, bool& succeeded) const {
    auto printer = _codeGen->typeNamePrinter(field->getType());
    std::optional<std::string> result = _codeGen->getTypeNameOpt<SwiftNameInSwift>(printer);
    succeeded = result.has_value();
    return result.value_or("$ERR$fieldSwiftNameInSwift$"+field->getType().getAsString()+"`$");
}


std::string NamesHelper::FieldNames::returnIndirectionAllocation(const clang::CXXMethodDecl *method) const {
    return "__returnIndirectionAllocation_" + method->getNameAsString();
}

std::string NamesHelper::MethodNames::fieldGetter(const clang::FieldDecl *f) const {
    return "__" + f->getNameAsString() + "_get()";
}

std::string NamesHelper::MethodNames::fieldSetter(const clang::FieldDecl *f) const {
    return "__" + f->getNameAsString() + "_set()";
}

std::string NamesHelper::MethodNames::fieldGetterSwiftReturnsUnretainedIfNeeded(const clang::FieldDecl* f, const SwiftSubclassCxxCodeGen* codeGen) const {
    bool isShared = false;
    TypesHelper(codeGen).isImplicitlyIndirectFRT(f->getType(), &isShared);
    if (isShared) {
        return " SWIFT_RETURNS_UNRETAINED";
    } else {
        return "";
    }
}

NamesHelper::NamesHelper(SwiftSubclassCxxCodeGen* codeGen,
                         const clang::TagDecl* tagDecl) :
types(codeGen), _codeGen(codeGen),
_cxxRecordDecl(clang::dyn_cast<clang::CXXRecordDecl>(tagDecl)) {
    _analysisResult = _codeGen->getSwiftSubclassCxxAnalysisPass()->find(tagDecl)->second;
    
    // tagSwiftNameInCpp, tagSwiftNameInSwift
    {
        auto printer = _codeGen->typeNamePrinter(tagDecl);
        types.tagSwiftNameInCpp = codeGen->getTypeName<SwiftNameInCpp>(printer);
        types.tagSwiftNameInSwift = codeGen->getTypeName<SwiftNameInSwift>(printer);
    }
    
    // fullyQualifiedCxxAdapter
    {
        std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
        components[0] = "__Overlay";
        components.push_back(types.cxxAdapter);
        types.fullyQualifiedCxxAdapter = joinQualifiedNameComponents(components, "::");
    }
    
    // swiftFullyQualifiedCxxAdapter
    {
        std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
        components[0] = "__Overlay";
        components.push_back(types.cxxAdapter);
        types.swiftFullyQualifiedCxxAdapter = joinQualifiedNameComponents(components, ".");
    }
            
    // swiftProtocolCompositionTypealias
    {
        std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
        components[0] = "Overlay";
        components.back() += "Subclass";
        types.swiftProtocolCompositionTypealias = joinQualifiedNameComponents(components, ".");
    }
    
    // fullyQualifiedSwiftAdapter
    {
        std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
        components[0] = "__OverlaySwift";
        components.push_back(types.swiftAdapter);
        types.fullyQualifiedSwiftAdapter = joinQualifiedNameComponents(components, ".");
    }
    
    // fullyQualifiedPureVirtuals
    {
        types.fullyQualifiedPureVirtuals = types.fullyQualifiedSwiftAdapter + ".PureVirtuals";
    }
    
    // unmanagedSwiftAdapter
    {
        types.unmanagedSwiftAdapter = "Unmanaged<" + types.fullyQualifiedSwiftAdapter + ">";
    }
    
    // basesSwiftNameInCpp, basesSwiftNameInSwift
    {
        for (const auto& pair : AnalysisHelper(codeGen, tagDecl).baseAndFutureInheritancePairs) {
            auto printer = codeGen->typeNamePrinter(pair.first);
            types.basesSwiftNameInCpp[pair.first] = codeGen->getTypeName<SwiftNameInCpp>(printer);
            types.basesSwiftNameInSwift[pair.first] = codeGen->getTypeName<SwiftNameInSwift>(printer);
        }
    }
}

/* static */ std::vector<std::string> NamesHelper::computeQualifiedNameComponents(const clang::TagDecl* tagDecl) {
    std::vector<std::string> result;
    const clang::NamedDecl* namedDecl = tagDecl;
    while (namedDecl) {
        result.insert(result.begin(), namedDecl->getNameAsString());
        namedDecl = clang::dyn_cast<clang::NamedDecl>(namedDecl->getDeclContext());
    }
    if (!result.empty() && result[0] == PXR_NS) {
        result[0] = "pxr";
    }
    
    return result;
}
/* static */ std::string NamesHelper::joinQualifiedNameComponents(std::vector<std::string> components, std::string separator) {
    std::stringstream ss;
    for (size_t i = 0; i < components.size(); i++) {
        ss << components[i];
        if (i + 1 < components.size()) {
            ss << separator;
        }
    }
    return ss.str();
}

// MARK: AnalysisHelper

AnalysisHelper::AnalysisHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::TagDecl* tagDecl)  :
_codeGen(codeGen), _cxxRecordDecl(clang::dyn_cast<clang::CXXRecordDecl>(tagDecl))  {
    _analysisResult = _codeGen->getSwiftSubclassCxxAnalysisPass()->find(tagDecl)->second;
    // We generally don't want to introspect TfRefBase/TfWeakBase for subclassing, because
    // they're core mixin classes that users shouldn't mess with
    const clang::TagDecl* tfRefBase = _codeGen->getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfRefBase");
    const clang::TagDecl* tfWeakBase = _codeGen->getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfWeakBase");
    
    // conditionalDowncastingTargets
    for (const auto& pair : _analysisResult.bases) {
        if (pair.first != clang::AS_private) {
            if (pair.second != tfRefBase && pair.second != tfWeakBase) {
                conditionalDowncastingTargets.push_back(pair.second);
            }
        }
    }
    conditionalDowncastingTargets.push_back(_cxxRecordDecl);
    
    // unconditionalUpcastingTargets
    unconditionalUpcastingTargets = conditionalDowncastingTargets;
    
    // baseAndFutureInheritancePairs
    {
        // Build up a list of non-private bases, including the cxxRecordDecl that Swift will "subclass"
        std::vector<const clang::CXXRecordDecl*> temp;
        for (const auto& it : _analysisResult.bases) {
            if (it.first == clang::AS_private) { continue; }
            if (it.second == tfRefBase || it.second == tfWeakBase) { continue; }
            temp.push_back(it.second);
        }
        temp.push_back(_cxxRecordDecl);
        
        // Add pairs of (`x`, every record after `x`)
        for (auto it = temp.begin(); it != temp.end(); it++) {
            std::vector<const clang::CXXRecordDecl*> future;
            future.resize(std::distance(it + 1, temp.end()));
            std::copy(it + 1, temp.end(), future.begin());
            baseAndFutureInheritancePairs.push_back({ *it, future });
        }
    }
    
    destructorIsPublic = _analysisResult.destructor->getAccess() == clang::AS_public;
    destructorIsProtected = _analysisResult.destructor->getAccess() == clang::AS_private;
    destructorIsNotPrivate = _analysisResult.destructor->getAccess() != clang::AS_private;
    constructors = _analysisResult.constructors;
    
    // isPluginEntryPoint
    {
        isPluginEntryPoint = false;
        if (tagDecl == _codeGen->getImportAnalysisPass()->findTagDecl("class " PXR_NS"::HioImage")) {
            isPluginEntryPoint = true;
        }
        if (tagDecl == _codeGen->getImportAnalysisPass()->findTagDecl("class " PXR_NS"::SdfFileFormat")) {
            isPluginEntryPoint = true;
        }
    }
}

std::vector<const clang::FieldDecl*> AnalysisHelper::fieldsForInheritance(const clang::CXXRecordDecl* x) {
    std::vector<const clang::FieldDecl*> result;
    
    const auto& it = _codeGen->getSwiftSubclassCxxAnalysisPass()->find(x);
    if (it == _codeGen->getSwiftSubclassCxxAnalysisPass()->end()) {
        std::cerr << "Error! No analysis for record '" << ASTHelpers::getAsString(x) << "'" << std::endl;
        __builtin_trap();
    }
    
    for (const clang::FieldDecl* f : it->second.fields) {
        if (f->getAccess() != clang::AS_private) {
            result.push_back(f);
        }
    }
    return result;
}

std::vector<const clang::CXXMethodDecl*> AnalysisHelper::methodsForInheritance(const clang::CXXRecordDecl* current, std::vector<const clang::CXXRecordDecl*> future) {
    std::vector<const clang::CXXMethodDecl*> result;
    
    const auto& currentAnalysis = _codeGen->getSwiftSubclassCxxAnalysisPass()->find(current);
    if (currentAnalysis == _codeGen->getSwiftSubclassCxxAnalysisPass()->end()) {
        std::cerr << "Error! No analysis for record '" << ASTHelpers::getAsString(current) << "'" << std::endl;
        __builtin_trap();
    }

    
    // It isn't actually clear that this will handle overrides properly?
    // Longer inheritance chains might have overrides present, in which case
    // if this doesn't handle things properly, that'll result in either duplicate
    // methods and/or fields, or missing methods that should be inherited.
    #warning Make sure we're properly handling method overrides
    
    for (const clang::CXXMethodDecl* method : currentAnalysis->second.methods) {
        if (method->size_overridden_methods() == 0) {
            result.push_back(method);
        }
    }
    
    return result;
}

// MARK: MethodHelper

MethodHelper::MethodHelper(SwiftSubclassCxxCodeGen* codeGen,
                           const clang::CXXMethodDecl* method,
                           NamesHelper const& namesHelper) :
hadErrorPrintingTypeNames(false), _codeGen(codeGen), _method(method), _namesHelper(namesHelper) {
    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
        clang::FunctionTemplateDecl* functionTemplate = method->getDescribedFunctionTemplate();
        clang::TemplateParameterList* templateParameterList = functionTemplate->getTemplateParameters();
        templateS = "template <";
        for (size_t i = 0; i < templateParameterList->size(); i++) {
            templateS += ASTHelpers::getAsString(templateParameterList->asArray()[i]);
            if (i + 1 < templateParameterList->size()) {
                templateS += ", ";
            } else {
                templateS += ">";
            }
        }
    }
    
    functionPointerS = "__" + method->getNameAsString() + "_FP";
    
    // swiftSharedFRTReturnConvention
    {
        clang::QualType q = method->getReturnType();
        bool isShared = false;
        TypesHelper(codeGen).isImplicitlyIndirectFRT(q, &isShared);
        if (isShared) {
            if (method->getQualifiedNameAsString() == PXR_NS"::SdfFileFormat::_InstantiateNewLayer") {
                swiftSharedFRTReturnConvention = " SWIFT_RETURNS_RETAINED";
            } else {
                std::cerr << "Error! Shared FRT returning method requires manually-determined annotation: " << ASTHelpers::getAsString(method) << std::endl;
                std::cerr << method->getQualifiedNameAsString() << std::endl;
                hadErrorPrintingTypeNames = true;
                swiftSharedFRTReturnConvention = "$ERR$SwiftSharedFRTReturnConvention$";
            }
        } else {
            swiftSharedFRTReturnConvention = "";
        }
    }
    
    // swiftReturnIndirectionAllocationType, swiftReturnIndirectionAllocationTypeNonOptional, swiftReturnIndirectionExpressionConversion
    {
        TypesHelper th(codeGen);
        if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
            auto printer = codeGen->typeNamePrinter(th.getSwiftReturnIndirectionAllocationType(method));
            std::optional<std::string> x = codeGen->getTypeNameOpt<SwiftNameInSwift>(printer);
            if (x) {
                swiftReturnIndirectionAllocationType = *x;
            } else {
                swiftReturnIndirectionAllocationType = "$ERR$swiftReturnIndirectionAllocationType$";
                hadErrorPrintingTypeNames = true;
            }
            
            swiftReturnIndirectionAllocationTypeNonOptional = swiftReturnIndirectionAllocationType.substr(0, swiftReturnIndirectionAllocationType.size() - 1);
        }
    }
    
    // Useful for debugging changes to SwiftSubclassCxx codegen
    bool USE_LABELS = false;
    auto label = [=](std::string printedSignature, std::string label) -> std::string {
        if (USE_LABELS) {
            return printedSignature + " /* " + label + " */";
        } else {
            return printedSignature;
        }
    };
    
    // signature_cppOriginalDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        signature_cppOriginalDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration), "cppOriginalDeclaration");
    }
    
    // signature_cppOriginalDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.indexParams();
        signature_cppOriginalDefinition = label(abstractSignature.printCpp(AbstractSignature::definition), "cppOriginalDefinition");
    }
    
    // signature_cppOriginalBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.indexParams();
        abstractSignature.addSubclassParameter();
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.setPointersAsNeededMode(AbstractSignature::transformationAdd);
        abstractSignature.callFP();
        signature_cppOriginalBody = label(abstractSignature.printCpp(AbstractSignature::body), "cppOriginalBody");
    }
    
    // signature_cppForwardDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.makeForward();
        signature_cppForwardDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration) + swiftSharedFRTReturnConvention, "cppForwardDeclaration");
    }
    
    // signature_cppForwardDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.indexParams();
        abstractSignature.makeForward();
        signature_cppForwardDefinition = label(abstractSignature.printCpp(AbstractSignature::definition) + swiftSharedFRTReturnConvention, "cppForwardDefinition");
    }
    
    // signature_cppForwardBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.indexParams();
        abstractSignature.callOriginal();
        signature_cppForwardBody = label(abstractSignature.printCpp(AbstractSignature::body), "cppForwardBody");
    }
    
    // signature_cppDefaultDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.makeDefault();
        signature_cppDefaultDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration) + swiftSharedFRTReturnConvention, "cppDefaultDeclaration");
    }

    // signature_cppDefaultDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.indexParams();
        abstractSignature.makeDefault();
        signature_cppDefaultDefinition = label(abstractSignature.printCpp(AbstractSignature::definition) + swiftSharedFRTReturnConvention, "cppDefaultDefinition");
    }
    
    // signature_cppDefaultBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.indexParams();
        abstractSignature.callBase();
        signature_cppDefaultBody = label(abstractSignature.printCpp(AbstractSignature::body), "cppDefaultBody");
    }
    
    // signature_cppFunctionPointerDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.addSubclassParameter();
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.setPointersAsNeededMode(AbstractSignature::transformationAdd);
        signature_cppFunctionPointerDeclaration = label(abstractSignature.printCpp(AbstractSignature::functionPointer), "cppFunctionPointerDefinition");
    }
    
    // signature_swiftFunctionPointerBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.closureIndexParams();
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.setPointersAsNeededMode(AbstractSignature::transformationRemove);
        signature_swiftFunctionPointerBody = label(abstractSignature.printSwift(AbstractSignature::body), "swiftFunctionPointerBody");
    }

    // signature_swiftFunctionDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.removeConstRefAsNeeded();
        signature_swiftFunctionDeclaration = label(abstractSignature.printSwift(AbstractSignature::declaration), "swiftFunctionDeclaration");
    }
    
    // signature_swiftFunctionBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.removeConstRefAsNeeded();
        abstractSignature.callForwardOrDefault();
        signature_swiftFunctionBody = label(abstractSignature.printSwift(AbstractSignature::body), "swiftFunctionBody");
    }
    
    // signature_cppConstructorDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        signature_cppConstructorDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration), "cppConstructorDeclaration");
    }
    
    // signature_cppConstructorDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        abstractSignature.indexParams();
        signature_cppConstructorDefinition = label(abstractSignature.printCpp(AbstractSignature::definition), "cppConstructorDefinition");
    }
    
    // signature_cppConstructorBodyInitializerList
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.indexParams();
        signature_cppConstructorBodyInitializerList = label(abstractSignature.printCpp(AbstractSignature::body), "cppConstructorBodyInitializerList");
    }
    
    // signature_cppSwiftNewDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.makeCppNewOrSwiftInit();
        signature_cppSwiftNewDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration), "cppSwiftNewDeclaration");
    }
    
    // signature_cppSwiftNewDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.indexParams();
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.makeCppNewOrSwiftInit();
        signature_cppSwiftNewDefinition = label(abstractSignature.printCpp(AbstractSignature::definition), "cppSwiftNewDefinition");
    }
    
    // signature_cppSwiftNewBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.indexParams();
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationRemove);
        abstractSignature.makeCppNewOrSwiftInit();
        signature_cppSwiftNewBody = label(abstractSignature.printCpp(AbstractSignature::body), "cppSwiftNewBody");
    }
    
    // signature_swiftInitDefinition
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.makeCppNewOrSwiftInit();
        abstractSignature.removeConstRefAsNeeded();
        signature_swiftInitDefinition = label(abstractSignature.printSwift(AbstractSignature::definition), "swiftInitDefinition");
    }
    
    // signature_swiftInitBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.makeCppNewOrSwiftInit();
        signature_swiftInitBody = label(abstractSignature.printSwift(AbstractSignature::body), "swiftInitBody");
    }
    
    // signature_cppTemplateDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        signature_cppTemplateDeclaration = label(abstractSignature.printCpp(AbstractSignature::declaration), "cppTemplateDeclaration");
    }
    
    // signature_cppTemplateBody
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationAdd);
        signature_cppTemplateBody = label(abstractSignature.printCpp(AbstractSignature::body), "cppTemplateBody");
    }

    // signature_swiftProtocolRequirementDeclaration
    {
        AbstractSignature abstractSignature{codeGen, method, _namesHelper};
        abstractSignature.setWrappedTypesMode(AbstractSignature::transformationUse);
        abstractSignature.removeConstRefAsNeeded();
        signature_swiftProtocolRequirementDeclaration = label(abstractSignature.printSwift(AbstractSignature::declaration), "swiftProtocolRequirementDeclaration");
    }

    
    if (hadErrorPrintingTypeNames) {
        std::cerr << "Error! Had error printing type names for " << ASTHelpers::getAsString(method) << std::endl;
    }
}

std::string MethodHelper::swiftReturnIndirectionExpressionConversion(std::string expr) const {
    clang::QualType originalReturnType = _method->getReturnType();
    clang::QualType typeWithoutIndirection = originalReturnType;
    if (typeWithoutIndirection->isLValueReferenceType()) {
        typeWithoutIndirection = typeWithoutIndirection.getNonReferenceType();
    } else if (typeWithoutIndirection->isPointerType()) {
        typeWithoutIndirection = typeWithoutIndirection->getPointeeType();
    }
    
    bool needsForceUnwrap = originalReturnType->isLValueReferenceType();
    bool needsAddedConst = typeWithoutIndirection.isConstQualified();
    
    std::string result = expr;
    if (needsForceUnwrap) {
        result = result + "!";
    }
    if (needsAddedConst) {
        result = "UnsafePointer(" + result + ")";
    }
    return result;
}

MethodHelper::AbstractSignature::AbstractSignature(SwiftSubclassCxxCodeGen* codeGen,
                                                   const clang::CXXMethodDecl* method,
                                                   NamesHelper const& namesHelper) :
_currentReturnType(CurrentType::withQualType(method->getReturnType(), {codeGen})),
_currentFunctionName(method->getNameAsString()),
_currentParameters({}),
_originalReturnType(method->getReturnType()),
_originalFunctionName(method->getNameAsString()),
_originalParameters({}),
_codeGen(codeGen), _method(method), _namesHelper(namesHelper) {
    _currentReturnType.isReturnValue = true;
    
    for (size_t i = 0; i < method->param_size(); i++) {
        std::string parmName = method->parameters()[i]->getNameAsString();
        if (parmName.empty()) {
            // The definition of the method might omit parameter names that aren't used even
            // though the declaration had them. So, search through the methods in the record
            // this tag belongs to for a method whose definition is this method,
            // and then try to use its parameter names
            for (const auto& someMethod : method->getParent()->methods()) {
                if (someMethod->getDefinition() == method) {
                    parmName = someMethod->parameters()[i]->getNameAsString();
                    if (!parmName.empty()) {
                        break;
                    }
                }
            }
        }
        
        if (parmName.empty()) {
            // The parameter name is still empty, but we need some fallback.
            parmName = "arg" + std::to_string(i);
        }
        
        _currentParameters.push_back({parmName, CurrentType::withQualType(method->parameters()[i]->getType(), {codeGen})});
        _originalParameters.push_back({parmName, method->parameters()[i]->getType()});
    }
}

std::string MethodHelper::AbstractSignature::print(CurrentType x, bool inCpp) const {
    if (!x.customType.empty()) {
        std::string separator = inCpp ? "::" : ".";
        std::string joined = NamesHelper::joinQualifiedNameComponents(x.customType, separator);
            
        // Note: We're hard-coding custom noncopyables as always borrowing for now.
        // This might need to be revisited in the future
        if (x.isCustomTypeCopyable) {
            return joined;
        } else if (inCpp) {
            if (x.pointersAsNeededMode == transformationAdd) {
                return joined + " const *";
            } else {
                return joined + " const &";
            }
        } else {
            return "borrowing " + joined;
        }
    }
    
    auto printer = _codeGen->typeNamePrinter(x.qualType);
    std::optional<std::string> result;
    if (inCpp) {
        result = _codeGen->getTypeNameOpt<CppNameInCpp>(printer);
    } else {
        result = _codeGen->getTypeNameOpt<SwiftNameInSwift>(printer);
    }
    
    return result.value_or("$ERR$printCurrentType$");
}

template <typename Transform>
void MethodHelper::AbstractSignature::transformTypes(Transform transform) {
    for (auto& it : _currentParameters) {
        transform(it.second);
    }
    transform(_currentReturnType);
}

template <typename Transform>
void MethodHelper::AbstractSignature::transformLabels(Transform transform) {
    for (size_t i = 0; i < _currentParameters.size(); i++) {
        transform(i, _currentParameters[i].first);
    }
}


void MethodHelper::AbstractSignature::indexParams() {
    transformLabels([](size_t i, std::string& x){
        x = "arg" + std::to_string(i);
    });
}

void MethodHelper::AbstractSignature::closureIndexParams() {
    transformLabels([](size_t i, std::string& x){
        x = "$" + std::to_string(i + 1);
    });
    _currentFunctionName = "slf($0)." + _originalFunctionName;
}

void MethodHelper::AbstractSignature::addSubclassParameter() {
    _currentParameters.insert(
        _currentParameters.begin(), { "__swiftSubclass",
            CurrentType::withCustom({"SwiftSubclass"}, true, _codeGen)
        }
    );
}

void MethodHelper::AbstractSignature::setWrappedTypesMode(TransformationMode mode) {
    if (mode == transformationNone) { return; }
    
    transformTypes([&](CurrentType& x){
        if (!x.customType.empty()) { return; }
        auto printer = _codeGen->typeNamePrinter(x.qualType);
        std::optional<std::string> name = _codeGen->getTypeNameOpt<CppNameInCpp>(printer);
        if (name == "std::ostream &") {
            x.customType = {"Overlay", "StdOstreamWrapper"};
            x.isCustomTypeCopyable = false;
            x.wrappedTypesMode = mode;
        }
    });
}

void MethodHelper::AbstractSignature::setPointersAsNeededMode(TransformationMode mode) {
    if (mode == transformationNone) { return; }
    
    transformTypes([&](CurrentType& x){
        if (!x.customType.empty()) {
            std::vector<std::string> swiftSubclass = {"SwiftSubclass"};
            if (x.customType == swiftSubclass) {
                // pass
            } else {
                x.pointersAsNeededMode = mode;
            }
            return;
        }
        
        clang::QualType q = x.qualType;
        
        if (q->isLValueReferenceType()) {
            x.pointersAsNeededMode = mode;
            q = q.getNonReferenceType();
            q = _method->getASTContext().getPointerType(q);
            x.qualType = q;
        }
    });
}

void MethodHelper::AbstractSignature::removeConstRefAsNeeded() {
    transformTypes([&](CurrentType& x){
        if (!x.customType.empty()) {
            x.didRemoveConstRefAsNeeded = true;
            return;
        }
        
        clang::QualType q = x.qualType;
        
        if (TypesHelper::isConstRef(q)) {
            x.didRemoveConstRefAsNeeded = true;
            x.qualType = TypesHelper::removingConstRefIfPossible(q);;
        }
    });
}

void MethodHelper::AbstractSignature::makeForward() {
    _currentFunctionName = "__" + _originalFunctionName + "_forward";
}

void MethodHelper::AbstractSignature::makeDefault() {
    _currentFunctionName = "__" + _originalFunctionName + "_default";
}

void MethodHelper::AbstractSignature::makeCppNewOrSwiftInit() {
    _isCppNewOrSwiftInit = true;
}

void MethodHelper::AbstractSignature::callFP() {
    _currentFunctionName = "__" + _originalFunctionName + "_FP";
}

void MethodHelper::AbstractSignature::callOriginal() {
    _currentFunctionName = _originalFunctionName;
}

void MethodHelper::AbstractSignature::callBase() {
    _currentFunctionName = _namesHelper.types.tagSwiftNameInCpp + "::" + _originalFunctionName;
}

void MethodHelper::AbstractSignature::callForwardOrDefault() {
    std::string forwardOrDefaultString;
    if (_method->isVirtual()) {
        forwardOrDefaultString = "default";
    } else {
        forwardOrDefaultString = "forward";
    }
    
    if (_method->isStatic()) {
        _currentFunctionName = _namesHelper.types.swiftFullyQualifiedCxxAdapter + ".__" + _originalFunctionName + "_forward";
    } else {
        _currentFunctionName = "__cxxSubclass.__" + _originalFunctionName + "_" + forwardOrDefaultString;
    }
}

MethodHelper::AbstractSignature::CurrentType MethodHelper::AbstractSignature::CurrentType::withQualType(clang::QualType q, TypesHelper typesHelper) {
    return CurrentType(typesHelper, q, {}, true);
}
MethodHelper::AbstractSignature::CurrentType MethodHelper::AbstractSignature::CurrentType::withCustom(std::vector<std::string> components, bool isCustomTypeCopyable, TypesHelper typesHelper) {
    return CurrentType(typesHelper, {}, components, isCustomTypeCopyable);
}

MethodHelper::AbstractSignature::CurrentType::CurrentType(TypesHelper typesHelper, clang::QualType qualType, std::vector<std::string> customType, bool isCustomTypeCopyable) :
typesHelper(typesHelper), originalQualType(qualType), qualType(qualType), customType(customType), isCustomTypeCopyable(isCustomTypeCopyable) {}

std::string MethodHelper::AbstractSignature::CurrentType::convert(std::string expr, bool inCpp) const {
    std::cout << "Convert start: " << expr << std::endl;
    
    std::string result = expr;

    // StdOstreamWrapper handling
    if (inCpp) {
        std::vector<std::string> overlayStdOstreamWrapper = {"Overlay", "StdOstreamWrapper"};
        if (customType == overlayStdOstreamWrapper) {
            switch (wrappedTypesMode) {
                case transformationAdd: result = "__unsafeGetAddressOfTemporaryForFunctionPointer(Overlay::StdOstreamWrapper(" + result + "))"; break;
                case transformationUse: break;
                case transformationRemove: result = "*" + result + ".get()"; break;
                case transformationNone: break;
            }
        }
    }
    
    // C++ can add pointers to the original signature
    if (inCpp) {
        if (customType.empty() && pointersAsNeededMode == transformationAdd) {
            result = (isReturnValue ? "*" : "&") + result;
        }
    }
    
    if (!inCpp) {
        // Parameters may get a force-unwrap if they were originally non-optional (C++ reference) and they got a pointers-as-needed transformation
        // for C function pointer use
        if (originalQualType->isLValueReferenceType() && (pointersAsNeededMode == transformationRemove) && !isReturnValue) {
            result = result + "!";
        }
    }

    if (!inCpp) {
        // Swift might want to use `.pointee` to dereference pointers
        bool wantsPointee = false;
        if (customType.empty() && !isReturnValue && pointersAsNeededMode == transformationRemove) {
            wantsPointee = true;
        } else if (!customType.empty() && !isCustomTypeCopyable && pointersAsNeededMode == transformationRemove) {
            wantsPointee = true;
        } else if (isReturnValue && didRemoveConstRefAsNeeded) {
            wantsPointee = true;
        }
        // FRTs don't get `.pointee`
        if (wantsPointee && customType.empty() && typesHelper.isImplicitlyIndirectFRT(originalQualType) && (pointersAsNeededMode == transformationNone || pointersAsNeededMode == transformationRemove)) {
            wantsPointee = false;
        }
        
        if (wantsPointee) {
            result = result + ".pointee";
        }
    }
    
    
    if (!inCpp && customType.empty()) {
        // Non-const references become UnsafeMutablePointer<T>, which when passed back to
        // a forwarding method needs &pointer.pointee for inout reconversion
        if (!typesHelper.isConstRef(originalQualType) && originalQualType->isLValueReferenceType()) {
            result = "&" + result + ".pointee";
        }
    }
    
    std::cout << "Convert end: " << result << std::endl << std::endl;
        
    return result;
}

std::string MethodHelper::AbstractSignature::printCpp(PrintingPurpose printingPurpose) const {
    std::stringstream ss;
    // Static indicator
    if (_method->isStatic() || _isCppNewOrSwiftInit) {
        switch (printingPurpose) {
            case declaration: ss << "static "; break;
            case definition: ss << "/* static */ "; break;
            case body: break;
            case functionPointer: break;
        }
    }
    
    // Return type at the front of a signature
    if (!clang::dyn_cast<clang::CXXConstructorDecl>(_method)) {
        switch (printingPurpose) {
            case declaration: ss << print(_currentReturnType, true) << " "; break;
            case definition: ss << print(_currentReturnType, true) << " "; break;
            case body: break;
            case functionPointer: ss << print(_currentReturnType, true) << " "; break;
        }
    } else if (_isCppNewOrSwiftInit && printingPurpose != body) {
        ss << _namesHelper.types.fullyQualifiedCxxAdapter << "*_Nonnull ";
    }
    
    
    // Full qualification of function definition names
    if (printingPurpose == definition) {
        ss << _namesHelper.types.fullyQualifiedCxxAdapter + "::";
    }
    
    // The unqualified name and arguments to this function
    std::stringstream nameAndArgumentList;
    std::string fname = _currentFunctionName;
    if (clang::dyn_cast<clang::CXXConstructorDecl>(_method)) {
        if (printingPurpose == body) {
            fname = _namesHelper.types.tagSwiftNameInCpp;
        } else {
            fname = _namesHelper.types.cxxAdapter;
        }
    }
    
    if (_method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate && printingPurpose == body) {
        std::string tempTemplateS = "<";
        clang::FunctionTemplateDecl* functionTemplate = _method->getDescribedFunctionTemplate();
        clang::TemplateParameterList* templateParameterList = functionTemplate->getTemplateParameters();
        for (size_t i = 0; i < templateParameterList->size(); i++) {
            // Don't use `ASTHelpers::getAsString`, because that'll give `typename T`, but
            // we just want `T` for forwarding in a method call
            tempTemplateS += templateParameterList->asArray()[i]->getNameAsString();
            if (i + 1 < templateParameterList->size()) {
                tempTemplateS += ", ";
            } else {
                tempTemplateS += ">";
            }
        }
        if (_method->isStatic()) {
            fname = _namesHelper.types.tagSwiftNameInCpp+"::"+_originalFunctionName+tempTemplateS;
        } else {
            fname = "this->"+_namesHelper.types.tagSwiftNameInCpp+"::"+_originalFunctionName+tempTemplateS;
        }
    }
    
    if (_isCppNewOrSwiftInit) {
        switch (printingPurpose) {
            case declaration: fname = "__swiftNew"; break;
            case definition: fname = "__swiftNew"; break;
            case body: fname = "new " + _namesHelper.types.cxxAdapter; break;
            case functionPointer: break;
        }
    }
    
    switch (printingPurpose) {
        case declaration: nameAndArgumentList << fname; break;
        case definition: nameAndArgumentList << fname; break;
        case body: nameAndArgumentList << fname; break;
        case functionPointer: nameAndArgumentList << "(*_Nullable __" << fname << "_FP)"; break;
    }
        
    nameAndArgumentList << "(";
    for (size_t i = 0; i < _currentParameters.size(); i++) {
        auto parm = _currentParameters[i];
        switch (printingPurpose) {
            case declaration: /* fallthrough */
            case definition:
                nameAndArgumentList << print(parm.second, true);
                if (!parm.first.empty()) {
                    nameAndArgumentList << " " << parm.first;
                }
                break;

            case body: nameAndArgumentList << parm.second.convert(parm.first, true); break;
                
            case functionPointer: nameAndArgumentList << print(parm.second, true); break;
        }
        
        if (i + 1 < _currentParameters.size()) {
            nameAndArgumentList << ", ";
        }
    }
    nameAndArgumentList << ")";
    
    // Body function call expressions get converted as needed
    if (printingPurpose == body) {
        ss << _currentReturnType.convert(nameAndArgumentList.str(), true);
    } else {
        ss << nameAndArgumentList.str();
    }
    
    // Suffix effects like const, override, and final can be applied
    switch (printingPurpose) {
        case declaration: /* fallthrough */
        case definition:
            if (_method->isConst()) {
                ss << " const";
            }
            break;
        case body: break;
        case functionPointer: break;
    }
    if (printingPurpose == declaration) {
        if (_method->isVirtual() && _originalFunctionName == _currentFunctionName) {
            if (!_method->isPureVirtual()) {
                ss << " override";
            }
            ss << " final";
        }
    }
    
    if (printingPurpose == declaration && _method->getTemplatedKind() == clang::FunctionDecl::TK_NonTemplate && !clang::dyn_cast<clang::CXXConstructorDecl>(_method)) {
        // Apply a SWIFT_NAME to all C++ function declarations, except for templates (rdar://124031460 (SWIFT_NAME doesn't work with templates)),
        // and constructors.
        //
        // The Swift compiler can be inconsistent across compiler versions wrt how/when it renames functions as __Unsafe,
        // and it would be complicated and a bit fragile to update our codegen to follow that. Also, we know how to manage
        // the memory of what we're doing as safely as it can be done.
        // So, just use SWIFT_NAME to suppress any __Unsafe naming, which works if we SWIFT_NAME a function to itself
        ss << " SWIFT_NAME(";
        ss << _currentFunctionName << "(";
        for (size_t i = 0; i < _currentParameters.size(); i++) {
            ss << "_:";
        }
        ss << ")";
        ss << ")";
    }
    
    return ss.str();
}

std::string useBackticksOnSwiftParametersIfNeeded(const std::string& s) {
    // Swift keywords need backticks to be allowed as parameters. We can just
    // hard-code the ones we've encountered so far instead of defensively enumerating
    // all the backticks Swift supports, since this'll be a compile error that stops SwiftUsd-Tests
    // from building. 
    std::vector<std::string> needsBackticks = {"extension"};
    for (const auto& other : needsBackticks) {
        if (s == other) { return "`" + s + "`"; };
    }
    return s;
}

std::string MethodHelper::AbstractSignature::printSwift(PrintingPurpose printingPurpose) const {
    std::stringstream ss;
    
    // Access control
    if (printingPurpose != body) {
        if (_method->isVirtual()) {
            if (_method->isPureVirtual()) {
                // This is a protocol requirement in PureVirtuals,
                // so don't print any access controls on the method decl
            } else {
                ss << "open ";
            }
        } else {
            ss << "public ";
        }
    }
    
    
    if (printingPurpose != body) {
        // Static indicator
        if (_method->isStatic()) {
            ss << "static ";
        }
    }
    
    if (printingPurpose != body && !_isCppNewOrSwiftInit) {
        ss << "func ";
    }
    
    std::stringstream nameAndArgumentList;
    std::string fname = _currentFunctionName;
    if (_isCppNewOrSwiftInit) {
        if (printingPurpose == body) {
            fname = _namesHelper.types.swiftFullyQualifiedCxxAdapter + ".__swiftNew";
        } else {
            fname = "init";
        }
    }
    nameAndArgumentList << fname;
    nameAndArgumentList << "(";
    for (size_t i = 0; i < _currentParameters.size(); i++) {
        auto parm = _currentParameters[i];
        
        if (printingPurpose == body) {
            nameAndArgumentList << parm.second.convert(useBackticksOnSwiftParametersIfNeeded(parm.first), false);
        } else {
            nameAndArgumentList << "_";
            if (!parm.first.empty()) {
                nameAndArgumentList << " " << useBackticksOnSwiftParametersIfNeeded(parm.first);
            }
            nameAndArgumentList << ": ";
            nameAndArgumentList << print(parm.second, false);
        }
        if (i + 1 < _currentParameters.size()) {
            nameAndArgumentList << ", ";
        }
    }
    nameAndArgumentList << ")";
    
    // Body function call expressions get converted as needed
    if (printingPurpose == body) {
        ss << _currentReturnType.convert(nameAndArgumentList.str(), false);
    } else {
        ss << nameAndArgumentList.str();
    }

    if (printingPurpose != body) {
        if (!_isCppNewOrSwiftInit) {
            ss << " -> " << print(_currentReturnType, false);
        }
    }
    
    return ss.str();
}

