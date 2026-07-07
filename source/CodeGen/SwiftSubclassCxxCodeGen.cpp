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

#include "CodeGen/SwiftSubclassCxxCodeGen.h"
#include "AnalysisPass/SwiftSubclassCxxAnalysisPass.h"

/*
 As of Swift 6.1 to Swift 6.4, Swift-Cxx interop does not support Swift classes
 that subclass C++ types. OpenUSD's plugin mechanism relies on the user subclassing C++
 types, so to allow OpenUSD plugins to be written in Swift, we provide our own
 mechanisms for "faking" subclassing in limited circumstances.

 At a high level, to let Swift subclass the C++ type `T`, we:
 1. Define a C++ class named `CxxAdapter` that derives from `T`. This lets us override virtual
    methods and redirect them to Swift implementations of them.
 2. Define a Swift class named `SwiftAdapter` that works with `CxxAdapter`. `SwiftAdapter` lets
    us create an actual class hierarchy in Swift and use Swift inheritance.
 3. Define a Swift protocol named `PureVirtuals` that contains any pure-virtual methods from `T`.
    This lets us force the user to implement them at compile time, rather than crashing at runtime
    with a "must be overridden" message
 
 Users subclass the `(SwiftAdapter & PureVirtuals)` type composition, and are required to implement the
 requirements of `PureVirtuals`, and can also override any non-pure virtual methods, as well as call
 non-virtual methods, call base class implementations of virtual methods, and access fields, (both public and protected).
 
 
 Memory safety:
 The `CxxAdapter` holds a `SwiftAdapter` by (type-erased) strong pointer. The `SwiftAdapter` holds `CxxAdapter`
 by weak pointer. When the `CxxAdapter` is destroyed, it tells its `SwiftAdapter` and performs a release. This
 gives Swift a chance to set its weak pointer to nil, and lets the Swift class instance know if it is now in a
 "zombie" state. Zombie Swift class instances are Swift class instances that are not leaked, but whose `CxxAdapter`
 has been destroyed. Swift class instances could become zombies if they're stored in an Array and their `CxxAdapter`
 is destroyed, for example.
 We provide the `SWIFTUSD_SWIFT_SUBCLASS_ZOMBIE_CREATION_BEHAVIOR` environment variable that can be used to
 make the system more/less strict about allowing you to create zombies.
 
 `SwiftAdapter` exposes the C++ `new/delete` operations to Swift. When users use `new`, they are expected to use `delete`
 later, or else they'll end up leaking the `CxxAdapter`. (This is a traditional leak, detectable by leak checkers, not a zombie.)

 When the `SwiftAdapter` is destroyed, if it still has a `CxxAdapter`, it safely terminates because the invariant that
 `CxxAdapter` has a strong pointer to `SwiftAdapter` has been violated. This avoids a potential use-after-free. 
 
 When the `CxxAdapter` is destroyed, if it still has a `SwiftAdapter` (it should under almost all circumstances),
 it tells its `SwiftAdapter` that it is going away, so that the `SwiftAdapter` can set its C++ pointer to null.
 This avoids a potential use-after-free
 */

SwiftSubclassCxxCodeGen::SwiftSubclassCxxCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<SwiftSubclassCxxCodeGen>(codeGenRunner) {}

std::string SwiftSubclassCxxCodeGen::fileNamePrefix() const {
    return "SwiftSubclassCxx";
}

SwiftSubclassCxxCodeGen::Data SwiftSubclassCxxCodeGen::preprocess() {
    std::vector<const clang::TagDecl*> result;
    const auto& swiftSubclassCxxData = getSwiftSubclassCxxAnalysisPass()->getData();
    for (const auto& it : swiftSubclassCxxData) {
        result.push_back(clang::dyn_cast<clang::TagDecl>(it.first));
    }
    return result;
}

SwiftSubclassCxxCodeGen::Data SwiftSubclassCxxCodeGen::extraSpecialCaseFiltering(const Data& data) const {
    // For now, we only support subclassing from HioImage,
    // because subclassing is very experimental
    SwiftSubclassCxxCodeGen::Data result;
    for (const auto& it : data) {
        if (ASTHelpers::getAsString(it) != "class " PXR_NS"::HioImage") {
            continue;
        }
        result.push_back(it);
    }
    return result;
}

