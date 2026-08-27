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
#include "CodeGen/SwiftSubclassCxxCodeGenHelpers.h"

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
 
 
 Shared or unsafe FRTs:
 If a subclass-enabled type derives from TfRefBase, instead of making the CxxAdapter a shared FRT (like the OpenUSD type itself), we make it an unsafe FRT. This is fairly unusual, but it _is_ the right approach:
 If the CxxAdapter is shared, then during SwiftAdater.init, after Swift creates the CxxAdapter and wires the two adapters together,
 Swift will drop the pointer to the CxxAdapter, performing a release. Since nothing in C++ has a TfRefPtr to
 the CxxAdapter at this point, the CxxAdapter would be destroyed during the SwiftAdapter.init, so the SwiftAdapter
 would immediately become a zombie before it could ever be used in a useful way. There's no good way to try to
 autorelease the CxxAdapter, so we could perform an unbalanced retain, and then use TfCreateRefPtr (which we would
 normally use anyways) to consume the unbalanced retain when handing the CxxAdapter to the OpenUSD plugin system.
 But: Since the SwiftAdapter has a weak pointer to the CxxAdapter, and FRTs don't support native Swift weak references,
 we'd need to use some non-owning reference. We don't want to use Overlay.WeakReferenceHolder because that requires
 conforming to Overlay._TfWeakBaseProtocol, which is way more ceremony than we need or want (remember, users shouldn't
 be using CxxAdapter under almost any circumstances), so we'd have to roll our own. It could be a void* that we
 constantly static_cast, but then we end up with refcount churn on every pass from Swift into C++. We could synthesize
 another forwarding wrapper type just so Swift can hold the CxxAdapter weakly, but that makes everything more complicated.
 
 If instead we just make the CxxAdapter be an unsafe FRT even when its base class is a shared FRT, then we completely
 disable Swift's ARC operations on it. This solves the zombie-during-init problem, and it means that it falls back
 to the unsafe CxxAdapter behavior in Swift, but can still be used via TfRefPtr in C++.
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
    // For now, we only support subclassing from HioImage and SdfFileFormat,
    // because subclassing is very experimental
    SwiftSubclassCxxCodeGen::Data result;
    for (const auto& it : data) {
        if (ASTHelpers::getAsString(it) != "class " PXR_NS"::HioImage" &&
            ASTHelpers::getAsString(it) != "class " PXR_NS"::SdfFileFormat") {
            continue;
        }
        result.push_back(it);
    }
    return result;
}

std::string computeIndentation(size_t x) {
    std::string result;
    for (size_t i = 0; i < x; i++) {
        result+= "    ";
    }
    return result;
}


