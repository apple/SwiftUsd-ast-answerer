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

#ifndef SwiftSubclassCxxCodeGenHelpers_h
#define SwiftSubclassCxxCodeGenHelpers_h

#include "CodeGen/SwiftSubclassCxxCodeGen.h"

/// Helper for reasoning about types (not concerned with how they're printed)
struct TypesHelper {
    TypesHelper(const SwiftSubclassCxxCodeGen* codeGen);
    
    static bool isConstRef(clang::QualType q);
    static clang::QualType removingConstRefIfPossible(clang::QualType q);
    static clang::QualType convertingConstRefToConstStar(clang::QualType q, const clang::FunctionDecl* f);
    
    bool isValueTypeIndirection(clang::QualType q) const;
    clang::QualType removingValueTypeIndirectionIfPossible(clang::QualType q) const;
    bool doesMethodGetSwiftReturnIndirectionAllocation(const clang::CXXMethodDecl* method) const;
    clang::QualType getSwiftReturnIndirectionAllocationType(const clang::CXXMethodDecl* method) const;
    std::string convertSwiftReturnIndirectionAllocationCxxFpToCxxVirtual(const clang::CXXMethodDecl* method, std::string expr) const;
    bool isImplicitlyIndirectFRT(clang::QualType q, bool* isShared = nullptr) const;
    
private:
    bool _removeValueTypeIndirectionIfPossible(clang::QualType& q) const;
    
private:
    const SwiftSubclassCxxCodeGen* _codeGen;
};

/// Helper for working with the names of declarations
struct NamesHelper {
    struct TypeNames {
        std::string tagSwiftNameInCpp; // The OpenUSD type we're subclassing
        std::string tagSwiftNameInSwift; // The OpenUSD type we're subclassing
        
        std::string cxxAdapter = "CxxAdapter";
        std::string fullyQualifiedCxxAdapter;
        
        std::string swiftAdapter = "SwiftAdapter";
        std::string swiftFullyQualifiedCxxAdapter; // fullyQualifiedCxxAdapter for use in Swift
        std::string fullyQualifiedPureVirtuals;
        std::string swiftProtocolCompositionTypealias; // (SwiftAdapter & PureVirtuals) type
        std::string fullyQualifiedSwiftAdapter;
        std::string unmanagedSwiftAdapter; // Unmanaged<fullyQualifiedSwiftAdapter>
        
        std::map<const clang::CXXRecordDecl*, std::string> basesSwiftNameInCpp;
        std::map<const clang::CXXRecordDecl*, std::string> basesSwiftNameInSwift;
        
        std::string fieldCppNameInCpp(const clang::FieldDecl*, bool& succeeded) const;
        std::string fieldSwiftNameInSwift(const clang::FieldDecl*, bool& succeeded) const;
        
        static std::string overlaySwiftEnum; // Workaround for rdar://156631442 (Public typealias to Swift type defined in C++ namespace extension not visible across module boundary)
        
        TypeNames(SwiftSubclassCxxCodeGen* codeGen);
        
    private:
        SwiftSubclassCxxCodeGen* _codeGen;
    };
    struct FieldNames {
        std::string swiftSubclassPointer = "__swiftSubclass";
        std::string releaseFP = "__releaseSwiftSubclass_FP";
        std::string cxxSubclassPointer = "__cxxSubclass";
        std::string swiftIsZombie = "__swiftIsZombie";
        std::string swiftDidDeinit = "__swiftDidDeinit";

        std::string returnIndirectionAllocation(const clang::CXXMethodDecl* method) const;
    };
    struct MethodNames {
        std::string dynamicCast = "__dynamic_cast";
        std::string staticCast = "__static_cast";
        std::string swiftDeleteBase = "__swiftDeleteBase";
        std::string swiftNew = "__swiftNew";
        std::string swiftDeleteCxxAdapter = "__swiftDeleteCxxAdapter";
        std::string wireToCxx = "__wireToCxx";
        std::string toRaw = "__toRaw";