std::vector<std::string> computeQualifiedNameComponents(const clang::TagDecl* tagDecl) {
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

std::string joinQualifiedNameComponents(std::vector<std::string> components, std::string separator) {
    std::stringstream ss;
    for (size_t i = 0; i < components.size(); i++) {
        ss << components[i];
        if (i + 1 < components.size()) {
            ss << separator;
        }
    }
    return ss.str();
}

std::string computeIndentation(size_t x) {
    std::string result;
    for (size_t i = 0; i < x; i++) {
        result+= "    ";
    }
    return result;
}

struct TypesHelper {
    const SwiftSubclassCxxCodeGen* codeGen;
    
    TypesHelper(const SwiftSubclassCxxCodeGen* codeGen) : codeGen(codeGen) {}
    
    static bool isConstRef(clang::QualType q) {
        return q != clang::QualType() && q->isLValueReferenceType() && q.getNonReferenceType().isConstQualified();
    }
    
    static clang::QualType removingConstRefIfPossible(clang::QualType q) {
        return ASTHelpers::removingRefConst(q);
    }
    
    static clang::QualType convertingConstRefToConstStar(clang::QualType q, const clang::FunctionDecl* f) {
        clang::QualType orig = q;
        q = ASTHelpers::removingRefConst(q);
        if (orig == q) { return orig; }
        return f->getASTContext().getPointerType(q.withConst());
    }
    
    
private:
    static bool _removeValueTypeIndirectionIfPossible(clang::QualType& q, const SwiftSubclassCxxCodeGen* codeGen) {
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
        
        const auto& it = codeGen->getImportAnalysisPass()->find(tagDecl);
        if (it == codeGen->getImportAnalysisPass()->end()) {
            std::cerr << "Error! Unable to determine value type indirection of " << original.getAsString() << ", no import for " << ASTHelpers::getAsString(tagDecl) << std::endl;
            __builtin_trap();
        }
        
        bool result = it->second.isImportedAsValue();
        if (!result) {
            q = original;
        }
        return result;
    }
public:
    
    static bool isValueTypeIndirection(clang::QualType q, const SwiftSubclassCxxCodeGen* codeGen) {
        return _removeValueTypeIndirectionIfPossible(q, codeGen);
    }
    
    static clang::QualType removingValueTypeIndirectionIfPossible(clang::QualType q, const SwiftSubclassCxxCodeGen* codeGen) {
        _removeValueTypeIndirectionIfPossible(q, codeGen);
        return q;
    }
    
    bool doesMethodGetSwiftReturnIndirectionAllocation(const clang::CXXMethodDecl* method) {
        if (method->getAccess() == clang::AS_private) { return false; }
        if (!method->isVirtual()) { return false; }
        return TypesHelper::isValueTypeIndirection(method->getReturnType(), codeGen);
    }
    
    clang::QualType getSwiftReturnIndirectionAllocationType(const clang::CXXMethodDecl* method) {
        if (!doesMethodGetSwiftReturnIndirectionAllocation(method)) { return method->getReturnType(); }
        clang::QualType q = method->getReturnType();
        
        q = removingValueTypeIndirectionIfPossible(q, codeGen);
        // Swift allocates an UnsafeMutablePointer to the value, regardless of its kind of value type indirection
        q.removeLocalConst();
        q = method->getASTContext().getPointerType(q);
        return q;
    }
    
    std::string convertSwiftReturnIndirectionAllocationCxxFpToCxxVirtual(const clang::CXXMethodDecl* method, std::string expr) {
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
};

struct StringsHelper {
    struct TypeNames {
        std::string cxxAdapter = "CxxAdapter";
        std::string fullyQualifiedCxxAdapter;
        
        std::string swiftAdapter = "SwiftAdapter";
        std::string swiftFullyQualifiedCxxAdapter; // fullyQualifiedCxxAdapter for use in Swift
        std::string fullyQualifiedPureVirtuals;
        std::string swiftProtocolCompositionTypealias; // (SwiftAdapter & PureVirtuals) type
        std::string fullyQualifiedSwiftAdapter;
        std::string unmanagedSwiftAdapter; // Unmanaged<fullyQualifiedSwiftAdapter>
        
        static std::string overlaySwiftEnum; // Workaround for rdar://156631442 (Public typealias to Swift type defined in C++ namespace extension not visible across module boundary)
    };
    struct FieldNames {
        std::string swiftSubclassPointer = "__swiftSubclass";
        std::string releaseFP = "__releaseSwiftSubclass_FP";
        std::string cxxSubclassPointer = "__cxxSubclass";
        std::string swiftHasWired = "__swiftHasWired";
        std::string swiftIsZombie = "__swiftIsZombie";
        std::string swiftDidDeinit = "__swiftDidDeinit";
        
        std::string returnIndirectionAllocation(const clang::CXXMethodDecl* method) {
            return "__returnIndirectionAllocation_" + method->getNameAsString();
        }
    };
    struct MethodNames {
        std::string dynamicCast = "__dynamic_cast";
        std::string staticCast = "__static_cast";
        std::string swiftDeleteBase = "__swiftDeleteBase";
        std::string swiftNew = "__swiftNew";
        std::string swiftDeleteCxxAdapter = "__swiftDeleteCxxAdapter";
        std::string wireToCxx = "__wireToCxx";
        
        std::string fieldGetter(const clang::FieldDecl* f) {
            return "__" + f->getNameAsString() + "_get()";
        }
        
        std::string fieldSetter(const clang::FieldDecl* f) {
            return "__" + f->getNameAsString() + "_set()";
        }
    };

    TypeNames typeNames;
    FieldNames fieldNames;
    MethodNames methodNames;
    
    SwiftSubclassCxxCodeGen* codeGen;
    const clang::CXXRecordDecl* cxxRecordDecl;
    SwiftSubclassCxxAnalysisResult analysisResult;
    
    std::vector<const clang::CXXRecordDecl*> conditionalDowncastingTargets;
    std::vector<const clang::CXXRecordDecl*> unconditionalUpcastingTargets;
    using BaseAndFutureInheritancePair = std::pair<const clang::CXXRecordDecl*, std::vector<const clang::CXXRecordDecl*>>;
    std::vector<BaseAndFutureInheritancePair> baseAndFutureInheritancePairs;
    std::string adapterDefinitionCloseFRTAnnotation;
    
    StringsHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::TagDecl* tagDecl,
                  SwiftSubclassCxxAnalysisResult analysisResult)
    : codeGen(codeGen), cxxRecordDecl(clang::dyn_cast<clang::CXXRecordDecl>(tagDecl)),
    analysisResult(analysisResult) {
        // fullyQualifiedCxxAdapter
        {
            std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
            components[0] = "__Overlay";
            components.push_back(typeNames.cxxAdapter);
            typeNames.fullyQualifiedCxxAdapter = joinQualifiedNameComponents(components, "::");
        }
        
        // swiftFullyQualifiedCxxAdapter
        {
            std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
            components[0] = "__Overlay";
            components.push_back(typeNames.cxxAdapter);
            typeNames.swiftFullyQualifiedCxxAdapter = joinQualifiedNameComponents(components, ".");
        }
                
        // swiftProtocolCompositionTypealias
        {
            std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
            components[0] = "Overlay";
            components.back() += "Subclass";
            typeNames.swiftProtocolCompositionTypealias = joinQualifiedNameComponents(components, ".");
        }
        
        // fullyQualifiedSwiftAdapter
        {
            std::vector<std::string> components = computeQualifiedNameComponents(tagDecl);
            components[0] = "__OverlaySwift";
            components.push_back(typeNames.swiftAdapter);
            typeNames.fullyQualifiedSwiftAdapter = joinQualifiedNameComponents(components, ".");
        }
        
        // fullyQualifiedPureVirtuals
        {
            typeNames.fullyQualifiedPureVirtuals = typeNames.fullyQualifiedSwiftAdapter + ".PureVirtuals";
        }
        
        // unmanagedSwiftAdapter
        {
            typeNames.unmanagedSwiftAdapter = "Unmanaged<" + typeNames.fullyQualifiedSwiftAdapter + ">";
        }
        
        // conditionalDowncastingTargets
        for (const auto& pair : analysisResult.bases) {
            if (pair.first != clang::AS_private) {
                conditionalDowncastingTargets.push_back(pair.second);
            }
        }
        conditionalDowncastingTargets.push_back(cxxRecordDecl);
        
        // unconditionalUpcastingTargets
        unconditionalUpcastingTargets = conditionalDowncastingTargets;
        
        // baseAndFutureInheritancePairs
        {
            // Build up a list of non-private bases, including the cxxRecordDecl that Swift will "subclass"
            std::vector<const clang::CXXRecordDecl*> temp;
            for (const auto& it : analysisResult.bases) {
                if (it.first == clang::AS_private) { continue; }
                temp.push_back(it.second);
            }
            temp.push_back(cxxRecordDecl);
            
            // Add pairs of (`x`, every record after `x`)
            for (auto it = temp.begin(); it != temp.end(); it++) {
                std::vector<const clang::CXXRecordDecl*> future;
                std::copy(it + 1, temp.end(), future.begin());
                baseAndFutureInheritancePairs.push_back({ *it, future });
            }
        }
        
        // adapterDefinitionCloseFRTAnnotation
        {
            adapterDefinitionCloseFRTAnnotation = "";
            const auto& it = codeGen->getImportAnalysisPass()->find(tagDecl);
            if (it != codeGen->getImportAnalysisPass()->end()) {
                if (!it->second.isImportedSomehow()) {
                    adapterDefinitionCloseFRTAnnotation = " SWIFT_UNSAFE_REFERENCE";
                }
            }
        }
        
    }
    
    std::vector<const clang::FieldDecl*> fieldsForInheritance(const clang::CXXRecordDecl* x) {
        std::vector<const clang::FieldDecl*> result;
        
        const auto& it = codeGen->getSwiftSubclassCxxAnalysisPass()->find(x);
        if (it == codeGen->getSwiftSubclassCxxAnalysisPass()->end()) {
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
    
    std::vector<const clang::CXXMethodDecl*> methodsForInheritance(const clang::CXXRecordDecl* current, std::vector<const clang::CXXRecordDecl*> future) {
        std::vector<const clang::CXXMethodDecl*> result;
        
        const auto& currentAnalysis = codeGen->getSwiftSubclassCxxAnalysisPass()->find(current);
        if (currentAnalysis == codeGen->getSwiftSubclassCxxAnalysisPass()->end()) {
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
};
std::string StringsHelper::TypeNames::overlaySwiftEnum = "__OverlaySwift";

struct MethodHelper {
    SwiftSubclassCxxCodeGen* codeGen;
    const clang::CXXMethodDecl* method;
    
    std::string templateS; // `template <T1, T2>` prefix as needed
    std::string staticS; // `static` prefix as needed
    std::string staticCommentedS; // The `static` prefix commented out, for use in a .cpp file
    std::string nameS; // The name of the method
    std::string constS; // `const` suffix as needed
    
    std::string functionPointerS; // The name of the function pointer for virtual wiring
    std::string defaultImplementationS; // The name of the exposed default implementation of non-pure virtuals
    std::string forwardS; // The name of the forwarding method
    
    std::string baseNameS; // The name of the default/base invocation, including template arguments and `this->` as needed
    
    std::string cxxDeclareArgumentsS; // Function arguments appropriate for use in a C++ function declaration in a header
    std::string cxxDeclareArgumentsNoDefaultExprsInsertSwiftSubclassPointerS; // Function arguments with a `SwiftSubclass` argument prepended, appropriate for use in a C++ function pointer
    std::string cxxDefineArgumentsIndexedLabelsS; // Function arguments approriate for use in a C++ function definition, using indexing for the parameter labels
    std::string cxxForwardArgumentsOriginalLabelsS; // Function arguments appropriate for calling a C++ function from a C++ file
    std::string cxxForwardArgumentsIndexedLabelsS; // Function arguments appropriate for calling a C++ function from a C++ file, using indexing for the parameter labels
    std::string cxxForwardArgumentsIndexedLabelsInsertSwiftSubclassPointerS; // Function arguments appropriate for calling a C++ function pointer from a C++ file, using indexing for the parameter labels, inserting the Swift subclass pointer at the start

    std::string swiftReturnTypeWithReturnIndirectionAllocationS; // The return type as printed in Swift, taking into consideration value-type indirection unwrapping
    std::string swiftDeclareArgumentsS; // Function arguments appropriate for use in a Swift function definition
    std::string swiftDeclareArgumentsNoDefaultExprsS; // Function arguments appropriate for use in a Swift protocol requirement function declaration
    std::string swiftForwardArgumentsS; // Function arguments appropriate for calling a C++ function from a Swift file
    std::string swiftForwardArgumentsForFunctionPointer; // Function appropriates for caling a Swift function using 1-indexed based anonymous closure arguments
    std::string swiftAccessLevelS; // `open` for virtuals, `public` for non-virtuals
    
    std::string joinCxxDeclare(std::optional<std::vector<SwiftSubclassCxxCodeGen::PrintedFunctionParam>> argsOpt,
                               bool allowDefaultExprs,
                               bool internalNames,
                               bool forFunctionPointer) {
        if (!argsOpt) {
            std::cerr << "Error! Could not get function parameter names and printed types for " << ASTHelpers::getAsString(method) << std::endl;
            __builtin_trap();
        }
        std::stringstream ss;
        for (size_t i = 0; i < argsOpt->size(); i++) {
            auto param = (*argsOpt)[i];
            
            // rdar://156635576 (Runtime crash calling C function pointer with const& argument from C++ when initialized with Swift closure)
            // Workaround: Use const* instead of const& for FP arguments.
            if (forFunctionPointer && TypesHelper::isConstRef(param.qualType)) {
                clang::QualType orig = param.qualType;
                param.qualType = TypesHelper::convertingConstRefToConstStar(param.qualType, method);
                auto printer = codeGen->typeNamePrinter(param.qualType);
                std::optional<std::string> x = codeGen->getTypeNameOpt<CppNameInCpp>(printer);
                if (!x) {
                    std::cerr << "Error! Could not remove ref-const and reprint " << orig.getAsString() << std::endl;
                    __builtin_trap();
                }
                ss << *x;
                
            } else {
                ss << param.printedType;
            }
            if (internalNames) {
                ss << " " << param.internalName;
            } else {
                if (!param.externalName.empty()) {
                    ss << " " << param.externalName;
                }
            }
            if (allowDefaultExprs && param.defaultArg) {
                ss << " = " << *param.defaultArg;
            }
            if (i + 1 < argsOpt->size()) {
                ss << ", ";
            }
        }
        return ss.str();
    }
    
    std::string joinCxxForward(std::optional<std::vector<SwiftSubclassCxxCodeGen::PrintedFunctionParam>> argsOpt,
                               bool useExternalLabels,
                               bool forFunctionPointer) {
        
        if (!argsOpt) {
            std::cerr << "Error! Could not get function parameter names and printed types for " << ASTHelpers::getAsString(method) << std::endl;
            __builtin_trap();
        }
        std::stringstream ss;
        for (size_t i = 0; i < argsOpt->size(); i++) {
            const auto& param = (*argsOpt)[i];
            
            // rdar://156635576 (Runtime crash calling C function pointer with const& argument from C++ when initialized with Swift closure)
            // Workaround: Use const* instead of const& for FP arguments.
            if (forFunctionPointer && TypesHelper::isConstRef(param.qualType)) {
                ss << "&";
            }
            
            if (forFunctionPointer) {
                // Function pointers need to be reindexed after the Swift pointer
                if (i == 0) {
                    ss << param.internalName;
                } else {
                    ss << "arg" << (i - 1);
                }
            } else {
                if (useExternalLabels) {
                    ss << param.externalName;
                } else {
                    ss << param.internalName;
                }
            }
            if (i + 1 < argsOpt->size()) {
                ss << ", ";
            }
        }
        return ss.str();
    }
    
    std::string joinSwiftDeclare(std::optional<std::vector<SwiftSubclassCxxCodeGen::PrintedFunctionParam>> argsOpt,
                                 bool allowDefaultExprs) {
        // We could write a C++ function per default expression per function and have Swift call that instead. But just
        // disable it for now for prototyping
        #warning Disabling Swift declaring any methods with default expressions because unscoped enums will have problems, should fix this at some point
        allowDefaultExprs = false;
        
        if (!argsOpt) {
            std::cerr << "Error! Could not get function parameter names and printed types for " << ASTHelpers::getAsString(method) << std::endl;
            __builtin_trap();
        }
        std::stringstream ss;
        for (size_t i = 0; i < argsOpt->size(); i++) {
            auto param = (*argsOpt)[i];
            
            if (TypesHelper::isConstRef(param.qualType)) {
                clang::QualType orig = param.qualType;
                param.qualType = TypesHelper::removingConstRefIfPossible(param.qualType);
                auto printer = codeGen->typeNamePrinter(param.qualType);
                std::optional<std::string> x = codeGen->getTypeNameOpt<SwiftNameInSwift>(printer);
                if (!x) {
                    std::cerr << "Error! Could not remove ref-const and reprint " << orig.getAsString() << std::endl;
                    __builtin_trap();
                }
                param.printedType = *x;
            }
            
            ss << "_";
            if (!param.externalName.empty()) {
                ss << " " << param.externalName;
            }
            ss << ": " << param.printedType;
            if (param.qualType->isPointerType()) {
                ss << "?";
            }
            if (allowDefaultExprs) {
                if (param.defaultArg) {
                    ss << " = " << *param.defaultArg;
                }
            }
            
            if (i + 1 < argsOpt->size()) {
                ss << ", ";
            }
        }
        return ss.str();

    }
    
    std::string joinSwiftForward(std::optional<std::vector<SwiftSubclassCxxCodeGen::PrintedFunctionParam>> argsOpt,
                                 bool forFunctionPointer) {
        if (!argsOpt) {
            std::cerr << "Error! Could not get function parameter names and printed types for " << ASTHelpers::getAsString(method) << std::endl;
            __builtin_trap();
        }
        std::stringstream ss;
        for (size_t i = 0; i < argsOpt->size(); i++) {
            const auto& param = (*argsOpt)[i];
            
            if (forFunctionPointer) {
                ss << param.internalName;
            } else {
                ss << param.externalName;
            }
            // rdar://156635576 (Runtime crash calling C function pointer with const& argument from C++ when initialized with Swift closure)
            // Workaround: Use const* instead of const& for FP arguments.
            if (forFunctionPointer && TypesHelper::isConstRef(param.qualType)) {
                ss << ".pointee";
            }
            
            if (i + 1 < argsOpt->size()) {
                ss << ", ";
            }
        }
        return ss.str();
    }
    
    MethodHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::CXXMethodDecl* method, std::string baseTypeS)
    : codeGen(codeGen), method(method) {
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
        
        staticS = method->isStatic() ? "static " : "";
        staticCommentedS = method->isStatic() ? "/* static */ " : "";
        nameS = method->getNameAsString();
        constS = method->isConst() ? " const" : "";
        
        functionPointerS = "__" + nameS + "_FP";
        defaultImplementationS = "__" + nameS + " _default";
        forwardS = "__" + nameS + "_forward";
        
        // baseNameS
        {
            std::string tempTemplateS = "";
            if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
                tempTemplateS = "<";
                clang::FunctionTemplateDecl* functionTemplate = method->getDescribedFunctionTemplate();
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
            }
            
            if (method->isStatic()) {
                baseNameS = baseTypeS+"::"+nameS+tempTemplateS;
            } else {
                baseNameS = "this->"+baseTypeS+"::"+nameS+tempTemplateS;
            }
        }
        
        // cxxDeclareArgumentsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, {}, false);
            cxxDeclareArgumentsS = joinCxxDeclare(argsOpt,
                                                  /*allowDefaultExprs=*/true,
                                                  /*internalNames=*/false,
                                                  /*forFunctionPointer=*/false);
        }
        
        // cxxDeclareArgumentsNoDefaultExprsInsertSwiftSubclassPointerS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, {"SwiftSubclass"}, false);
            cxxDeclareArgumentsNoDefaultExprsInsertSwiftSubclassPointerS = joinCxxDeclare(argsOpt,
                                                                                          /*allowDefaultExprs=*/false,
                                                                                          /*internalNames=*/false,
                                                                                          /*forFunctionPointer=*/true);
        }
        
        // cxxDefineArgumentsIndexedLabelsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, {}, false);
            cxxDefineArgumentsIndexedLabelsS = joinCxxDeclare(argsOpt,
                                                              /*allowDefaultExprs=*/false,
                                                              /*internalNames=*/true,
                                                              /*forFunctionPointer=*/false);
        }
        
        // cxxForwardArgumentsOriginalLabelsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, {}, false);
            cxxForwardArgumentsOriginalLabelsS = joinCxxForward(argsOpt,
                                                                /*useExternalLabels=*/true,
                                                                /*forFunctionPointer=*/false);
        }
        
        // cxxForwardArgumentsIndexedLabelsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, {}, false);
            cxxForwardArgumentsIndexedLabelsS = joinCxxForward(argsOpt,
                                                               /*useExternalLabels=*/false,
                                                               /*forFunctionPointer=*/false);
        }
        
        // cxxForwardArgumentsIndexedLabelsInsertSwiftSubclassPointerS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<CppNameInCpp>(method, { "SwiftSubclass", "", "__swiftSubclass" }, false);
            cxxForwardArgumentsIndexedLabelsInsertSwiftSubclassPointerS = joinCxxForward(argsOpt,
                                                                                         /*useExternalLabels=*/false,
                                                                                         /*forFunctionPointer=*/true);
        }
        
        // swiftReturnTypeWithReturnIndirectionAllocationS
        {
            clang::QualType q = TypesHelper::removingValueTypeIndirectionIfPossible(method->getReturnType(), codeGen);
            auto typeNamePrinter = codeGen->typeNamePrinter(q);
            std::optional<std::string> printedType = codeGen->getTypeName<SwiftNameInSwift>(typeNamePrinter);
            swiftReturnTypeWithReturnIndirectionAllocationS = *printedType;
        }
        // swiftDeclareArgumentsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<SwiftNameInSwift>(method, {}, false);
            swiftDeclareArgumentsS = joinSwiftDeclare(argsOpt, true);
        }
        // swiftDeclareArgumentsNoDefaultExprsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<SwiftNameInSwift>(method, {}, false);
            swiftDeclareArgumentsNoDefaultExprsS = joinSwiftDeclare(argsOpt, false);
        }
        // swiftForwardArgumentsS
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<SwiftNameInSwift>(method, {}, false);
            swiftForwardArgumentsS = joinSwiftForward(argsOpt, false);
        }
        
        // swiftForwardArgumentsForFunctionPointer
        {
            auto argsOpt = codeGen->getPrintedFunctionParamsOpt<SwiftNameInSwift>(method, {}, true);
            swiftForwardArgumentsForFunctionPointer = joinSwiftForward(argsOpt, true);
        }
        
        // swiftAccessLevelS
        swiftAccessLevelS = method->isVirtual() ? "open " : "public ";
    }
};

