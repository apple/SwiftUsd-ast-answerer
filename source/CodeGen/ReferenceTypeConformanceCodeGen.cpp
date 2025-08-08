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

#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

ReferenceTypeConformanceCodeGen::ReferenceTypeConformanceCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<ReferenceTypeConformanceCodeGen>(codeGenRunner) {}


std::string ReferenceTypeConformanceCodeGen::fileNamePrefix() const {
    return "ReferenceTypeConformances";
}

ReferenceTypeConformanceCodeGen::Data ReferenceTypeConformanceCodeGen::preprocess() {
    ReferenceTypeConformanceCodeGen::Data result;
    for (const auto& it : getImportAnalysisPass()->getData()) {
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (it.second.isImportedAsAnyReference() && hasTypeName<SwiftNameInSwift>(itFirst)) {
            result.push_back(itFirst);
        }
    }
    return result;
}

bool ReferenceTypeConformanceCodeGen::isTfRefBaseSubclass(const clang::TagDecl* tagDecl) const {
    const clang::CXXRecordDecl* tfRefBase = clang::dyn_cast<clang::CXXRecordDecl>(getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfRefBase"));
    return ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(clang::dyn_cast<clang::CXXRecordDecl>(tagDecl), tfRefBase);
}
bool ReferenceTypeConformanceCodeGen::isTfWeakBaseSubclass(const clang::TagDecl* tagDecl) const {
    const clang::CXXRecordDecl* tfWeakBase = clang::dyn_cast<clang::CXXRecordDecl>(getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfWeakBase"));
    return ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(clang::dyn_cast<clang::CXXRecordDecl>(tagDecl), tfWeakBase);
}
bool ReferenceTypeConformanceCodeGen::isTfSingletonImmortalSpecialization(const clang::TagDecl* tagDecl) const {
    const clang::ClassTemplateDecl* tfSingleton = clang::dyn_cast<clang::ClassTemplateDecl>(getImportAnalysisPass()->findNamedDecl("template <class T> class " PXR_NS"::TfSingleton"));
    if (!tfSingleton) {
        __builtin_trap();
    }
    for (const clang::ClassTemplateSpecializationDecl* specialization : tfSingleton->specializations()) {
        const clang::TemplateArgument& templateArg = specialization->getTemplateInstantiationArgs().asArray()[0];
        if (templateArg.getAsType()->getAsTagDecl()->getDefinition() == tagDecl) {
            return true;
        }
    }
    return false;
}
bool ReferenceTypeConformanceCodeGen::isExactlyTfRefBase(const clang::TagDecl* tagDecl) const {
    const clang::CXXRecordDecl* tfRefBase = clang::dyn_cast<clang::CXXRecordDecl>(getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfRefBase"));
    return tagDecl == tfRefBase;
}

const ReferenceTypeConformanceCodeGen::RefTypeNameHelper ReferenceTypeConformanceCodeGen::nameHelper(TypeNamePrinter& p) {
    RefTypeNameHelper result;
    
    result.swiftNameInSwift = getTypeName<SwiftNameInSwift>(p);
    result.swiftNameInCpp = getTypeName<SwiftNameInCpp>(p);
    result.mangledName = mangleName(p.getType().getNamedDecl());
    result.frtTypedef = "__SwiftUsd_Typedef_" + result.mangledName;
    result.refPtrInCpp = "pxr::TfRefPtr<" + result.swiftNameInCpp + ">";
    result.refPtrTypedef = result.frtTypedef + "_RefPtr";
    result.weakPtrInCpp = "pxr::TfWeakPtr<" + result.swiftNameInCpp + ">";
    result.weakPtrTypedef = result.frtTypedef + "_WeakPtr";
    result.constRefPtrInCpp = "pxr::TfRefPtr<const " + result.swiftNameInCpp + ">";
    result.constRefPtrTypedef = result.frtTypedef + "_ConstRefPtr";
    result.constWeakPtrInCpp = "pxr::TfWeakPtr<const " + result.swiftNameInCpp + ">";
    result.constWeakPtrTypedef = result.frtTypedef + "_ConstWeakPtr";
    
    return result;
}

void ReferenceTypeConformanceCodeGen::writeHeaderFile(const ReferenceTypeConformanceCodeGen::Data& data) {
    // Make sure we include TfRefPtr, TfWeakPtr, and TfAnyWeakPtr
    
    const clang::TagDecl* usdStageRefPtr = getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfRefPtr<class " PXR_NS"::UsdStage>");
    const clang::TagDecl* usdStageWeakPtr = getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfWeakPtr<class " PXR_NS"::UsdStage>");
    const clang::TagDecl* tfAnyWeakPtr = getImportAnalysisPass()->findTagDecl("class " PXR_NS"::TfAnyWeakPtr");
    writeIncludeLines({usdStageRefPtr, usdStageWeakPtr, tfAnyWeakPtr});
    writeLines({
        "#include \"pxr/base/tf/retainReleaseHelper.h\"",
        "#include <swift/bridging>",
        "#include \"swiftUsd/SwiftOverlay/FRTProtocols.h\"",
        "#include \"swiftUsd/SwiftOverlay/SwiftCxxMacros.h\"",
    });
    
    for (const clang::TagDecl* tagDecl : data) {
        auto printer = typeNamePrinter(tagDecl);
        auto names = nameHelper(printer);
        
        // TfRefBase needs to be special cased in some places, because
        // TfRefPtr<TfRefBase> is disallowed
        
        if (!isTfRefBaseSubclass(tagDecl) && !isTfWeakBaseSubclass(tagDecl) && !isTfSingletonImmortalSpecialization(tagDecl)) {
            std::cout << ASTHelpers::getAsString(tagDecl) << std::endl;
            __builtin_trap();
        }
        
        // Make unnested typedefs, for SWIFT_NAME purposes
        writeLines({
            "",
            "typedef " + names.swiftNameInCpp + " " + names.frtTypedef + ";",
        });
        if (isTfRefBaseSubclass(tagDecl)) {
            writeLines({
                "typedef " + names.refPtrInCpp + " " + names.refPtrTypedef + ";",
                "typedef " + names.constRefPtrInCpp + " " + names.constRefPtrTypedef + ";",
            });
        }
        if (isTfWeakBaseSubclass(tagDecl)) {
            writeLines({
                "typedef " + names.weakPtrInCpp + " " + names.weakPtrTypedef + ";",
                "typedef " + names.constWeakPtrInCpp + " " + names.constWeakPtrTypedef + ";",
            });
        }
        
        // Write the `var _address: UnsafeMutableRawPointer` computed property
        writeLines({
            "void* _Nonnull _address(" + names.swiftNameInCpp + "* _Nonnull)",
            "    SWIFT_NAME(getter:" + names.frtTypedef + "._address(self:));",
        });
        
        if (isTfRefBaseSubclass(tagDecl)) {
            // Write the retain and release functions
            writeLines({
                "inline void __retain" + names.mangledName + "(" + names.swiftNameInCpp + "* _Nonnull x) {",
                "    pxr::Tf_RetainReleaseHelper::retain(x);",
                "}",
                "inline void __release" + names.mangledName + "(" + names.swiftNameInCpp + "* _Nonnull x) {",
                "    pxr::Tf_RetainReleaseHelper::release(x);",
                "}",
            });
            
            // Write the `func _asRefPtrType() -> Self.TfRefPtrType` method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.refPtrInCpp + " _asRefPtrType(" + names.swiftNameInCpp + "* _Nonnull)",
                    "    SWIFT_NAME(" + names.frtTypedef + "._asRefPtrType(self:));",
                });
            }
            // Write the `static func _fromRefPtrType(_ x: Self.TfRefPtrType) -> Self?` static method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromRefPtrType(const " + names.refPtrInCpp + "&)",
                    "    SWIFT_RETURNS_RETAINED",
                    "    SWIFT_NAME(" + names.frtTypedef + "._fromRefPtrType(_:));",
                });
            }
            // Write the `static func _fromConstRefPtrType(_ x: Self.TfConstRefPtrType) -> Self?` static method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromConstRefPtrType(const " + names.constRefPtrInCpp + "&)",
                    "    SWIFT_RETURNS_RETAINED",
                    "    SWIFT_NAME(" + names.frtTypedef + "._fromConstRefPtrType(_:));",
                });
            }
            // Write the `static func _fromRawPointer(_ p: UnsafeMutableRawPointer) -> SelfType?` static method
            writeLines({
                names.swiftNameInCpp + " * _Nullable _fromRawPointer" + names.mangledName + "(void* _Nullable)",
                "    SWIFT_RETURNS_RETAINED",
                "    SWIFT_NAME(" + names.frtTypedef + "._fromRawPointer(_:));",
            });
            // Write the `static func _nullPtr() -> Self` static method on `Self.TfRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.refPtrInCpp + "_nullRefPtr" + names.mangledName + "()",
                    "    SWIFT_NAME(" + names.refPtrTypedef + "._nullPtr());",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.refPtrInCpp + " &)",
                    "    SWIFT_NAME(" + names.refPtrTypedef + "._isNonnull(self:));",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfConstRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.constRefPtrInCpp + " &)",
                    "    SWIFT_NAME(" + names.constRefPtrTypedef + "._isNonnull(self:));",
                });
            }
        } // if isTfRefBaseSubclass(tagDecl)
        
        if (isTfWeakBaseSubclass(tagDecl)) {
            // Write the `func _asWeakPtrType() -> Self.TfWeakPtrType` method function
            writeLines({
                names.weakPtrInCpp + " _asWeakPtrType(" + names.swiftNameInCpp + "* _Nonnull)",
                "    SWIFT_NAME(" + names.frtTypedef + "._asWeakPtrType(self:));",
            });
            // Write the `static func _fromWeakPtrType(_ x: Self.TfWeakPtrType) -> Self?` static method
            writeLines({
                names.swiftNameInCpp + " * _Nullable _fromWeakPtrType(const " + names.weakPtrInCpp + "&) ",
                (isTfRefBaseSubclass(tagDecl) ? "    SWIFT_RETURNS_RETAINED" : "    // SWIFT_RETURNS_RETAINED is an error on immortal types"),
                "    SWIFT_NAME(" + names.frtTypedef + "._fromWeakPtrType(_:));",
            });
            // Write the `static func _fromConstWeakPTrType(_ x: Self.TfConstWeakPtrType) -> Self?` static method
            writeLines({
                names.swiftNameInCpp + " * _Nullable _fromConstWeakPtrType(const " + names.constWeakPtrInCpp + " &) ",
                (isTfRefBaseSubclass(tagDecl) ? "    SWIFT_RETURNS_RETAINED" : "    // SWIFT_RETURNS_RETAINED is an error on immortal types"),
                "    SWIFT_NAME(" + names.frtTypedef + "._fromConstWeakPtrType(_:));",
            });
            // Write the `func _asAnyWeakPtr() -> pxr.TfAnyWeakPtr` method on `Self.TfWeakPtrType`
            writeLines({
                "pxr::TfAnyWeakPtr _asAnyWeakPtr(const " + names.weakPtrInCpp + "&)",
                "    SWIFT_NAME(" + names.weakPtrTypedef + "._asAnyWeakPtr(self:));",
            });
            // Write the `static func _fromAnyWeakPtr(_ x: pxr.TfAnyWeakPtr) -> Self` static method on `Self.TfWeakPtrType`
            writeLines({
                names.weakPtrInCpp + "_fromAnyWeakPtr" + names.mangledName + "(const pxr::TfAnyWeakPtr&)",
                "    SWIFT_NAME(" + names.weakPtrTypedef + "._fromAnyWeakPtr(_:));",
            });
            // Write the `static func _nullPtr() -> Self` static method on `Self.TfWeakPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.weakPtrInCpp + "_nullWeakPtr" + names.mangledName + "()",
                    "    SWIFT_NAME(" + names.weakPtrTypedef + "._nullPtr());",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfWeakPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.weakPtrInCpp + " &)",
                    "    SWIFT_NAME(" + names.weakPtrTypedef + "._isNonnull(self:));",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfConstWeakPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.constWeakPtrInCpp + " &)",
                    "    SWIFT_NAME(" + names.constWeakPtrTypedef + "._isNonnull(self:));",
                });
            }
        } // if isTfWeakBaseSubclass(tagDecl)
    }
}