        std::string fieldGetter(const clang::FieldDecl* f) const;
        std::string fieldSetter(const clang::FieldDecl* f) const;
        std::string fieldGetterSwiftReturnsUnretainedIfNeeded(const clang::FieldDecl* f, const SwiftSubclassCxxCodeGen* codeGen) const;
    };
    
    NamesHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::TagDecl* tagDecl);
        
    static std::vector<std::string> computeQualifiedNameComponents(const clang::TagDecl*);
    static std::string joinQualifiedNameComponents(std::vector<std::string> components, std::string separator);
    
    TypeNames types;
    FieldNames fields;
    MethodNames methods;
    
    
private:
    SwiftSubclassCxxCodeGen* _codeGen;
    const clang::CXXRecordDecl* _cxxRecordDecl;
    SwiftSubclassCxxAnalysisResult _analysisResult;
};

/// Helper for working with high level analysis results
struct AnalysisHelper {
    AnalysisHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::TagDecl* tagDecl);
    
    std::vector<const clang::CXXRecordDecl*> conditionalDowncastingTargets;
    std::vector<const clang::CXXRecordDecl*> unconditionalUpcastingTargets;
    using BaseAndFutureInheritancePair = std::pair<const clang::CXXRecordDecl*, std::vector<const clang::CXXRecordDecl*>>;
    std::vector<BaseAndFutureInheritancePair> baseAndFutureInheritancePairs;
    
    bool destructorIsPublic;
    bool destructorIsProtected;
    bool destructorIsNotPrivate;
    std::vector<const clang::CXXConstructorDecl*> constructors;
    bool isPluginEntryPoint;
    
    std::vector<const clang::FieldDecl*> fieldsForInheritance(const clang::CXXRecordDecl* x);
    std::vector<const clang::CXXMethodDecl*> methodsForInheritance(const clang::CXXRecordDecl* current, std::vector<const clang::CXXRecordDecl*> future);
    
private:
    SwiftSubclassCxxCodeGen* _codeGen;
    const clang::CXXRecordDecl* _cxxRecordDecl;
    SwiftSubclassCxxAnalysisResult _analysisResult;
};

/// Helper for dealing with all of the complexity of declaring, defining, and calling methods in
/// Swift and C++
struct MethodHelper {
    bool hadErrorPrintingTypeNames;

    std::string templateS; // `template <T1, T2>` prefix as needed
    std::string functionPointerS; // The name of the function pointer for virtual wiring
        
    std::string swiftSharedFRTReturnConvention; // empty for non-FRT returns, `SWIFT_RETURNS_RETAINED` or `SWIFT_RETURNS_UNRETAINED` for implicitly indirect FRT returns
    std::string swiftReturnIndirectionAllocationType; // UnsafeMutablePointer allocation type for virtual methods that return value types indirectly
    std::string swiftReturnIndirectionAllocationTypeNonOptional;
    std::string swiftReturnIndirectionExpressionConversion(std::string expr) const;
    
    // There are several different "calling styles" (not exactly calling
    // conventions) that we need to be mindful of here. For example,
    // C++ function declarations that override the original inherited
    // functions need to match the original function declaration exactly,
    // whereas Swift function declarations want to hide const& from users.
    // There isn't really a way to write a clean algorithm or heuristic for
    // this stuff, unfortunately.
    //
    // - Declaration: the forward declaration that appears in the class definition in the
    //   C++ header file or Swift file
    // - Definition: the redeclaration-and-definition that appears in the C++ .cpp file
    // - Body: the statements and expressions that occur within the function definition
    // Declarations and definitions are signatures, but bodies are executable code
    //
    // Declare and assign all of these with an error string,
    // so if we add one and forget to initialize it, there'll be
    // an obvious error in the codegen output
#define DECLARE_DEFAULTED_SIGNATURE(X) \
    std::string X = "$ERR$" #X "$"
    DECLARE_DEFAULTED_SIGNATURE(signature_cppOriginalDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppOriginalDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppOriginalBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppForwardDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppForwardDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppForwardBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppDefaultDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppDefaultDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppDefaultBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppFunctionPointerDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftFunctionPointerBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftFunctionDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftFunctionBody);
    