void SwiftSubclassCxxCodeGen::writeHeaderFile(const Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        SwiftSubclassCxxAnalysisResult analysisResult = getSwiftSubclassCxxAnalysisPass()->find(tagDecl)->second;
        StringsHelper h{this, tagDecl, analysisResult};
        
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            std::string tagDeclName = getTypeName<SwiftNameInCpp>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            writeLine("// MARK: "+tagDeclName+" subclassing");
            
            std::vector<std::string> namespaces = computeQualifiedNameComponents(tagDecl);
            namespaces[0] = "__Overlay";
            for (size_t i = 0; i < namespaces.size(); i++) {
                writeLine(computeIndentation(i)+"namespace "+namespaces[i]+" {");
            }
            std::string indent = computeIndentation(namespaces.size());
            
            writeLines({
                indent+"class "+h.typeNames.cxxAdapter+" final: public "+tagDeclName+" {",
                indent+"public:",
                indent+"    // Swift pointer",
                indent+"    typedef void*_Nullable SwiftSubclass;",
                indent+"    ",
                indent+"    SwiftSubclass "+h.fieldNames.swiftSubclassPointer+" = nullptr;",
                indent+"    void (*_Nullable "+h.fieldNames.releaseFP+")(SwiftSubclass) = nullptr;",
                indent+"    virtual ~"+h.typeNames.cxxAdapter+"();",
                indent+"",
            });
            
            writeLine(indent+"    // Conditional downcasting");
            for (const clang::CXXRecordDecl* targetDecl : h.conditionalDowncastingTargets) {
                auto localPrinter = typeNamePrinter(targetDecl);
                std::string targetName = getTypeName<SwiftNameInCpp>(localPrinter);
                writeLines({
                    indent+"    static inline "+h.typeNames.cxxAdapter+"*_Nullable "+h.methodNames.dynamicCast+"("+targetName+"*_Nullable p) {",
                    indent+"        return dynamic_cast<"+h.typeNames.cxxAdapter+"*>(p);",
                    indent+"    }",
                });
            }
            if (!h.conditionalDowncastingTargets.empty()) { writeLine(indent+"    "); }
            
            
            writeLine(indent+"    // Unconditional upcasting");
            for (const auto& targetDecl : h.unconditionalUpcastingTargets) {
                auto localPrinter = typeNamePrinter(targetDecl);
                std::string targetName = getTypeName<SwiftNameInCpp>(localPrinter);
                writeLines({
                    // `unused` argument lets us overload on `__static_cast`, otherwise we'd be overloading on the return type
                    // which isn't allowed in C++
                    indent+"    static inline "+targetName+"*_Nonnull "+h.methodNames.staticCast+"("+h.typeNames.cxxAdapter+"*_Nonnull p, "+targetName+"*_Nullable unused) {",
                    indent+"        return static_cast<"+targetName+"*>(p);",
                    indent+"    }"
                });
            }
            if (!h.unconditionalUpcastingTargets.empty()) { writeLine(indent+"    "); }
            
            
            if (analysisResult.destructor->getAccess() == clang::AS_public) {
                writeLines({
                    indent+"    // Public destructor of "+tagDeclName+" will be exposed to Swift",
                    indent+"    static inline void "+h.methodNames.swiftDeleteBase+"("+tagDeclName+"*_Nonnull p) {",
                    indent+"        delete p;",
                    indent+"    }",
                    indent+"    ",
                });
            } else {
                writeLines({
                    indent+"    // Non-public destructor of "+tagDeclName+" will not be exposed to Swift",
                    indent+"    ",
                });
            }
            
            writeLine(indent+"    // Non-private constructors of "+tagDeclName+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysisResult.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                MethodHelper mh{this, ctor, tagDeclName};
                writeLines({
                    indent+"    "+h.typeNames.cxxAdapter+"(" + mh.cxxDeclareArgumentsS + ");",
                    indent+"    static "+h.typeNames.cxxAdapter+"*_Nonnull "+h.methodNames.swiftNew+"(" + mh.cxxDeclareArgumentsS + ");",
                    indent+"    ",
                });
            }
            
            if (analysisResult.destructor->getAccess() != clang::AS_private) {
                writeLines({
                    indent+"    // Non-private destructor of "+h.typeNames.cxxAdapter+" will be exposed to Swift",
                    indent+"    void "+h.methodNames.swiftDeleteCxxAdapter+"();",
                    indent+"    ",
                });
            } else {
                writeLines({
                    indent+"    // Private destructor of "+h.typeNames.cxxAdapter+" will not be exposed to Swift",
                    indent+"    ",
                });
            }
            
            writeLine(indent+"    // Start total inheritance from "+tagDeclName);
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInCpp>(recordPrinter);
                
                writeLine(indent+"    // Start fields from "+targetName);
                for (const clang::FieldDecl* field : h.fieldsForInheritance(record)) {
                    auto typePrinter = typeNamePrinter(field->getType());
                    std::string typeString = getTypeName<CppNameInCpp>(typePrinter);
                    
                    writeLine(indent+"    "+" "+h.methodNames.fieldGetter(field)+"() const;");
                    if (!field->getType().isConstQualified()) {
                        writeLine(indent+"    void "+h.methodNames.fieldSetter(field)+"_set("+typeString+");");
                    }
                    writeLine(indent+"    ");
                }
                writeLines({
                    indent+"    // End fields from "+targetName,
                    indent+"    ",
                    indent+"    // Start methods from "+targetName,
                });
                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    TypesHelper th{this};
                    
                    auto retPrinter = typeNamePrinter(method->getReturnType());
                    std::string retString = getTypeName<CppNameInCpp>(retPrinter)+" ";
                    
                    std::string fpRetString = retString;
                    if (TypesHelper(this).doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                        auto fpRetPrinter = typeNamePrinter(th.getSwiftReturnIndirectionAllocationType(method));
                        fpRetString = getTypeName<CppNameInCpp>(fpRetPrinter)+" ";
                    }
                    
                                        
                    // Templated methods write their body in the header file...
                    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
                        writeLines({
                            indent+"    "+mh.templateS,
                            indent+"    " + retString + mh.staticS + mh.nameS + "("+  mh.cxxDeclareArgumentsS + ")" + mh.constS + " {",
                            indent+"        return "+mh.baseNameS+"("+mh.cxxForwardArgumentsOriginalLabelsS+");",
                            indent+"    }",
                            indent+"    ",
                        });
                        continue;
                    }
                    
                    // All virtual methods get a function pointer that will point to a Swift implementation of it
                    if (method->isVirtual()) {
                        writeLine(indent+"    " + fpRetString + "(*_Nullable " + mh.functionPointerS + ")(" + mh.cxxDeclareArgumentsNoDefaultExprsInsertSwiftSubclassPointerS + ") = nullptr;");
                        
                        // Virtual methods with a default implementation expose that default implementation to Swift,
                        // in case Swift wants to call it
                        if (!method->isPureVirtual()) {
                            writeLine(indent+"    " + retString + mh.defaultImplementationS + "(" + mh.cxxDeclareArgumentsS + ")" + mh.constS + ";");
                        }
                    }
                    
                    // All methods get a "forwarding version" that exposes them to Swift, because
                    // if they're inherited from a non-imported C++ type, Swift won't be able to access them
                    // otherwise. https://github.com/swiftlang/swift/issues/83114
                    writeLine(indent+"    " + mh.staticS + retString + mh.forwardS + "(" + mh.cxxDeclareArgumentsS + ")" + mh.constS + ";");
                    
                    // Finally, all virtual methods get their actual inherited decl overridden. (This is the method
                    // that C++code that doesn't know anything about SwiftSubclassCxx tricks calls, just using
                    // bog-standard virtual dispatch in vanilla C++.)
                    if (method->isVirtual()) {
                        writeLine(indent+"    " + retString + mh.nameS + "(" + mh.cxxDeclareArgumentsS + ")"+mh.constS+" override final;");
                    }
                    
                    writeLine(indent+"    ");
                }
                writeLines({
                    indent+"    // End methods from "+tagDeclName,
                    indent+"    ",
                });
                
            } // for (const auto& recordFuturePair : inheritancePairs(tagDecl))
            writeLine(indent+"    // End total inheritance from "+tagDeclName);
            
            writeLine(indent+"}"+h.adapterDefinitionCloseFRTAnnotation+";");
            
            // Close the namespaces that contain our CxxAdapter
            for (size_t i = 0; i < namespaces.size(); i++) {
                writeLine(computeIndentation(namespaces.size() - i - 1)+"}");
            }
        }
        
        // Add extra space between each type in the for loop,
        // after the printer and guard blocker have been destroyed
        writeLines({
            "",
            "",
            "",
            "",
        });
    } // for (const clang::TagDecl* tagDecl : data)
}

