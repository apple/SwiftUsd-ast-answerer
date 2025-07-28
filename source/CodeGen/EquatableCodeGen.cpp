//
//  EquatableCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/2/24.
//

#include "CodeGen/EquatableCodeGen.h"

// MARK: Virtual

EquatableCodeGen::EquatableCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<EquatableCodeGen>(codeGenRunner) {}

std::string EquatableCodeGen::fileNamePrefix() const {
    return "Equatable";
}

EquatableCodeGen::Data EquatableCodeGen::preprocess() {
    std::vector<const clang::TagDecl*> result;
    const auto& equatableData = getEquatableAnalysisPass()->getData();
    for (const auto& it : equatableData) {
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (it.second.isAvailable() && hasTypeName<CppNameInCpp>(itFirst)) {
            result.push_back(itFirst);
        }
    }
    return result;
}

EquatableCodeGen::Data EquatableCodeGen::extraSpecialCaseFiltering(const Data& data) const {
    Data result;
    
    for (const clang::TagDecl* tagDecl : data) {
        // Swift warns about finding a cycle while resolving the swift_name for operator==
        // for these types. They cause emit-module to crash while compiling the framework
        auto printer = typeNamePrinter(tagDecl);
        auto swiftName = getTypeNameOpt<SwiftNameInSwift>(printer);
        
        // HgiShaderProgramDesc declares `inline bool operator==` in the header, but defines it
        // in the implementation, and Swift raises a linker error due to not finding the operator==
        // inside Swift's func ==. 
        //
        // Note that this isn't a Swift bug, but a Usd bug (present in v25.05.01);
        // in a pure C++ world, the `operator==` still can't be called in a C++ code base
        // because the symbol for it isn't emitted.
        if (swiftName == "pxr.HgiShaderProgramDesc") {
            continue;
        }
        
        // Swift 6.1 crashes when doing Equatable conformance for these types
        // https://github.com/swiftlang/swift/pull/78974 (Swiftc crash when C++ operator== doesn't name the rhs parameter in header)
        if (swiftName == "pxr.TraceThreadId" ||
            swiftName == "pxr.UsdPhysicsRigidBodyMaterialDesc" ||
            swiftName == "pxr.UsdPhysicsSceneDesc" ||
            swiftName == "pxr.UsdPhysicsCollisionGroupDesc" ||
            swiftName == "pxr.UsdPhysicsSphereShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCapsuleShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCapsule1ShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCylinderShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCylinder1ShapeDesc" ||
            swiftName == "pxr.UsdPhysicsConeShapeDesc" ||
            swiftName == "pxr.UsdPhysicsPlaneShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCustomShapeDesc" ||
            swiftName == "pxr.UsdPhysicsCubeShapeDesc" ||
            swiftName == "pxr.UsdPhysicsMeshShapeDesc" ||
            swiftName == "pxr.UsdPhysicsSpherePoint" ||
            swiftName == "pxr.UsdPhysicsSpherePointsShapeDesc" ||
            swiftName == "pxr.UsdPhysicsRigidBodyDesc" ||
            swiftName == "pxr.UsdPhysicsJointLimit" ||
            swiftName == "pxr.UsdPhysicsJointDrive" ||
            swiftName == "pxr.UsdPhysicsArticulationDesc" ||
            swiftName == "pxr.UsdPhysicsJointDesc" ||
            swiftName == "pxr.UsdPhysicsCustomJointDesc" ||
            swiftName == "pxr.UsdPhysicsFixedJointDesc" ||
            swiftName == "pxr.UsdPhysicsD6JointDesc" ||
            swiftName == "pxr.UsdPhysicsPrismaticJointDesc" ||
            swiftName == "pxr.UsdPhysicsSphericalJointDesc" ||
            swiftName == "pxr.UsdPhysicsRevoluteJointDesc" ||
            swiftName == "pxr.UsdPhysicsDistanceJointDesc") {
            continue;
        }
        
        result.push_back(tagDecl);
    }
    return result;
}

void EquatableCodeGen::writeHeaderFile(const EquatableCodeGen::Data &data) {
    writeLine("namespace __Overlay {");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        BinaryOpProtocolAnalysisResult analysisResult = getEquatableAnalysisPass()->find(tagDecl)->second;
        if (analysisResult._kind == BinaryOpProtocolAnalysisResult::availableFoundBySwift) {
            continue;
        }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "  bool operatorEqualsEquals(const " + cppTypeName + "& l,",
            "                            const " + cppTypeName + "& r);",
        });
    }
    writeLine("}");
}