void SwiftSubclassCxxCodeGen::writeHeaderFile(const Data& data) {
    // Includes that we need for various reasons
    writeLines({
        "#include <swift/bridging>",
        "#include \"pxr/usd/sdf/spec.h\"",
        "#include \"pxr/usd/sdf/layerHints.h\"",
        "#include \"swiftUsd/Wrappers/StdOstreamWrapper.h\"",
        "",
    });
    
    for (const clang::TagDecl* tagDecl : data) {
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            getTypeNameOpt<CppNameInCpp>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            NamesHelper names{this, tagDecl};
            AnalysisHelper analysis{this, tagDecl};
            
            writeLine("// MARK: "+names.types.tagSwiftNameInCpp+" subclassing");
            
            std::vector<std::string> namespaces = NamesHelper::computeQualifiedNameComponents(tagDecl);
            namespaces[0] = "__Overlay";
            for (size_t i = 0; i < namespaces.size(); i++) {
                writeLine(computeIndentation(i)+"namespace "+namespaces[i]+" {");
            }
            std::string indent = computeIndentation(namespaces.size());
            
            writeLines({
                indent+"class "+names.types.cxxAdapter+" final: public "+names.types.tagSwiftNameInCpp+" {",
                indent+"public:",
                indent+"    // Swift pointer",
                indent+"    typedef void*_Nullable SwiftSubclass;",
                indent+"    ",
                indent+"    SwiftSubclass "+names.fields.swiftSubclassPointer+" = nullptr;",
                indent+"    void (*_Nullable "+names.fields.releaseFP+")(SwiftSubclass) = nullptr;",
                indent+"    virtual ~"+names.types.cxxAdapter+"();",
                indent+"    ",
                indent+"    static void*_Nullable "+names.methods.toRaw+"("+names.types.cxxAdapter+"*_Nullable);",
                indent+"",
            });
            
            writeLine(indent+"    // Conditional downcasting");
            for (const clang::CXXRecordDecl* targetDecl : analysis.conditionalDowncastingTargets) {
                std::string targetName = names.types.basesSwiftNameInCpp.at(targetDecl);
                writeLines({
                    indent+"    static inline "+names.types.cxxAdapter+"*_Nullable "+names.methods.dynamicCast+"("+targetName+"*_Nullable p) {",
                    indent+"        return dynamic_cast<"+names.types.cxxAdapter+"*>(p);",
                    indent+"    }",
                });
            }
            if (!analysis.conditionalDowncastingTargets.empty()) { writeLine(indent+"    "); }
            
            
            writeLine(indent+"    // Unconditional upcasting");
            for (const auto& targetDecl : analysis.unconditionalUpcastingTargets) {
                std::string targetName = names.types.basesSwiftNameInCpp.at(targetDecl);
                writeLines({
                    // `unused` argument lets us overload on `__static_cast`, otherwise we'd be overloading on the return type
                    // which isn't allowed in C++
                    indent+"    static inline "+targetName+"*_Nonnull "+names.methods.staticCast+"("+names.types.cxxAdapter+"*_Nonnull p, "+targetName+"*_Nullable unused) {",
                    indent+"        return static_cast<"+targetName+"*>(p);",
                    indent+"    }"
                });
            }
            if (!analysis.unconditionalUpcastingTargets.empty()) { writeLine(indent+"    "); }
            
            
            if (analysis.destructorIsPublic) {
                writeLines({
                    indent+"    // Public destructor of "+names.types.tagSwiftNameInCpp+" will be exposed to Swift",
                    indent+"    static inline void "+names.methods.swiftDeleteBase+"("+names.types.tagSwiftNameInCpp+"*_Nonnull p) {",
                    indent+"        delete p;",
                    indent+"    }",
                    indent+"    ",
                });
            } else {
                writeLines({
                    indent+"    // Non-public destructor of "+names.types.tagSwiftNameInCpp+" will not be exposed to Swift",
                    indent+"    ",
                });
            }
            
            writeLine(indent+"    // Non-private constructors of "+names.types.tagSwiftNameInCpp+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysis.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                MethodHelper mh{this, ctor, names};
                writeLines({
                    indent+"    "+mh.signature_cppConstructorDeclaration + ";",
                    indent+"    "+mh.signature_cppSwiftNewDeclaration + ";",
                    indent+"    ",
                });
            }
            
            if (analysis.destructorIsNotPrivate) {
                writeLines({
                    indent+"    // Non-private destructor of "+names.types.cxxAdapter+" will be exposed to Swift",
                    indent+"    void "+names.methods.swiftDeleteCxxAdapter+"();",
                    indent+"    ",
                });
            } else {
                writeLines({
                    indent+"    // Private destructor of "+names.types.cxxAdapter+" will not be exposed to Swift",
                    indent+"    ",
                });
            }
            
            writeLine(indent+"    // Start total inheritance from "+names.types.tagSwiftNameInCpp);
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInCpp.at(record);
                
                writeLine(indent+"    // Start fields from "+targetName);
                for (const clang::FieldDecl* field : analysis.fieldsForInheritance(record)) {
                    bool succeeded;
                    std::string typeString = names.types.fieldCppNameInCpp(field, succeeded);
                    
                    writeLine(indent+"    "+" "+names.methods.fieldGetter(field)+"() const"+names.methods.fieldGetterSwiftReturnsUnretainedIfNeeded(field, this)+";");
                    if (!field->getType().isConstQualified()) {
                        writeLine(indent+"    void "+names.methods.fieldSetter(field)+"_set("+typeString+");");
                    }
                    writeLine(indent+"    ");
                }
                writeLines({
                    indent+"    // End fields from "+targetName,
                    indent+"    ",
                    indent+"    // Start methods from "+targetName,
                });
                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    TypesHelper th{this};
                    
                                        
                    // Templated methods write their body in the header file...
                    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
                        writeLines({
                            indent+"    "+mh.templateS,
                            indent+"    "+mh.signature_cppTemplateDeclaration + " {",
                            indent+"        return "+mh.signature_cppTemplateBody + ";",
                            indent+"    }",
                            indent+"    ",
                        });
                        continue;
                    }
                    
                    // All virtual methods get a function pointer that will point to a Swift implementation of it
                    if (method->isVirtual()) {
                        writeLine(indent+"    "+mh.signature_cppFunctionPointerDeclaration + ";");
                        
                        // Virtual methods with a default implementation expose that default implementation to Swift,
                        // in case Swift wants to call it
                        if (!method->isPureVirtual()) {
                            writeLine(indent+"    "+mh.signature_cppDefaultDeclaration + ";");
                        }
                    }
                    
                    // All non-virtual methods get a "forwarding version" that exposes them to Swift, because
                    // if they're inherited from a non-imported C++ type, Swift won't be able to access them
                    // otherwise. https://github.com/swiftlang/swift/issues/83114
                    if (!method->isVirtual()) {
                        writeLine(indent+"    "+mh.signature_cppForwardDeclaration+";");
                    }
                    
                    // Finally, all virtual methods get their actual inherited decl overridden. (This is the method
                    // that C++code that doesn't know anything about SwiftSubclassCxx tricks calls, just using
                    // bog-standard virtual dispatch in vanilla C++.)
                    if (method->isVirtual()) {
                        writeLine(indent+"    "+mh.signature_cppOriginalDeclaration+";");
                    }
                    
                    writeLine(indent+"    ");
                }
                writeLines({
                    indent+"    // End methods from "+names.types.tagSwiftNameInCpp,
                    indent+"    ",
                });
                
            } // for (const auto& recordFuturePair : inheritancePairs(tagDecl))
            writeLine(indent+"    // End total inheritance from "+names.types.tagSwiftNameInCpp);
            
            writeLine(indent+"} SWIFT_UNSAFE_REFERENCE;");
            
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
    writeLines({
        "// When constructing non-copyable wrapper types that get immediately passed to function pointers,",
        "// we need to pass the wrapper as const* and not const& due to",
        "// rdar://156635576 (Runtime crash calling C function pointer with const& argument from C++ when initialized with Swift closure).",
        "// We can't use `fp(&SomeObject());` because taking the address of a temporary is illegal in C++,",
        "// so we launder through this function. (Taking the address of a temporary is typically bad,",
        "// but in the case of non-copyables, those are exposed as `borrowing` parameters in Swift, so",
        "// Swift users can't escape the pointer/wrapper beyond the function pointer call, and C++ guarantees",
        "// that temporary objects persist for the duration of the statement that constructs them, which is the",
        "// function pointer call in `fp(__unsafeGetAddressOfTemporaryForFunctionPointer(SomeObject())`).",
        "",
        "template <typename T>",
        "const T* __unsafeGetAddressOfTemporaryForFunctionPointer(const T& x) {",
        "    return &x;",
        "}",
        "",
    });
    
    for (const clang::TagDecl* tagDecl : data) {
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            getTypeNameOpt<CppNameInCpp>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            NamesHelper names{this, tagDecl};
            AnalysisHelper analysis{this, tagDecl};
            
            writeLine("// MARK: "+names.types.tagSwiftNameInCpp+" subclassing");
            
            writeLines({
                names.types.fullyQualifiedCxxAdapter+"::~"+names.types.cxxAdapter+"() {",
                "    if (" + names.fields.swiftSubclassPointer + ") {",
                "        " + names.fields.releaseFP + "(" + names.fields.swiftSubclassPointer + ");",
                "        " + names.fields.swiftSubclassPointer + " = nullptr;",
                "        " + names.fields.releaseFP + " = nullptr;",
                "    }",
                "}",
                "",
                "/* static */ void* "+names.types.fullyQualifiedCxxAdapter+"::"+names.methods.toRaw+"("+names.types.fullyQualifiedCxxAdapter+"* x) {",
                "    return reinterpret_cast<void*>(x);",
                "}",
            });

            
            // Conditional downcasting is already handled by inline method in header
            
            // Unconditional upcasting is already handled by inline method in header
            
            // Public destructor of tagDeclName is already handled by inline method in header
            
            writeLine("// Non-private constructors of "+names.types.tagSwiftNameInCpp+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysis.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                MethodHelper mh{this, ctor, names};
                writeLines({
                    mh.signature_cppConstructorDefinition+":",
                    mh.signature_cppConstructorBodyInitializerList,
                    "{}",
                    mh.signature_cppSwiftNewDefinition+" {",
                    "    return "+mh.signature_cppSwiftNewBody + ";",
                    "}",
                });
            }
            writeLine("");
            
            if (analysis.destructorIsNotPrivate) {
                writeLines({
                    "// Non-private destructor of "+names.types.cxxAdapter+" will be exposed to Swift",
                    "void "+names.types.fullyQualifiedCxxAdapter+"::"+names.methods.swiftDeleteCxxAdapter+"() {",
                    "    delete this;",
                    "}",
                });
            }
            writeLines({
                "",
                "// Start total inheritance from "+names.types.tagSwiftNameInCpp,
            });
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInCpp.at(record);
                
                writeLine("// Start fields from "+targetName);
                for (const clang::FieldDecl* field : analysis.fieldsForInheritance(record)) {
                    bool succeeded;
                    std::string typeString = names.types.fieldCppNameInCpp(field, succeeded);
                    
                    writeLines({
                        names.types.fullyQualifiedCxxAdapter+"::"+names.methods.fieldGetter(field)+"() const "+names.methods.fieldGetterSwiftReturnsUnretainedIfNeeded(field, this)+"{",
                        "    return this->"+field->getNameAsString()+";",
                        "}",
                    });
                    if (!field->getType().isConstQualified()) {
                        writeLines({
                            names.types.fullyQualifiedCxxAdapter+"::"+names.methods.fieldSetter(field)+"_set("+typeString+" x) {",
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
                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    TypesHelper th{this};
                                        
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
                                mh.signature_cppDefaultDefinition+" {",
                                "    return "+mh.signature_cppDefaultBody + ";",
                                "}",
                            });
                        }
                    }
                    
                    // All non-virtual methods get a "forwarding version" that exposes them to Swift, because
                    // if they're inherited from a non-imported C++ type, Swift won't be able to access them
                    // otherwise. https://github.com/swiftlang/swift/issues/83114
                    if (!method->isVirtual()) {
                        writeLines({
                            mh.signature_cppForwardDefinition+" {",
                            "    return "+mh.signature_cppForwardBody + ";",
                            "}",
                        });
                    }
                    
                    // Finally, all virtual methods get their actual inherited decl overridden. (This is the method
                    // that C++code that doesn't know anything about SwiftSubclassCxx tricks calls, just using
                    // bog-standard virtual dispatch in vanilla C++.)
                    if (method->isVirtual()) {
                        writeLines({
                            mh.signature_cppOriginalDefinition + " {",
                            "    return "+mh.signature_cppOriginalBody + ";",
                            "}",
                        });
                    }

                    writeLine("");
                }
                writeLine("// End methods from "+targetName);
                
            } // for (const auto& recordFuturePair : inheritancePairs(tagDecl))
            writeLine("// End total inheritance from "+names.types.tagSwiftNameInCpp);

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
        "public enum " + NamesHelper::TypeNames::overlaySwiftEnum + "{}"
        "",
        "",
        "",
        "",
    });
    
    for (const clang::TagDecl* tagDecl : data) {
        // Use an extra local scope so we can make the blocker and printer go away
        // before the end of this for loop, because we want to print space between
        // different types of the for loop, and we want the printer feature flag guard
        // to go before that space
        {
            auto printer = typeNamePrinter(tagDecl);
            getTypeNameOpt<CppNameInCpp>(printer);
            auto blocker = typeNamePrinterGuardBlocker();
            NamesHelper names{this, tagDecl};
            AnalysisHelper analysis{this, tagDecl};

            writeLine("// MARK: "+names.types.tagSwiftNameInSwift+" subclassing");

            // Make it convenient to access the corresponding SwiftAdapter
            // of any CxxAdapter
            writeLines({
                "extension " + names.types.swiftFullyQualifiedCxxAdapter + " {",
                "    public var swift: " + names.types.swiftProtocolCompositionTypealias + " {",
                "        "+names.types.unmanagedSwiftAdapter+".fromOpaque("+names.fields.swiftSubclassPointer+"!).takeUnretainedValue() as! " + names.types.swiftProtocolCompositionTypealias,
                "    }",
                "}",
                "",
            });
            
            
            writeLine("// Conditional downcasting");
            for (const clang::CXXRecordDecl* targetDecl : analysis.conditionalDowncastingTargets) {
                std::string targetName = names.types.basesSwiftNameInSwift.at(targetDecl);
                
                writeLines({
                   "extension " + targetName + " {",
                    "    public func `as`<T: " + names.types.swiftProtocolCompositionTypealias + ">(_ t: T.Type = T.self) -> T? {",
                    "        " + names.types.swiftFullyQualifiedCxxAdapter + "." + names.methods.dynamicCast + "(self)?.swift as? T",
                    "    }",
                    "}",
                    "",
                });
            }
            if (!analysis.conditionalDowncastingTargets.empty()) { writeLine(""); }

            // Unconditional upcasting will be handled in the SwiftAdapter definition, which we haven't started yet
            
            if (analysis.destructorIsPublic) {
                writeLines({
                    "// Public destructor of "+names.types.tagSwiftNameInSwift+" will be exposed to Swift",
                    "",
                    "extension "+names.types.tagSwiftNameInSwift+" {",
                    "    /// Deletes the argument using `delete x;` in C++. ",
                    "    /// ",
                    "    /// The caller is responsible for ensuring that the argument",
                    "    /// is not used after this function returns.",
                    "    public static func delete(_ x: consuming "+names.types.tagSwiftNameInSwift+") {",
                    "        " + names.types.swiftFullyQualifiedCxxAdapter + "." + names.methods.swiftDeleteBase + "(x)",
                    "    }",
                    "}",
                    "",
                });
            } else {
                writeLines({
                    "// Non-public destructor of "+names.types.tagSwiftNameInSwift+" will not be exposed to Swift",
                    "",
                });
            }

            std::vector<std::string> namespaces = NamesHelper::computeQualifiedNameComponents(tagDecl);
            namespaces[0] = names.types.overlaySwiftEnum;
            namespaces.push_back(names.types.swiftAdapter);
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
                indent+"private typealias UnmanagedSelf = "+names.types.unmanagedSwiftAdapter,
                indent,
                indent+"// Detect if we're a zombie object to avoid setting "+names.fields.swiftDidDeinit+".pointee in deinit after "+names.fields.releaseFP+" deallocated it",
                indent+"private var "+names.fields.swiftIsZombie+": Bool",
                indent,
                indent+"// Detect whether or not a call to Unmanaged.release() deinitialized us",
                indent+"// or if there's an outstanding strong pointer, making us a zombie",
                indent+"private var "+names.fields.swiftDidDeinit+": UnsafeMutablePointer<Bool>",
                indent,
                indent+"// Cxx pointer",
                indent+"private var "+names.fields.cxxSubclassPointer+": "+names.types.swiftFullyQualifiedCxxAdapter+"!",
                indent,
                indent+"public func __get_cxxUnsafe() -> "+names.types.swiftFullyQualifiedCxxAdapter+"? { "+names.fields.cxxSubclassPointer+" }",
                indent+"public func __get_cxx_rawUnsafe() -> UnsafeMutableRawPointer? { "+names.types.swiftFullyQualifiedCxxAdapter+"."+names.methods.toRaw+"("+names.fields.cxxSubclassPointer+") }",
                indent,
            });
            
            writeLine(indent+"// Unconditional upcasting");
            for (const clang::CXXRecordDecl* targetDecl : analysis.unconditionalUpcastingTargets) {
                std::string targetName = names.types.basesSwiftNameInSwift.at(targetDecl);
                
                writeLines({
                    indent+"public func `as`(_ t: " + targetName + ".Type) -> " + targetName + " {",
                    indent+"    "+names.types.swiftFullyQualifiedCxxAdapter+"."+names.methods.staticCast+"("+names.fields.cxxSubclassPointer+", nil)",
                    indent+"}",
                });
            }
            if (!analysis.unconditionalUpcastingTargets.empty()) { writeLine(""); }

            // Important: We do our best to turn virtual methods that return values by indirection
            // into overriddable Swift methods that return values by copy. This is important for
            // memory safety, because otherwise Swift could easily return dangling pointers.
            // When Swift virtual implementations return values by copy that are by indirection in C++,
            // we copy the value into a pointer allocation, then return that pointer to give C++
            // a stable address.
            writeLine(indent+"// Pointer/ref-returning method allocations");
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInSwift.at(record);
                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    TypesHelper th{this};
                    
                    if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                        writeLine(indent+"private var "+names.fields.returnIndirectionAllocation(method)+": "+mh.swiftReturnIndirectionAllocationType);
                    }
                }
            }
            writeLine(indent);
            
            // The `wireToCxx` method is important but decently complicated to read and write, so
            // pull it out into a local scope for increased code clarity
            {
                writeLines({
                    indent+"private func "+names.methods.wireToCxx+"(_ " + names.fields.cxxSubclassPointer + ": " + names.types.swiftFullyQualifiedCxxAdapter + ") {",
                    indent+"    func slf(_ raw: UnsafeMutableRawPointer?) -> "+names.types.swiftProtocolCompositionTypealias + " {",
                    indent+"        UnmanagedSelf.fromOpaque(raw!).takeUnretainedValue() as! "+names.types.swiftProtocolCompositionTypealias,
                    indent+"    }",
                    indent+"    ",
                    indent+"    self."+names.fields.cxxSubclassPointer+" = "+names.fields.cxxSubclassPointer,
                    indent+"    "+names.fields.cxxSubclassPointer+"."+names.fields.swiftSubclassPointer+" = UnmanagedSelf.passRetained(self).toOpaque()",
                    indent+"    "+names.fields.cxxSubclassPointer+"."+names.fields.releaseFP+" = {",
                    indent+"        let swiftDidDeinitPointer = slf($0)."+names.fields.swiftDidDeinit,
                    indent+"        let cxxSubclassPointer = slf($0)."+names.fields.cxxSubclassPointer,
                    indent+"        slf($0)."+names.fields.cxxSubclassPointer+" = nil",
                    indent+"        UnmanagedSelf.fromOpaque($0!).release()",
                    indent+"        let isZombie = !swiftDidDeinitPointer.pointee",
                    indent+"        swiftDidDeinitPointer.deallocate()",
                    indent+"        if isZombie {",
                    indent+"            slf($0)."+names.fields.swiftIsZombie+" = true",
                    indent+"            let cppInstance: UnsafeMutableRawPointer? = "+names.types.swiftFullyQualifiedCxxAdapter+"."+names.methods.toRaw+"(cxxSubclassPointer)",
                    indent+"            __Overlay.SwiftSubclassCxx_zombieCreated(swiftInstance: $0, cppInstance: cppInstance, typeName: \""+names.types.tagSwiftNameInSwift+"\")",
                    indent+"        }",
                    indent+"    }",
                });
                
                for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                    const clang::CXXRecordDecl* record = recordFuturePair.first;
                    std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                    
                    std::string targetName = names.types.basesSwiftNameInSwift.at(record);
                    
                    writeLine(indent+"    // Start wire virtual methods from " + targetName);
                    for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                        // Pull out some common strings that we'll want to use regardless of the method kind
                        MethodHelper mh{this, method, names};
                        TypesHelper th{this};
                        
                        if (method->getAccess() == clang::AS_private) { continue; }
                        if (!method->isVirtual()) { continue; }
                        
                        if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                            
                            writeLines({
                                indent+"    "+names.fields.cxxSubclassPointer+"."+mh.functionPointerS + " = {",
                                indent+"        let newValue = "+mh.signature_swiftFunctionPointerBody,
                                indent+"        if slf($0)."+names.fields.returnIndirectionAllocation(method)+"?.pointee != newValue {",
                                indent+"            if let p = slf($0)."+names.fields.returnIndirectionAllocation(method)+" {",
                                indent+"                p.deinitialize(count: 1).deallocate()",
                                indent+"            }",
                                indent+"            slf($0)."+names.fields.returnIndirectionAllocation(method)+" = "+mh.swiftReturnIndirectionAllocationTypeNonOptional+".allocate(capacity: 1)",
                                indent+"            slf($0)."+names.fields.returnIndirectionAllocation(method)+"!.initialize(to: newValue)",
                                indent+"        }",
                                indent+"        return " + mh.swiftReturnIndirectionExpressionConversion("slf($0)."+names.fields.returnIndirectionAllocation(method)),
                                indent+"    }",
                            });
                        } else {
                            writeLine(indent+"    "+names.fields.cxxSubclassPointer+"."+mh.functionPointerS + " = { "+mh.signature_swiftFunctionPointerBody+" }");
                        }
                    }
                    writeLines({
                        indent+"    // End wire virtual methods from " + targetName,
                        indent,
                    });
                }
                
                writeLines({
                    indent+"} // public func "+names.methods.wireToCxx,
                    indent,
                });
            } // End of local scope for `wireToCxx`
            
            
            writeLines({
                indent+"deinit {",
                indent+"    // "+names.fields.releaseFP + ", which is run by the C++ destructor, sets "+names.fields.cxxSubclassPointer+" to nil.",
                indent+"    // CxxAdapter has a strong pointer to SwiftAdapter, so SwiftAdapter.deinit running before ~CxxAdapter is a user-induced memory-safety issue",
                indent+"    if self."+names.fields.cxxSubclassPointer+" != nil {",
                indent+"        fatalError(\"Swift deinit running before C++ destructor ran violates invariants. Did you overrelease the Swift subclass instance?\")",
                indent+"    }",
            });
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInSwift.at(record);
                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    TypesHelper th{this};
                    
                    if (th.doesMethodGetSwiftReturnIndirectionAllocation(method)) {
                        writeLine(indent+"    "+names.fields.returnIndirectionAllocation(method)+"?.deinitialize(count: 1).deallocate()");
                    }
                }
            }
            writeLines({
                indent+"    if !self."+names.fields.swiftIsZombie+" {",
                indent+"        // Set the flag but don't deallocate. The releaseFP will deallocate after checking if this flag was set",
                indent+"        self."+names.fields.swiftDidDeinit+".pointee = true",
                indent+"    }",
                indent+"} // deinit",
                indent,
            });
            

            writeLine(indent+"// Non-private constructors of "+names.types.tagSwiftNameInSwift+" will be exposed to Swift");
            for (const clang::CXXConstructorDecl* ctor : analysis.constructors) {
                if (ctor->getAccess() != clang::AS_public && ctor->getAccess() != clang::AS_protected) {
                    continue;
                }
                
                bool isProtected = ctor->getAccess() == clang::AS_protected;
                std::string methodName = isProtected ? "_newProtected" : "new";

                MethodHelper mh{this, ctor, names};
                writeLines({
                    indent+"",
                    indent+"/// Creates a new instance of this subclass using `new "+names.types.tagSwiftNameInCpp+"(...);` in C++.",
                    indent+"/// ",
                    indent+"/// The caller is responsible for deleting the instance in one of the following ways:",
                    indent+"/// - Using `"+names.types.swiftProtocolCompositionTypealias+".delete(_:)` in Swift",
                    indent+"/// - Using `"+names.types.tagSwiftNameInSwift+".delete(_:)` on the value returned by `self.get_cxx()`",
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
                    indent+mh.signature_swiftInitDefinition+" {",
                    indent+"    self."+names.fields.swiftIsZombie+" = false",
                    indent+"    self."+names.fields.swiftDidDeinit+" = UnsafeMutablePointer<Bool>.allocate(capacity: 1)",
                    indent+"    self."+names.fields.swiftDidDeinit+".initialize(to: false)",
                    indent+"",
                    indent+"    if type(of: self) == "+names.types.fullyQualifiedSwiftAdapter+".self {",
                    indent+"        fatalError(\"Cannot construct instance of abstract base class "+names.types.swiftProtocolCompositionTypealias+"\")",
                    indent+"    }",
                    indent+"    guard self is "+names.types.swiftProtocolCompositionTypealias+" else {",
                    indent+"        fatalError(\"Subclass \\(type(of: self)) of "+names.types.tagSwiftNameInSwift+" must inherit from "+names.types.swiftProtocolCompositionTypealias+"\")",
                    indent+"    }",
                    indent+"    let cxxAdapter = "+mh.signature_swiftInitBody,
                    indent+"    self."+names.methods.wireToCxx+"(cxxAdapter)",
                    indent+"}",
                    indent,
                });
            }
            
            if (analysis.destructorIsNotPrivate) {
                std::string methodName = analysis.destructorIsProtected ? "_deleteProtected" : "delete";
                
                // Important: Use a static method so that we can end the lifetime of the argument before the end of the method,
                // to avoid false-positives for zombie creation detection
                writeLines({
                    indent+"// Non-private destructor of "+names.types.cxxAdapter+" will be exposed to Swift",
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
                });
                if (analysis.destructorIsProtected) {
                    writeLines({
                        indent+"/// ",
                        indent+"/// This destructor was originally protected in C++. Use with caution."
                    });
                }
                writeLines({
                    indent+"public static func "+methodName+"(_ swiftAdapter: consuming "+names.types.fullyQualifiedSwiftAdapter+") {",
                    indent+"    let cxxAdapter = swiftAdapter."+names.fields.cxxSubclassPointer,
                    indent+"    _ = consume swiftAdapter",
                    indent+"    cxxAdapter?."+names.methods.swiftDeleteCxxAdapter+"()",
                    indent+"}",
                    indent,

                });
            } else {
                writeLines({
                    indent+"// Private destructor of "+names.types.cxxAdapter+" will not be exposed to Swift",
                    indent,
                });
            }
            
            writeLine(indent+"// Start total inheritance from " + names.types.tagSwiftNameInSwift);
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInCpp.at(record);
                
                writeLine(indent+"// Start fields from "+targetName);
                for (const clang::FieldDecl* field : analysis.fieldsForInheritance(record)) {
                    bool succeeded;
                    std::string typeString = names.types.fieldSwiftNameInSwift(field, succeeded);


                    writeLines({
                        indent+"public var " + field->getNameAsString() + ": " + typeString + "{",
                        indent+"    get { "+names.fields.cxxSubclassPointer+"."+names.methods.fieldGetter(field)+"() }",
                    });
                    if (!field->getType().isConstQualified()) {
                        // set { cxx.`field`_set(newValue) }
                        writeLine(indent+"    set { "+names.fields.cxxSubclassPointer+"."+names.methods.fieldSetter(field)+"(newValue) }");
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

                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    
                    if (method->getAccess() == clang::AS_private) { continue; }
                    if (method->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) { continue; }
                    if (method->isPureVirtual()) { continue; }
                    
                    writeLines({
                        indent+mh.signature_swiftFunctionDeclaration+" {",
                        indent+"    return "+mh.signature_swiftFunctionBody,
                        indent+"}",
                    });
                }
                writeLine(indent+"// End methods from "+targetName);
            }
            writeLines({
                indent+"// End total inheritance from " + names.types.tagSwiftNameInSwift,
                indent,
            });
            
            
            writeLine(indent+"public protocol PureVirtuals {");
            for (const auto& recordFuturePair : analysis.baseAndFutureInheritancePairs) {
                const clang::CXXRecordDecl* record = recordFuturePair.first;
                std::vector<const clang::CXXRecordDecl*> future = recordFuturePair.second;
                
                std::string targetName = names.types.basesSwiftNameInSwift.at(record);
                
                writeLine(indent+"    // Start methods from "+targetName);
                if (record == tagDecl && analysis.isPluginEntryPoint) {
                    // We don't have to worry about this being a duplicate declaration with a pure virtual method,
                    // because C++ constructors can't be pure virtual
                    writeLine(indent+"    init() // Synthesized, required for plugin entry points");
                }
                
                for (const clang::CXXMethodDecl* method : analysis.methodsForInheritance(record, future)) {
                    // Pull out some common strings that we'll want to use regardless of the method kind
                    MethodHelper mh{this, method, names};
                    if (method->isPureVirtual()) {
                        writeLine(indent+"    "+mh.signature_swiftProtocolRequirementDeclaration);
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
                    writeLine(computeIndentation(i)+"public typealias "+namespaces.back()+" = "+names.types.fullyQualifiedSwiftAdapter+" & "+names.types.fullyQualifiedPureVirtuals);
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