void ReferenceTypeConformanceCodeGen::writeCppFile(const ReferenceTypeConformanceCodeGen::Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        auto printer = typeNamePrinter(tagDecl);
        const auto names = nameHelper(printer);
        
        // Write the `var _address: UnsafeMutableRawPointer` computed property
        writeLines({
            "",
            "void* _Nonnull _address(" + names.swiftNameInCpp + "* _Nonnull x) {",
            "    return reinterpret_cast<void*>(x);",
            "}",
        });

        if (isTfRefBaseSubclass(tagDecl)) {
            // Write the `func _asRefPtrType() -> Self.TfRefPtrType` method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.refPtrInCpp + " _asRefPtrType(" + names.swiftNameInCpp + "* _Nonnull x) {",
                    "    return " + names.refPtrInCpp + "(x);",
                    "}",
                });
            }
            // Write the `static func _fromRefPtrType(_ x: Self.TfRefPtrType) -> Self?` static method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromRefPtrType(const " + names.refPtrInCpp + "& x) {",
                    "    if (!x) { return nullptr; }",
                    "    pxr::Tf_RetainReleaseHelper::retain(x.operator->());",
                    "    return x.operator->();",
                    "}",
                });
            }
            // Wrie the `static func _fromConstRefPtrType(_ x: Self.TfConstRefPtrType) -> Self?` static method
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromConstRefPtrType(const " + names.constRefPtrInCpp + "& x) {",
                    "    if (!x) { return nullptr; }",
                    "    " + names.swiftNameInCpp + "* result = const_cast<" + names.swiftNameInCpp + " *>(x.operator->());",
                    "    pxr::Tf_RetainReleaseHelper::retain(result);",
                    "    return result;",
                    "}",
                });
            }
            // Write the `static func _fromRawPointer(_ p: UnsafeMutableRawPointer) -> SelfType?` static method
            writeLines({
                names.swiftNameInCpp + " * _Nullable _fromRawPointer" + names.mangledName + "(void* _Nullable p) {",
                "    return __Overlay::dynamic_cast_raw_to_frt<" + names.swiftNameInCpp + ">(p);",
                "}",
            });
            // Write the `static func _nullPtr() -> Self` static method on `Self.TfRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    names.refPtrInCpp + "_nullRefPtr" + names.mangledName + "() {",
                    "    return " + names.refPtrInCpp + "(nullptr);",
                    "}",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.refPtrInCpp + " & p) {",
                    "    return (bool)p;",
                    "}",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfConstRefPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.constRefPtrInCpp + " & p) {",
                    "    return (bool)p;",
                    "}",
                });
            }
        } // if isTfRefBaseSubclass(tagDecl)
        
        if (isTfWeakBaseSubclass(tagDecl)) {
            // Write the `func _asWeakPtrType() -> Self.TfWeakPtrType` method function
            writeLines({
                names.weakPtrInCpp + " _asWeakPtrType(" + names.swiftNameInCpp + "* _Nonnull x) {",
                "    return " + names.weakPtrInCpp + "(x);",
                "}",
            });
            // Write the `static func _fromWeakPtrType(_ x: Self.TfWeakPtrType) -> Self?` static method
            if (isTfRefBaseSubclass(tagDecl)) {
                writeLines({
                    // Important: Turn it into a ref ptr and then use its dereferencing logic.
                    // Checking the pointer's validity, then incrementing the ref-count for valid pointers,
                    // could have a race condition if the pointer is deleted between the check and the increment. 
                    names.swiftNameInCpp + " * _Nullable _fromWeakPtrType(const " + names.weakPtrInCpp + "& x) {",
                    "    return _fromRefPtrType(" + names.refPtrInCpp + "(x));",
                    "}",
                });
            } else {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromWeakPtrType(const " + names.weakPtrInCpp + "& x) {",
                    "    if (!x) { return nullptr; }",
                    "    return x.operator->();",
                    "}",
                });
            }
            // Write the `static func _fromConstWeakPtrType(_ x: Self.TfConstWeakPtrType) -> Self?` static method
            if (isTfRefBaseSubclass(tagDecl)) {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromConstWeakPtrType(const " + names.constWeakPtrInCpp + " & x) {",
                    "    if (!x) { return nullptr; }",
                    "    " + names.swiftNameInCpp + " * result = const_cast<" + names.swiftNameInCpp + " *>(x.operator->());",
                    "    pxr::Tf_RetainReleaseHelper::retain(result);",
                    "    return result;",
                    "}",
                });
            } else {
                writeLines({
                    names.swiftNameInCpp + " * _Nullable _fromConstWeakPtrType(const " + names.constWeakPtrTypedef + "& x) {",
                    "    if (!x) { return nullptr; }",
                    "    return const_cast<" + names.swiftNameInCpp + " *>(x.operator->());",
                    "}",
                });
            }
            // Write the `func _asAnyWeakPtr() -> pxr.TfAnyWeakPtr` method on `Self.TfWeakPtrType`
            writeLines({
                "pxr::TfAnyWeakPtr _asAnyWeakPtr(const " + names.weakPtrInCpp + "& x) {",
                "    return pxr::TfAnyWeakPtr(x);",
                "}",
            });
            // Write the `static func _fromAnyWeakPtr(_ x: pxr.TfAnyWeakPtr) -> Self` static method on `Self.TfWeakPtrType`
            #warning "consider using dynamic_cast for better safety?"
            writeLines({
                names.weakPtrInCpp + "_fromAnyWeakPtr" + names.mangledName + "(const pxr::TfAnyWeakPtr& x) {",
                "    if (!x) { return " + names.weakPtrInCpp + "(nullptr); }",
                "    " + names.swiftNameInCpp + "* rawPtr = static_cast<" + names.swiftNameInCpp + "*>(const_cast<pxr::TfWeakBase*>(x.GetWeakBase()));",
                "    return " + names.weakPtrInCpp + "(rawPtr);",
                "}",
            });
            // Write the `static func _nullPtr() -> Self` static method on `Self.TfWeakPtrType`
            writeLines({
                names.weakPtrInCpp + "_nullWeakPtr" + names.mangledName + "() {",
                "    return " + names.weakPtrInCpp + "(nullptr);",
                "}",
            });
            // Write the `func _isNonnull() -> Bool` method on `Self.TfWeakPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.weakPtrInCpp + " & p) {",
                    "    return (bool)p;",
                    "}",
                });
            }
            // Write the `func _isNonnull() -> Bool` method on `Self.TfConstWeakPtrType`
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "bool _isNonnull(const " + names.constWeakPtrInCpp + " & p) {",
                    "    return (bool)p;"
                    "}",
                });
            }
        } // if isTfWeakBaseSubclass(tagDecl)
    }
}