void SwiftSubclassCxxCodeGen::writeCppFile(const Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        SwiftSubclassCxxAnalysisResult analysisResult = getSwiftSubclassCxxAnalysisPass()->find(tagDecl)->second;
        StringsHelper h{this, tagDecl, analysisResult};
        
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            std::string tagDeclName = getTypeName<SwiftNameInCpp>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            writeLine("// MARK: "+tagDeclName+" subclassing");
            
            writeLines({
                h.typeNames.fullyQualifiedCxxAdapter+"::~"+h.typeNames.cxxAdapter+"() {",
                "    if (" + h.fieldNames.swiftSubclassPointer + ") {",
                "        " + h.fieldNames.releaseFP + "(" + h.fieldNames.swiftSubclassPointer + ");",
                "        " + h.fieldNames.swiftSubclassPointer + " = nullptr;",
                "        " + h.fieldNames.releaseFP + " = nullptr;",
                "    }",
                "}",
                ""
            });

            
            // Conditional downcasting is already handled by inline method in header
            
            // Unconditional upcasting is already handled by inline method in header
            
            // Public destructor of tagDeclName is already handled by inline method in header
            
            writeLine("// Non-private constructors of "+tagDeclName+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysisResult.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                MethodHelper mh{this, ctor, tagDeclName};
                writeLines({
                    h.typeNames.fullyQualifiedCxxAdapter+"::"+h.typeNames.cxxAdapter+"(" + mh.cxxDefineArgumentsIndexedLabelsS + ") :",
                    tagDeclName+"("+ mh.cxxForwardArgumentsIndexedLabelsS +")",
                    "{}",
                    "/* static */ "+h.typeNames.fullyQualifiedCxxAdapter+"*_Nonnull "+h.typeNames.fullyQualifiedCxxAdapter+"::"+h.methodNames.swiftNew+"(" + mh.cxxDefineArgumentsIndexedLabelsS + ") {",
                    "    return new "+h.typeNames.fullyQualifiedCxxAdapter+"("+mh.cxxForwardArgumentsIndexedLabelsS+");",
                    "}",
                });
            }
            writeLine("");
            
            if (analysisResult.destructor->getAccess() != clang::AS_private) {
                writeLines({
                    "// Non-private destructor of "+h.typeNames.cxxAdapter+" will be exposed to Swift",
                    "void "+h.typeNames.fullyQualifiedCxxAdapter+"::"+h.methodNames.swiftDeleteCxxAdapter+"() {",
                    "    delete this;",
                    "}",
                });
            }
            writeLines({
                "",
                "// Start total inheritance from "+tagDeclName,
            });
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInCpp>(recordPrinter);
                
                writeLine("// Start fields from "+targetName);
                for (const clang::FieldDecl* field : h.fieldsForInheritance(record)) {
                    auto typePrinter = typeNamePrinter(field->getType());
                    std::string typeString = getTypeName<CppNameInCpp>(typePrinter);
                    
                    writeLines({
                        h.typeNames.fullyQualifiedCxxAdapter+"::"+h.methodNames.fieldGetter(field)+"() const {",
                        "    return this->"+field->getNameAsString()+";",
                        "}",
                    });
                    if (!field->getType().isConstQualified()) {
                        writeLines({
                            h.typeNames.fullyQualifiedCxxAdapter+"::"+h.methodNames.fieldSetter(field)+"_set("+typeString+" x) {",
                            "    this->"+field->getNameAsString()+" = x;",
                            "}",
                        });
                    }
                    writeLine("");
                }
                writeLines({
                    "// End fields from "+targetName,
                    "",
                    "// Start methods from "+targetName,
                });
                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    TypesHelper th{this};
                    
                    auto retPrinter = typeNamePrinter(method->getReturnType());
                    std::string retString = getTypeName<CppNameInCpp>(retPrinter)+" ";
                    
                    // Templated methods write their body in the header file...
                    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
                        continue;
                    }
                    
                    // All virtual methods get a function pointer that will point to a Swift implementation of it
                    if (method->isVirtual()) {
                        // Virtual methods with a default implementation expose that default implementation to Swift,
                        // in case Swift wants to call it
                        if (!method->isPureVirtual()) {
                            writeLines({
                                retString + h.typeNames.fullyQualifiedCxxAdapter+"::"+mh.defaultImplementationS+"("+mh.cxxDefineArgumentsIndexedLabelsS+")"+mh.constS+" {",
                                "    return "+mh.baseNameS+"("+mh.cxxForwardArgumentsIndexedLabelsS+");",
                                "}",
                            });
                        }
                    }
                    
                    // All methods get a "forwarding version" that exposes them to Swift, because
                    // if they're inherited from a non-imported C++ type, Swift won't be able to access them
                    // otherwise. https://github.com/swiftlang/swift/issues/83114
                    writeLines({
                        mh.staticCommentedS + retString + h.typeNames.fullyQualifiedCxxAdapter+"::"+mh.forwardS + "(" + mh.cxxDefineArgumentsIndexedLabelsS + ")" + mh.constS + " {",
                        "    return " + mh.nameS + "(" + mh.cxxForwardArgumentsIndexedLabelsS + ");",
                        "}",
                    });
                    
                    // Finally, all virtual methods get their actual inherited decl overridden. (This is the method
                    // that C++code that doesn't know anything about SwiftSubclassCxx tricks calls, just using
                    // bog-standard virtual dispatch in vanilla C++.)
                    if (method->isVirtual()) {
                        std::string toConvert = mh.functionPointerS + "(" + mh.cxxForwardArgumentsIndexedLabelsInsertSwiftSubclassPointerS + ")";
                        std::string converted = th.convertSwiftReturnIndirectionAllocationCxxFpToCxxVirtual(method, toConvert);
                        writeLines({
                            retString + h.typeNames.fullyQualifiedCxxAdapter+"::"+mh.nameS+"(" + mh.cxxDefineArgumentsIndexedLabelsS + ")" + mh.constS + " {",
                            "    return " + converted + ";",
                            "}",
                        });
                    }

                    writeLine("");
                }
                writeLine("// End methods from "+targetName);
                
            } // for (const auto& recordFuturePair : inheritancePairs(tagDecl))
            writeLine("// End total inheritance from "+tagDeclName);

        }

        // Add extra space between each type in the for loop,
        // after the printer and guard blocker have been destroyed
        writeLines({
            "",
            "",
            "",
            "",
        });
    } // for (const clang::TagDecl* tagDecl : data)

}


