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

#include "CodeGen/CustomStringConvertibleCodeGen.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"
#include "CodeGen/EnumsCodeGen.h"

CustomStringConvertibleCodeGen::CustomStringConvertibleCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<CustomStringConvertibleCodeGen>(codeGenRunner) {}

std::string CustomStringConvertibleCodeGen::fileNamePrefix() const {
    return "CustomStringConvertible";
}

CustomStringConvertibleCodeGen::Data CustomStringConvertibleCodeGen::preprocess() {
    std::vector<const clang::TagDecl*> result;
    const auto& customStringConvertibleData = getCustomStringConvertibleAnalysisPass()->getData();
    for (const auto& it : customStringConvertibleData) {
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        auto printer = typeNamePrinter(itFirst);
        if (it.second.isAvailable() &&
            hasTypeName<SwiftNameInCpp>(itFirst) &&
            getTypeName<SwiftNameInCpp>(printer).starts_with("pxr::")) {
            result.push_back(itFirst);
        }
    }
    return result;
}

CustomStringConvertibleCodeGen::Data CustomStringConvertibleCodeGen::extraSpecialCaseFiltering(const CustomStringConvertibleCodeGen::Data& data) const {
    Data result;
    for (const auto& it : data) {
        auto printer = typeNamePrinter(it);
        auto swiftName = getTypeNameOpt<SwiftNameInSwift>(printer);
        // https://github.com/swiftlang/swift/issues/83115 (Conforming C++ enum to Swift protocol causes linker errors (missing destructors for STL types)
        if (swiftName == "pxr.HdSelection.HighlightMode") {
            continue;
        }
        result.push_back(it);
    }
    return result;
}

void CustomStringConvertibleCodeGen::writeHeaderFile(const Data &data) {
    writeLine("namespace __Overlay {");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        switch (getCustomStringConvertibleAnalysisPass()->find(tagDecl)->second._kind) {
            case CustomStringConvertibleAnalysisResult::unknown: // fallthrough
            case CustomStringConvertibleAnalysisResult::blockedByNoCandidate: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseNonCopyable:
                std::cerr << "Internal state error" << std::endl;
                __builtin_trap();
                
            case CustomStringConvertibleAnalysisResult::available: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableEnum: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdObjectSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableSdfSpecSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableSdfSpecHandleSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdMetadataValueMap: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdGeomXformOp:
                writeLine("  std::string to_string(const " + cppTypeName + "& x);");
                break;
        }
    }
    writeLine("}");
}

void CustomStringConvertibleCodeGen::writeMmFile(const Data &data) {
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        switch (getCustomStringConvertibleAnalysisPass()->find(tagDecl)->second._kind) {
            case CustomStringConvertibleAnalysisResult::unknown: // fallthrough
            case CustomStringConvertibleAnalysisResult::blockedByNoCandidate: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseNonCopyable:
                std::cerr << "Internal state error" << std::endl;
                __builtin_trap();
                
            case CustomStringConvertibleAnalysisResult::available:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    std::stringstream ss;",
                    "    ss << x;",
                    "    return ss.str();",
                    "}",
                });
                break;
                
            case CustomStringConvertibleAnalysisResult::availableEnum:
            {
                FindEnumsAnalysisResult analysisResult = getFindEnumsAnalysisPass()->find(tagDecl)->second;
                const clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(tagDecl);
                
                writeLine("std::string __Overlay::to_string(const " + cppTypeName + "& x) {");
                writeLine("    switch (x) {");
                std::set<int64_t> addedCases;
                for (int i = 0; i < analysisResult.caseNames.size(); i++) {
                    std::string x = analysisResult.caseNames[i];
                    int64_t value = analysisResult.caseValues[i];
                    if (addedCases.find(value) != addedCases.end()) {
                        continue;
                    }
                    addedCases.insert(value);
                    std::string pxrMember = getCodeGenRunner()->getEnumsCodeGen()->pxrMember(enumDecl, x, true, printer);
                    writeLine("    case " + pxrMember + ": return \"" + pxrMember + "\";");
                }
                writeLine("    default: return \"" + cppTypeName + "(rawValue: \" + std::to_string(static_cast<int64_t>(x)) + \")\";");
                writeLine("    }");
                writeLine("}");
                break;
            }
                
            case CustomStringConvertibleAnalysisResult::availableUsdObjectSubclass:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    return x.GetDescription();",
                    "}",
                });
                break;
                
            case CustomStringConvertibleAnalysisResult::availableSdfSpecSubclass:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    if (x.IsDormant()) {",
                    "        return \"dormant " + cppTypeName + "\";",
                    "    }",
                    "    return \"" + cppTypeName + "(\" + x.GetLayer()->GetIdentifier() + \", \" + x.GetPath().GetString() + \")\";",
                    "}",
                });
                break;
                
            case CustomStringConvertibleAnalysisResult::availableSdfSpecHandleSubclass:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    if (!x) {",
                    "        return \"expired " + cppTypeName + "\";",
                    "    }",
                    "    return \"" + cppTypeName + "(\" + __Overlay::to_string(x.GetSpec()) + \")\";",
                    "}",
                    
                });
                break;
                
            case CustomStringConvertibleAnalysisResult::availableUsdMetadataValueMap:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    std::stringstream ss;",
                    "    ss << \"< \";",
                    "    for (const auto& p : x) {",
                    "        ss << \"<\" << p.first << \": \" << p.second << \"> \";"
                    "    }",
                    "    ss << \">\";",
                    "    return ss.str();",
                    "}",
                });
                break;
                
            case CustomStringConvertibleAnalysisResult::availableUsdGeomXformOp:
                writeLines({
                    "std::string __Overlay::to_string(const " + cppTypeName + "& x) {",
                    "    return \"" + cppTypeName + "(\" + x.GetAttr().GetPath().GetString() + \")\";",
                    "}",
                });
                break;
        } // end switch
    } // end for
}

void CustomStringConvertibleCodeGen::writeSwiftFile(const Data &data) {
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
        
        auto printer = typeNamePrinter(tagDecl);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        switch (getCustomStringConvertibleAnalysisPass()->find(tagDecl)->second._kind) {
            case CustomStringConvertibleAnalysisResult::unknown: // fallthrough
            case CustomStringConvertibleAnalysisResult::blockedByNoCandidate: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case CustomStringConvertibleAnalysisResult::noAnalysisBecauseNonCopyable:
                std::cerr << "Internal state error" << std::endl;
                __builtin_trap();
                
            case CustomStringConvertibleAnalysisResult::available: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableEnum: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdObjectSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableSdfSpecSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableSdfSpecHandleSubclass: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdMetadataValueMap: // fallthrough
            case CustomStringConvertibleAnalysisResult::availableUsdGeomXformOp:
                writeLines({
                    "extension " + swiftTypeName + ": CustomStringConvertible {",
                    "    public var description: String {",
                    "        Swift.String(__Overlay.to_string(self))",
                    "    }",
                    "}",
                });
                break;
        }
    }
}

std::vector<std::pair<std::string, CustomStringConvertibleCodeGen::Data>> CustomStringConvertibleCodeGen::writeDocCFile(std::string* outTitle,
                                    std::string* outOverview,
                                    const CustomStringConvertibleCodeGen::Data& processedData) {
    *outTitle = "CustomStringConvertible protocol conformances";
    *outOverview = "These types conform to `CustomStringConvertible` in Swift.";
    return {{"", processedData}};
}