void ReferenceTypeConformanceCodeGen::writeSwiftFile(const ReferenceTypeConformanceCodeGen::Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        auto printer = typeNamePrinter(tagDecl);
        const auto names = nameHelper(printer);
        
        // Write the Swift conformance to SwiftUsdReferenceTypeProtocol
        writeLines({
            "",
            "extension " + names.swiftNameInSwift + ": Overlay._SwiftUsdReferenceTypeProtocol {",
            "    public typealias _SelfType = " + names.swiftNameInSwift,
            "}",
        });

        if (isTfRefBaseSubclass(tagDecl)) {
            // Write the Swift conformance to TfRefBaseProtocol and TfRefPtrProtocol
            if (!isExactlyTfRefBase(tagDecl)) {
                writeLines({
                    "extension " + names.swiftNameInSwift + ": Overlay._TfRefBaseProtocol {}",
                    "extension " + names.refPtrTypedef + ": Overlay._TfRefPtrProtocol {",
                    "    public typealias _TfRefBaseType = " + names.swiftNameInSwift,
                    "}",
                    "extension " + names.constRefPtrTypedef + ": Overlay._TfConstRefPtrProtocol {",
                    "    public typealias _TfRefBaseType = " + names.swiftNameInSwift,
                    "}",
                });
            }
        } // if isTfRefBaseSubclass
        
        if (isTfWeakBaseSubclass(tagDecl)) {
            // Write the Swift conformance to TfWeakBaseProtocol and TfWeakPtrProtocol
            writeLines({
                "extension " + names.swiftNameInSwift + ": Overlay._TfWeakBaseProtocol {}",
                "extension " + names.weakPtrTypedef + ": Overlay._TfWeakPtrProtocol {",
                "    public typealias _TfWeakBaseType = " + names.swiftNameInSwift,
                "}",
                "extension " + names.constWeakPtrTypedef + ": Overlay._TfConstWeakPtrProtocol {",
                "    public typealias _TfWeakBaseType = " + names.swiftNameInSwift,
                "}",

            });
        } // if isTfWeakBaseSubclass
        
        if (isTfSingletonImmortalSpecialization(tagDecl)) {
            // Write the Swift conformance to ImmortalReferenceTypeProtocol
            writeLines({
                "extension " + names.swiftNameInSwift + ": Overlay._SwiftUsdImmortalReferenceTypeProtocol {}",
            });
        }
    }
}