void EquatableCodeGen::writeCppFile(const EquatableCodeGen::Data &data) {
    for (const clang::TagDecl* tagDecl : data) {
        
        if (!hasTypeName<SwiftNameInCpp>(tagDecl)) { continue; }
        
        BinaryOpProtocolAnalysisResult analysisResult = getEquatableAnalysisPass()->find(tagDecl)->second;
        if (analysisResult._kind == BinaryOpProtocolAnalysisResult::availableFoundBySwift) {
            continue;
        }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string cppTypeName = getTypeName<SwiftNameInCpp>(printer);
        writeLines({
            "bool __Overlay::operatorEqualsEquals(const " + cppTypeName + "& l,",
            "                                     const " + cppTypeName + "& r) {"
        });
        
        switch (analysisResult._kind) {
            case BinaryOpProtocolAnalysisResult::unknown: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailable: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailableBlockedByEquatable: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableFoundBySwift:
                std::cerr << "Internal state error in equatable code gen" << std::endl;
                __builtin_trap();
                
            case BinaryOpProtocolAnalysisResult::availableImportedAsReference:
                writeLine("    return &l == &r;");
                break;
                
            case BinaryOpProtocolAnalysisResult::availableShouldBeFoundBySwiftButIsnt: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableFriendFunction: // fallthrough
            case BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration: // fallthhrough
            case BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes:
                writeLine("    return l == r;");
                break;
                
        }
        writeLine("}");
    }
}

void EquatableCodeGen::writeSwiftFile(const EquatableCodeGen::Data &data) {
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
        BinaryOpProtocolAnalysisResult analysisResult = getEquatableAnalysisPass()->find(tagDecl)->second;
        
        auto printer = typeNamePrinter(tagDecl);
        std::string swiftTypeName = getTypeName<SwiftNameInSwift>(printer);
        switch (analysisResult._kind) {
            case BinaryOpProtocolAnalysisResult::unknown: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport: // fallthrough
            case BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailable: // fallthrough
            case BinaryOpProtocolAnalysisResult::unavailableBlockedByEquatable: // fallthrough
                std::cerr << "Internal state error in equatable code gen" << std::endl;
                __builtin_trap();
                
            case BinaryOpProtocolAnalysisResult::availableFoundBySwift:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable {} // foundBySwift",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableShouldBeFoundBySwiftButIsnt:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable { // availableShouldBeFoundBySwiftButIsnt",
                    "    public static func ==(lhs: Self, rhs: Self) -> Bool {",
                    "        __Overlay.operatorEqualsEquals(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableImportedAsReference:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable { // importedAsReference",
                    "    public static func ==(lhs: " + swiftTypeName + ", rhs: " + swiftTypeName + ") -> Bool {",
                    "        __Overlay.operatorEqualsEquals(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable { // classTemplateSpecialization",
                    "    public static func ==(lhs: Self, rhs: Self) -> Bool {",
                    "        __Overlay.operatorEqualsEquals(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
                
            case BinaryOpProtocolAnalysisResult::availableFriendFunction:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable {} // friendFunction",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable { // inlineMethodDefinedAfterDeclaration",
                    "    public static func ==(lhs: Self, rhs: Self) -> Bool {",
                    "        __Overlay.operatorEqualsEquals(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
                
            case BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes:
                writeLines({
                    "extension " + swiftTypeName + ": Equatable { // differentArgumentTypes",
                    "    public static func ==(lhs: Self, rhs: Self) -> Bool {",
                    "        __Overlay.operatorEqualsEquals(lhs, rhs)",
                    "    }",
                    "}",
                });
                break;
        } // end switch (analysisResult._kind) {
    } // end for (const clang::TagDecl* tagDecl : data) {
}

std::vector<std::pair<std::string, EquatableCodeGen::Data>> EquatableCodeGen::writeDocCFile(std::string* outTitle,
                                    std::string* outOverview,
                                    const EquatableCodeGen::Data& processedData) {
    *outTitle = "Equatable protocol conformances";
    *outOverview = "These types conform to `Equatable` in Swift.";
    return {{"", processedData}};
}