void SwiftSubclassCxxCodeGen::writeSwiftFile(const Data& data) {
    writeLines({
        "public enum " + StringsHelper::TypeNames::overlaySwiftEnum + "{}"
        "",
        "",
        "",
        "",
    });
    
    for (const clang::TagDecl* tagDecl : data) {
        SwiftSubclassCxxAnalysisResult analysisResult = getSwiftSubclassCxxAnalysisPass()->find(tagDecl)->second;
        StringsHelper h{this, tagDecl, analysisResult};
        
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            std::string tagDeclName = getTypeName<SwiftNameInSwift>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            writeLine("// MARK: "+tagDeclName+" subclassing");

            // Make it convenient to access the corresponding SwiftAdapter
            // of any CxxAdapter
            writeLines({
                "extension " + h.typeNames.swiftFullyQualifiedCxxAdapter + " {",
                "    public var swift: " + h.typeNames.swiftProtocolCompositionTypealias + " {",
                "        "+h.typeNames.unmanagedSwiftAdapter+".fromOpaque("+h.fieldNames.swiftSubclassPointer+"!).takeUnretainedValue() as! " + h.typeNames.swiftProtocolCompositionTypealias,
                "    }",
                "}",
                "",
            });
            
            
            writeLine("// Conditional downcasting");
            for (const clang::CXXRecordDecl* targetDecl : h.conditionalDowncastingTargets) {
                auto localPrinter = typeNamePrinter(targetDecl);
                std::string targetName = getTypeName<SwiftNameInSwift>(localPrinter);
                
                writeLines({
                   "extension " + targetName + " {",
                    "    public func `as`<T: " + h.typeNames.swiftProtocolCompositionTypealias + ">(_ t: T.Type = T.self) -> T? {",
                    "        " + h.typeNames.swiftFullyQualifiedCxxAdapter + "." + h.methodNames.dynamicCast + "(self)?.swift as? T",
                    "    }",
                    "}",
                    "",
                });
            }
            if (!h.conditionalDowncastingTargets.empty()) { writeLine(""); }

            // Unconditional upcasting will be handled in the SwiftAdapter definition, which we haven't started yet
            
            if (analysisResult.destructor->getAccess() == clang::AS_public) {
                writeLines({
                    "// Public destructor of "+tagDeclName+" will be exposed to Swift",
                    "",
                    "extension " + tagDeclName + " {",
                    "    /// Deletes the argument using `delete x;` in C++. ",
                    "    /// ",
                    "    /// The caller is responsible for ensuring that the argument",
                    "    /// is not used after this function returns.",
                    "    public static func delete(_ x: consuming "+tagDeclName+") {",
                    "        " + h.typeNames.swiftFullyQualifiedCxxAdapter + "." + h.methodNames.swiftDeleteBase + "(x)",
                    "    }",
                    "}",
                    "",
                });
            } else {
                writeLines({
                    "// Non-public destructor of "+tagDeclName+" will not be exposed to Swift",
                    "",
                });
            }

            std::vector<std::string> namespaces = computeQualifiedNameComponents(tagDecl);
            namespaces[0] = h.typeNames.overlaySwiftEnum;
            namespaces.push_back(h.typeNames.swiftAdapter);
            for (size_t i = 0; i < namespaces.size(); i++) {
                if (i == 0) {
                    writeLine(computeIndentation(i)+"public extension "+namespaces[i]+" {");
                } else if (i + 1 < namespaces.size()) {
                    writeLine(computeIndentation(i)+"public enum "+namespaces[i]+" {");
                } else {
                    writeLine(computeIndentation(i)+"open class "+namespaces[i]+" {");
                }
            }
            std::string indent = computeIndentation(namespaces.size());
            
            writeLines({
                indent+"private typealias UnmanagedSelf = "+h.typeNames.unmanagedSwiftAdapter,
                indent,
                indent+"// Detect calling "+h.methodNames.wireToCxx+" more than once, which is against core invariants",
                indent+"private var "+h.fieldNames.swiftHasWired+": Bool",
                indent,
                indent+"// Detect if we're a zombie object to avoid setting "+h.fieldNames.swiftDidDeinit+".pointee in deinit after "+h.fieldNames.releaseFP+" deallocated it",
                indent+"private var "+h.fieldNames.swiftIsZombie+": Bool",
                indent,
                indent+"// Detect whether or not a call to Unmanaged.release() deinitialized us",
                indent+"// or if there's an outstanding strong pointer, making us a zombie",
                indent+"private var "+h.fieldNames.swiftDidDeinit+": UnsafeMutablePointer<Bool>",
                indent,
                indent+"public required init() {",
                indent+"    self."+h.fieldNames.swiftHasWired+" = false",
                indent+"    self."+h.fieldNames.swiftIsZombie+" = false",
                indent+"    self."+h.fieldNames.swiftDidDeinit+" = UnsafeMutablePointer<Bool>.allocate(capacity: 1)",
                indent+"    self."+h.fieldNames.swiftDidDeinit+".initialize(to: false)",
                indent+"}",
                indent,
                indent+"// Cxx pointer",
                indent+"private var "+h.fieldNames.cxxSubclassPointer+": "+h.typeNames.swiftFullyQualifiedCxxAdapter+"!",
                indent,
                indent+"public func get_cxx() -> "+h.typeNames.swiftFullyQualifiedCxxAdapter+"? { "+h.fieldNames.cxxSubclassPointer+" }",
                indent,
            });
            
            writeLine(indent+"// Unconditional upcasting");
            for (const clang::CXXRecordDecl* targetDecl : h.unconditionalUpcastingTargets) {
                auto localPrinter = typeNamePrinter(targetDecl);
                std::string targetName = getTypeName<SwiftNameInSwift>(localPrinter);
                
                writeLines({
                    indent+"public func `as`(_ t: " + targetName + ".Type) -> " + targetName + " {",
                    indent+"    "+h.typeNames.swiftFullyQualifiedCxxAdapter+"."+h.methodNames.staticCast+"("+h.fieldNames.cxxSubclassPointer+", nil)",
                    indent+"}",
                });
            }
            if (!h.unconditionalUpcastingTargets.empty()) { writeLine(""); }

            // Important: We do our best to turn virtual methods that return values by indirection
            // into overriddable Swift methods that return values by copy. This is important for
            // memory safety, because otherwise Swift could easily return dangling pointers.
            // When Swift virtual implementations return values by copy that are by indirection in C++,
            // we copy the value into a pointer allocation, then return that pointer to give C++
            // a stable address.
            writeLine(indent+"// Pointer/ref-returning method allocations");
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInSwift>(recordPrinter);
                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    TypesHelper th{this};
                    
                    if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                        auto retPrinter = typeNamePrinter(th.getSwiftReturnIndirectionAllocationType(method));
                        std::string swiftRet = getTypeName<SwiftNameInSwift>(retPrinter) + "?";
                        writeLine(indent+"private var "+h.fieldNames.returnIndirectionAllocation(method)+": "+swiftRet);
                    }
                }
            }
            writeLine(indent);
            
            // The `wireToCxx` method is important but decently complicated to read and write, so
            // pull it out into a local scope for increased code clarity
            {
                writeLines({
                    indent+"private func "+h.methodNames.wireToCxx+"(_ " + h.fieldNames.cxxSubclassPointer + ": " + h.typeNames.swiftFullyQualifiedCxxAdapter + ") {",
                    indent+"    func slf(_ raw: UnsafeMutableRawPointer?) -> "+h.typeNames.swiftProtocolCompositionTypealias + " {",
                    indent+"        UnmanagedSelf.fromOpaque(raw!).takeUnretainedValue() as! "+h.typeNames.swiftProtocolCompositionTypealias,
                    indent+"    }",
                    indent+"    if self."+h.fieldNames.swiftHasWired+" {",
                    indent+"        fatalError(\"Cannot call "+h.methodNames.wireToCxx+" more than once on a given instance. Don't call this from user code.\")",
                    indent+"    } else {",
                    indent+"        self."+h.fieldNames.swiftHasWired+" = true",
                    indent+"    }",
                    indent+"    ",
                    indent+"    self."+h.fieldNames.cxxSubclassPointer+" = "+h.fieldNames.cxxSubclassPointer,
                    indent+"    "+h.fieldNames.cxxSubclassPointer+"."+h.fieldNames.swiftSubclassPointer+" = UnmanagedSelf.passRetained(self).toOpaque()",
                    indent+"    "+h.fieldNames.cxxSubclassPointer+"."+h.fieldNames.releaseFP+" = {",
                    indent+"        let swiftDidDeinitPointer = slf($0)."+h.fieldNames.swiftDidDeinit,
                    indent+"        let cxxSubclassPointer = slf($0)."+h.fieldNames.cxxSubclassPointer,
                    indent+"        slf($0)."+h.fieldNames.cxxSubclassPointer+" = nil",
                    indent+"        UnmanagedSelf.fromOpaque($0!).release()",
                    indent+"        let isZombie = !swiftDidDeinitPointer.pointee",
                    indent+"        swiftDidDeinitPointer.deallocate()",
                    indent+"        if isZombie {",
                    indent+"            slf($0)."+h.fieldNames.swiftIsZombie+" = true",
                    indent+"            let cppInstance: UnsafeMutableRawPointer? = if let cxxSubclassPointer { Unmanaged<"+h.typeNames.swiftFullyQualifiedCxxAdapter+">.passUnretained(cxxSubclassPointer).toOpaque() } else { nil }",
                    indent+"            __Overlay.SwiftSubclassCxx_zombieCreated(swiftInstance: $0, cppInstance: cppInstance, typeName: \""+tagDeclName+"\")",
                    indent+"        }",
                    indent+"    }",
                });
                
                for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                    const clang::CXXRecordDecl* record = recordFuturePair.first;
                    std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                    
                    auto recordPrinter = typeNamePrinter(record);
                    std::string targetName = getTypeName<SwiftNameInSwift>(recordPrinter);
                    
                    writeLine(indent+"    // Start wire virtual methods from " + targetName);
                    for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                        const clang::CXXRecordDecl* record = recordFuturePair.first;
                        std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                        
                        for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                            // Pull out some common strings that we'll want to use regardless of the method kind
                            MethodHelper mh{this, method, targetName};
                            TypesHelper th{this};
                            
                            if (method->getAccess() == clang::AS_private) { continue; }
                            if (!method->isVirtual()) { continue; }
                            
                            if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                                auto retPrinter = typeNamePrinter(th.getSwiftReturnIndirectionAllocationType(method));
                                std::string allocateType = getTypeName<SwiftNameInSwift>(retPrinter);
                                
                                writeLines({
                                    indent+"    "+h.fieldNames.cxxSubclassPointer+"."+mh.functionPointerS + " = {",
                                    indent+"        let newValue = slf($0)."+mh.nameS+"("+mh.swiftForwardArgumentsForFunctionPointer+")",
                                    indent+"        if slf($0)."+h.fieldNames.returnIndirectionAllocation(method)+"?.pointee != newValue {",
                                    indent+"            if let p = slf($0)."+h.fieldNames.returnIndirectionAllocation(method)+" {",
                                    indent+"                p.deinitialize(count: 1).deallocate()",
                                    indent+"            }",
                                    indent+"            slf($0)."+h.fieldNames.returnIndirectionAllocation(method)+" = "+allocateType+".allocate(capacity: 1)",
                                    indent+"            slf($0)."+h.fieldNames.returnIndirectionAllocation(method)+"!.initialize(to: newValue)",
                                    indent+"        }",
                                    indent+"        return slf($0)."+h.fieldNames.returnIndirectionAllocation(method),
                                    indent+"    }",
                                });
                            } else {
                                writeLine(indent+"    "+h.fieldNames.cxxSubclassPointer+"."+mh.functionPointerS + " = { slf($0)."+mh.nameS+"("+mh.swiftForwardArgumentsForFunctionPointer+") }");
                            }
                        }
                    }
                    writeLines({
                        indent+"    // End wire virtual methods from " + targetName,
                        indent,
                    });
                }
                
                writeLines({
                    indent+"} // public func "+h.methodNames.wireToCxx,
                    indent,
                });
            } // End of local scope for `wireToCxx`
            
            
            writeLines({
                indent+"deinit {",
                indent+"    // "+h.fieldNames.releaseFP + ", which is run by the C++ destructor, sets "+h.fieldNames.cxxSubclassPointer+" to nil.",
                indent+"    // CxxAdapter has a strong pointer to SwiftAdapter, so SwiftAdapter.deinit running before ~CxxAdapter is a user-induced memory-safety issue",
                indent+"    if self."+h.fieldNames.cxxSubclassPointer+" != nil {",
                indent+"        fatalError(\"Swift deinit running before C++ destructor ran violates invariants. Did you overrelease the Swift subclass instance?\")",
                indent+"    }",
            });
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInSwift>(recordPrinter);
                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    TypesHelper th{this};
                    
                    if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                        writeLine(indent+"    "+h.fieldNames.returnIndirectionAllocation(method)+"?.deinitialize(count: 1).deallocate()");
                    }
                }
            }
            writeLines({
                indent+"    if !self."+h.fieldNames.swiftIsZombie+" {",
                indent+"        // Set the flag but don't deallocate. The releaseFP will deallocate after checking if this flag was set",
                indent+"        self."+h.fieldNames.swiftDidDeinit+".pointee = true",
                indent+"    }",
                indent+"} // deinit",
                indent,
            });
            

            writeLine(indent+"// Non-private constructors of "+tagDeclName+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysisResult.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                bool isProtected = ctor->getAccess() == clang::AS_protected;
                std::string methodName = isProtected ? "_newProtected" : "new";

                MethodHelper mh{this, ctor, tagDeclName};
                std::string tagDeclCxxName = getTypeName<CppNameInCpp>(printer);
                writeLines({
                    indent+"",
                    indent+"/// Creates a new instance of this subclass using `new "+tagDeclCxxName+"(...);` in C++.",
                    indent+"/// ",
                    indent+"/// The caller is responsible for deleting the instance in one of the following ways:",
                    indent+"/// - Using `"+h.typeNames.swiftProtocolCompositionTypealias+".delete(_:)` in Swift",
                    indent+"/// - Using `"+tagDeclName+".delete(_:)` on the value returned by `self.get_cxx()`",
                    indent+"/// - Using `delete p;` in C++ on the value returned by `self.get_cxx()`",
                    indent+"/// - Passing the value returned by `self.get_cxx()` to something that will ensure it is eventually deleted, like `std::unique_ptr` or `std::shared_ptr`",
                    indent+"/// Note that Swift ARC will _not_ automatically free the returned instance. Dropping the last strong reference in user code to the instance can result in memory leaks. ",
                });
                if (isProtected) {
                    writeLines({
                        indent+"/// ",
                        indent+"/// This constructor was originally protected in C++. Use with caution."
                    });
                }
                writeLines({
                    indent+"public static func "+methodName+"(" + mh.swiftDeclareArgumentsS + ") -> "+h.typeNames.swiftProtocolCompositionTypealias+" {",
                    indent+"    let swiftAdapter = Self.init()",
                    indent+"    if type(of: swiftAdapter) == "+h.typeNames.fullyQualifiedSwiftAdapter+".self {",
                    indent+"        fatalError(\"Cannot construct instance of abstract base class "+h.typeNames.swiftProtocolCompositionTypealias+"\")",
                    indent+"    }",
                    indent+"    guard swiftAdapter is "+h.typeNames.swiftProtocolCompositionTypealias+" else {",
                    indent+"        fatalError(\"Subclass \\(type(of: swiftAdapter)) of "+tagDeclName+" must inherit from "+h.typeNames.swiftProtocolCompositionTypealias+"\")",
                    indent+"    }",
                    indent+"    let cxxAdapter = "+h.typeNames.swiftFullyQualifiedCxxAdapter+"."+h.methodNames.swiftNew+"("+mh.swiftForwardArgumentsS+")",
                    indent+"    swiftAdapter."+h.methodNames.wireToCxx+"(cxxAdapter)",
                    indent+"    return swiftAdapter as! "+h.typeNames.swiftProtocolCompositionTypealias,
                    indent+"}",
                    indent,
                });
            }
            
            
            if (analysisResult.destructor->getAccess() != clang::AS_private) {
                // Important: Use a static method so that we can end the lifetime of the argument before the end of the method,
                // to avoid false-positives for zombie creation detection
                writeLines({
                    indent+"// Non-private destructor of "+h.typeNames.cxxAdapter+" will be exposed to Swift",
                    indent+"",
                    indent+"/// Deletes an instance of this subclass using `delete p;` in C++ on the pointer",
                    indent+"/// returned by `self.get_cxx()`.",
                    indent+"///",
                    indent+"/// The caller is responsible for ensuring that the pointer returned by",
                    indent+"/// `self.get_cxx()` is not used after this function returns.",
                    indent+"///",
                    indent+"/// Due to the way that SwiftUsd's mechanisms for subclassing C++ types from Swift are implemented,",
                    indent+"/// this Swift instance may become a \"zombie\" if there is a strong reference keeping it",
                    indent+"/// alive after this function returns. By default, SwiftUsd will print a warning to stdout",
                    indent+"/// when this occurs. You can change this behavior by setting the environment variable",
                    indent+"/// `SWIFTUSD_SWIFT_SUBCLASS_CXX_ZOMBIE_CREATION_BEHAVIOR` to `ignore` to suppress the warning,",
                    indent+"/// or `terminate` to immediately exit the program.",
                    indent+"/// After a zombie is created, any use of it might safely terminate the program.",
                    indent+"public static func delete(_ swiftAdapter: consuming "+h.typeNames.fullyQualifiedSwiftAdapter+") {",
                    indent+"    let cxxAdapter = swiftAdapter."+h.fieldNames.cxxSubclassPointer,
                    indent+"    _ = consume swiftAdapter",
                    indent+"    cxxAdapter?."+h.methodNames.swiftDeleteCxxAdapter+"()",
                    indent+"}",
                    indent,

                });
            } else {
                writeLines({
                    indent+"// Private destructor of "+h.typeNames.cxxAdapter+" will not be exposed to Swift",
                    indent,
                });
            }
            
            writeLine(indent+"// Start total inheritance from " + tagDeclName);
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInCpp>(recordPrinter);
                
                writeLine(indent+"// Start fields from "+targetName);
                for (const clang::FieldDecl* field : h.fieldsForInheritance(record)) {
                    auto typePrinter = typeNamePrinter(field->getType());
                    std::string typeString = getTypeName<SwiftNameInSwift>(typePrinter);

                    writeLines({
                        indent+"public var " + field->getNameAsString() + ": " + typeString + "{",
                        indent+"    get { "+h.fieldNames.cxxSubclassPointer+"."+h.methodNames.fieldGetter(field)+"() }",
                    });
                    if (!field->getType().isConstQualified()) {
                        // set { cxx.`field`_set(newValue) }
                        writeLine(indent+"    set { "+h.fieldNames.cxxSubclassPointer+"."+h.methodNames.fieldSetter(field)+"(newValue) }");
                    }
                    writeLines({
                        indent+"}",
                        indent,
                    });
                }
                writeLines({
                    indent+"// End fields from "+targetName,
                    indent,
                    indent+"// Start methods from "+targetName,
                });

                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    
                    auto retPrinter = typeNamePrinter(method->getReturnType());
                    std::string retString = getTypeName<SwiftNameInSwift>(retPrinter)+" ";
               
                    if (method->getAccess() == clang::AS_private) { continue; }
                    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) { continue; }
                    if (method->isPureVirtual()) { continue; }
                    
                    std::string forwardMethodTarget = method->isStatic() ? h.typeNames.swiftFullyQualifiedCxxAdapter : h.fieldNames.cxxSubclassPointer;
                    writeLines({
                        indent+mh.swiftAccessLevelS + mh.staticS + "func "+mh.nameS+"("+mh.swiftDeclareArgumentsS+") -> "+mh.swiftReturnTypeWithReturnIndirectionAllocationS+" {",
                        indent+"    "+forwardMethodTarget+"."+mh.forwardS+"("+mh.swiftForwardArgumentsS+")",
                        indent+"}",
                    });
                }
                writeLine(indent+"// End methods from "+targetName);
            }
            writeLines({
                indent+"// End total inheritance from " + tagDeclName,
                indent,
            });
            
            
            writeLine(indent+"public protocol PureVirtuals {");
            for (const auto& recordFuturePair : h.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                auto recordPrinter = typeNamePrinter(record);
                std::string targetName = getTypeName<SwiftNameInSwift>(recordPrinter);
                
                writeLine(indent+"    // Start methods from "+targetName);
                for (const clang::CXXMethodDecl* method : h.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, targetName};
                    if (method->isPureVirtual()) {
                        writeLine(indent+"    func "+mh.nameS+"("+mh.swiftDeclareArgumentsNoDefaultExprsS+") -> "+mh.swiftReturnTypeWithReturnIndirectionAllocationS);
                    }
                }
                writeLine(indent+"    // End methods from "+targetName);
            }
            writeLine(indent+"} // public protocol PureVirtuals");
            
            
            // Close the namespaces that contain our CxxAdapter
            for (size_t i = 0; i < namespaces.size(); i++) {
                writeLine(computeIndentation(namespaces.size() - i - 1)+"}");
            }

            writeLine("");
            
            // Write the typealias for the protocol composition
            namespaces[0] = "Overlay";
            namespaces.pop_back();
            namespaces.back() += "Subclass";
            #warning this might not properly handle enough levels of nesting
            for (size_t i = 0; i < namespaces.size(); i++) {
                if (i + 1 < namespaces.size()) {
                    writeLine(computeIndentation(i)+"extension "+namespaces[i]+" {");
                } else {
                    writeLine(computeIndentation(i)+"public typealias "+namespaces.back()+" = "+h.typeNames.fullyQualifiedSwiftAdapter+" & "+h.typeNames.fullyQualifiedPureVirtuals);
                }
            }
            for (size_t i = 1; i < namespaces.size(); i++) {
                writeLine(computeIndentation(namespaces.size() - i - 1)+"}");
            }
        }
        // Add extra space between each type in the for loop,
        // after the printer and guard blocker have been destroyed
        writeLines({
            "",
            "",
            "",
            "",
        });
    } // for (const clang::TagDecl* tagDecl : data)
}
std::vector<std::pair<std::string, SwiftSubclassCxxCodeGen::Data>> SwiftSubclassCxxCodeGen::writeDocCFile(std::string* outTitle, std::string* outOverview, const Data& processedData) {
    *outTitle = "OpenUSD types that can be subclassed in Swift";
    *outOverview =
    "This page lists which Usd types can be subclassed from Swift.\n"
    "Subclassing OpenUSD types in Swift should only be done for writing an OpenUSD plugin, "
    "due to an increased risk of memory-safety issues. See <doc:WritingAndUsingOpenUSDPlugins> for more information.";
    
    return {{"", processedData}};
}
