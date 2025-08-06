// ===-------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer authors. All Rights Reserved. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at: 
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.     
// 
// SPDX-License-Identifier: Apache-2.0
// ===-------------------------------------------------------------------===//

#include "CodeGen/ComparableCodeGen.h"

ComparableCodeGen::ComparableCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<ComparableCodeGen>(codeGenRunner) {}

std::string ComparableCodeGen::fileNamePrefix() const {
    return "Comparable";
}

ComparableCodeGen::Data ComparableCodeGen::preprocess() {
    std::vector<const clang::TagDecl*> result;
    const auto& comparableData = getComparableAnalysisPass()->getData();
    for (const auto& it : comparableData) {
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (it.second.isAvailable() && hasTypeName<CppNameInCpp>(itFirst)) {
            result.push_back(itFirst);
        }
    }
    return result;
}

ComparableCodeGen::Data ComparableCodeGen::extraSpecialCaseFiltering(const Data& data) const {
    // Do all the same filtering as EquatableCodeGen
    Data result;
    for (const clang::TagDecl* tagDecl : data) {
        auto printer = typeNamePrinter(tagDecl);
        auto swiftName = getTypeNameOpt<SwiftNameInSwift>(printer);
        if (swiftName == "pxr.TraceThreadId" ||
            swiftName == "pxr.HgiShaderProgramDesc") {
            continue;
        }
        
        // Comparable-specific filtering
        
        // SdfMapEditProxy's operator< is templated and ends up calling down to
        // std::operator< <const std::string, PXR_NS::VtValue, const std::string, PXR_NS::VtValue>,
        // which fails because VtValue doesn't have an operator<
        if (swiftName == "pxr.SdfDictionaryProxy") {
            continue;
        }
        
        result.push_back(tagDecl);
    }
    return result;
}

void ComparableCodeGen::writeHeaderFile(const ComparableCodeGen::Data& data) {
    writeLine("namespace __Overlay {");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        BinaryOpProtocolAnalysisResult analysisResult = getComparableAnalysisPass()->find(tagDecl)->second;
        if (analysisResult._kind == BinaryOpProtocolAnalysisResult::availableFoundBySwift) {
            continue;
        }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        
        writeLines({
            "  bool operatorLess(const " + cppTypeName + "& l,",
            "                    const " + cppTypeName + "& r);",
        });
    }
    writeLine("}");
}

void ComparableCodeGen::writeCppFile(const ComparableCodeGen::Data& data) {
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        BinaryOpProtocolAnalysisResult analysisResult = getComparableAnalysisPass()->find(tagDecl)->second;
        if (analysisResult._kind == BinaryOpProtocolAnalysisResult::availableFoundBySwift) {
            continue;
        }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "bool __Overlay::operatorLess(const " + cppTypeName + "& l,",
            "                             const " + cppTypeName + "& r) {",
        });
        
        switch (analysisResult._kind) {
            case BinaryOpProtocolAnalysisResult::unknown: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailable: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailableBlockedByEquatable: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableFoundBySwift:
                std::cerr << "Internal state error in comparable code gen" << std::endl;
                __builtin_trap();
                
            case BinaryOpProtocolAnalysisResult::availableImportedAsReference:
                writeLine("    return &l < &r;");
                break;
            
            case BinaryOpProtocolAnalysisResult::availableShouldBeFoundBySwiftButIsnt: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableFriendFunction: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration: // fallthhrough
            case BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes:
                writeLine("    return l < r;");
                break;
        }
        writeLine("}");
    }
}

void ComparableCodeGen::writeSwiftFile(const ComparableCodeGen::Data& data) {
    writeLine("// Conformance blocked by no Swift type name:");
    writeLine("");
    clang::PrintingPolicy printingPolicy((clang::LangOptions()));
    printingPolicy.adjustForCPlusPlus();
    printingPolicy.SuppressTagKeyword = 1;
    
    for (const clang::TagDecl* tagDecl : data) {
        writeThatTagDeclHasNoSwiftNameInSwiftIfNeeded(tagDecl);
    }
    
    writeLine("");
    writeLine("");
    writeLine("// Conformance available:");
    writeLine("");
    
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInSwift>(tagDecl)) { continue; }
        BinaryOpProtocolAnalysisResult analysisResult = getComparableAnalysisPass()->find(tagDecl)->second;
        
        auto printer = typeNamePrinter(tagDecl);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        switch (analysisResult._kind) {
            case BinaryOpProtocolAnalysisResult::unknown: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailableBlockedByEquatable: // fallthrough
                std::cerr << "Internal state error in comparable code gen" << std::endl;
                __builtin_trap();
                
            case BinaryOpProtocolAnalysisResult::availableFoundBySwift:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable {} // foundBySwift",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableShouldBeFoundBySwiftButIsnt:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable { // availableShouldBeFoundBySwiftButIsnt",
                    "    public static func <(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorLess(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableImportedAsReference:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable { // importedAsReference",
                    "    public static func <(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorLess(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable { // classTemplateSpecialization",
                    "    public static func <(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorLess(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;

            case BinaryOpProtocolAnalysisResult::availableFriendFunction:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable {} // friendFunction",
                });
                break;

            case BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable { // inlineMethodDefinedAfterDeclaration",
                    "    public static func <(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorLess(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes:
                writeLines({
                    "extension " + swiftTypeName + ": Comparable { // differentArgumentTypes",
                    "    public static func <(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorLess(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
        } // end switch
    } // end for
}

std::vector<std::pair<std::string, ComparableCodeGen::Data>> ComparableCodeGen::writeDocCFile(std::string* outTitle,
                                    std::string* outOverview,
                                    const ComparableCodeGen::Data& processedData) {
    *outTitle = "Comparable protocol conformances";
    *outOverview = "These types conform to `Comparable` in Swift.";
    return {{"", processedData}};
}