    DECLARE_DEFAULTED_SIGNATURE(signature_cppConstructorDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppConstructorDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppConstructorBodyInitializerList);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppSwiftNewDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppSwiftNewDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppSwiftNewBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftInitDefinition);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftInitBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppTemplateDeclaration);
    DECLARE_DEFAULTED_SIGNATURE(signature_cppTemplateBody);
    DECLARE_DEFAULTED_SIGNATURE(signature_swiftProtocolRequirementDeclaration);
#undef DECLARE_DEFAULTED_SIGNATURE
    
    MethodHelper(SwiftSubclassCxxCodeGen* codeGen, const clang::CXXMethodDecl* method,
                 NamesHelper const& namesHelper);
    
    /// Used to build all the different flavors/styles of a signature for a given method.
    struct AbstractSignature {
        AbstractSignature(SwiftSubclassCxxCodeGen* codeGen,
                          const clang::CXXMethodDecl* method,
                          NamesHelper const& namesHelper);
        
        // Transformations are applied to a signature
        // to change its flavor/style, and then it is
        // printed according to the rules of that style
        enum TransformationMode {
            transformationAdd,
            transformationUse,
            transformationRemove,
            transformationNone
        };
        
        void setWrappedTypesMode(TransformationMode);
        void setPointersAsNeededMode(TransformationMode);
        
        void closureIndexParams();
        void indexParams();
        void addSubclassParameter();
        void removeConstRefAsNeeded();
        void makeForward();
        void makeDefault();
        void makeCppNewOrSwiftInit();
        void callFP();
        void callOriginal();
        void callBase();
        void callForwardOrDefault();
        
        /// Represents a type in the signature of a method (parameter or return).
        /// May represent the original, unaltered type, or a modification or even
        /// custom type
        struct CurrentType {
            TypesHelper typesHelper;
            
            clang::QualType originalQualType;
            clang::QualType qualType;
            std::vector<std::string> customType;
            bool isCustomTypeCopyable;
            
            static CurrentType withQualType(clang::QualType, TypesHelper typesHelper);
            static CurrentType withCustom(std::vector<std::string>, bool isCustomTypeCopyable, TypesHelper typesHelper);
            
            TransformationMode wrappedTypesMode = transformationNone;
            TransformationMode pointersAsNeededMode = transformationNone;
            bool didRemoveConstRefAsNeeded = false;
            bool isReturnValue = false;
            
            std::string convert(std::string expr, bool inCpp) const;
            
        private:
            CurrentType(TypesHelper typesHelper, clang::QualType qualType, std::vector<std::string> customType, bool isCustomTypeCopyable);
        };
        
        enum PrintingPurpose {
            declaration, definition, body, functionPointer
        };
        
        // printing
        std::string printCpp(PrintingPurpose printingPurpose) const;
        std::string printSwift(PrintingPurpose printingPurpose) const;
        
    private:
        std::string print(CurrentType, bool inCpp) const;
        
        template <typename Transform>
        void transformTypes(Transform transform);
        
        template <typename Transform>
        void transformLabels(Transform transform);
                
    private:
        bool _isCppNewOrSwiftInit = false;
        
        // Information about the current state of this signature,
        // which may be different from the original state after
        // transformations are applied
        CurrentType _currentReturnType;
        std::string _currentFunctionName;
        std::vector<std::pair<std::string, CurrentType>> _currentParameters;
        
        // The original, untransformed state of the signature
        clang::QualType _originalReturnType;
        std::string _originalFunctionName;
        std::vector<std::pair<std::string, clang::QualType>> _originalParameters;
        
        SwiftSubclassCxxCodeGen* _codeGen;
        const clang::CXXMethodDecl* _method;
        NamesHelper const& _namesHelper;
    };

private:
    SwiftSubclassCxxCodeGen* _codeGen;
    const clang::CXXMethodDecl* _method;
    NamesHelper const& _namesHelper;
};

#endif /* SwiftSubclassCxxCodeGenHelpers_h */
