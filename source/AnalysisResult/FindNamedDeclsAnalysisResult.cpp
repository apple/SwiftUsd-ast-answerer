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

#include "clang/AST/Decl.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

FindNamedDeclsAnalysisResult::operator std::string() const {
    std::stringstream s;
    for (const auto& it : _namedDeclMap) {
        s << it.first << ";";
        #define PRINT_IF(T) if (clang::dyn_cast<clang::T>(it.second)) { s << " " << #T << ";"; }
        PRINT_IF(NamedDecl)
            PRINT_IF(BaseUsingDecl)
                PRINT_IF(UsingDecl)
                PRINT_IF(UsingEnumDecl)
            PRINT_IF(HLSLBufferDecl)
            // PRINT_IF(HLSLRootSignatureDecl)
            PRINT_IF(LabelDecl)
            // PRINT_IF(NamespaceBaseDecl)
                PRINT_IF(NamespaceAliasDecl)
                PRINT_IF(NamespaceDecl)
            PRINT_IF(ObjCCompatibleAliasDecl)
            PRINT_IF(ObjCContainerDecl)
                PRINT_IF(ObjCCategoryDecl)
                PRINT_IF(ObjCImplDecl)
                    // PRINT_IF(ObjcCategoryImplDecl)
                    PRINT_IF(ObjCImplementationDecl)
                PRINT_IF(ObjCInterfaceDecl)
                // PRINT_IF(ObjcProtocolDecl)
            PRINT_IF(ObjCMethodDecl)
            PRINT_IF(ObjCPropertyDecl)
            PRINT_IF(TemplateDecl)
                PRINT_IF(BuiltinTemplateDecl)
                PRINT_IF(ConceptDecl)
                PRINT_IF(RedeclarableTemplateDecl)
                    PRINT_IF(ClassTemplateDecl)
                    PRINT_IF(FunctionTemplateDecl)
                    PRINT_IF(TypeAliasTemplateDecl)
                    PRINT_IF(VarTemplateDecl)
                PRINT_IF(TemplateTemplateParmDecl)
            PRINT_IF(TypeDecl)
                PRINT_IF(TagDecl)
                    PRINT_IF(EnumDecl)
                    PRINT_IF(RecordDecl)
                        PRINT_IF(CXXRecordDecl)
                            PRINT_IF(ClassTemplateSpecializationDecl)
                                PRINT_IF(ClassTemplatePartialSpecializationDecl)
                PRINT_IF(TemplateTypeParmDecl)
                PRINT_IF(TypedefNameDecl)
                    // PRINT_IF(ObjcTypeParamDecl)
                    PRINT_IF(TypeAliasDecl)
                    PRINT_IF(TypedefDecl)
                PRINT_IF(UnresolvedUsingTypenameDecl)
            PRINT_IF(UnresolvedUsingIfExistsDecl)
            PRINT_IF(UsingDirectiveDecl)
            PRINT_IF(UsingPackDecl)
            PRINT_IF(UsingShadowDecl)
                PRINT_IF(ConstructorUsingShadowDecl)
            PRINT_IF(ValueDecl)
                // PRINT_IF(OMPDeclarativeDirective<typename T>)
                    PRINT_IF(OMPDeclareMapperDecl)
                PRINT_IF(BindingDecl)
                PRINT_IF(DeclaratorDecl)
                    PRINT_IF(FieldDecl)
                        PRINT_IF(ObjCAtDefsFieldDecl)
                        // PRINT_IF(ObjClvarDecl)
                    PRINT_IF(FunctionDecl)
                        PRINT_IF(CXXDeductionGuideDecl)
                        PRINT_IF(CXXMethodDecl)
                            PRINT_IF(CXXConstructorDecl)
                            PRINT_IF(CXXConversionDecl)
                            PRINT_IF(CXXDestructorDecl)
                    PRINT_IF(MSPropertyDecl)
                    PRINT_IF(NonTypeTemplateParmDecl)
                    PRINT_IF(VarDecl)
                        PRINT_IF(DecompositionDecl)
                        PRINT_IF(ImplicitParamDecl)
                        PRINT_IF(OMPCapturedExprDecl)
                        PRINT_IF(ParmVarDecl)
                        PRINT_IF(VarTemplateSpecializationDecl)
                            PRINT_IF(VarTemplatePartialSpecializationDecl)
                PRINT_IF(EnumConstantDecl)
                PRINT_IF(IndirectFieldDecl)
                PRINT_IF(MSGuidDecl)
                PRINT_IF(OMPDeclareReductionDecl)
                PRINT_IF(TemplateParamObjectDecl)
                PRINT_IF(UnnamedGlobalConstantDecl)
                PRINT_IF(UnresolvedUsingValueDecl)
        #undef PRINT_IF
        s << std::endl;
    }
    for (const auto& it : _typeMap) {
        s << it.first << ";" << std::endl;
    }
    return s.str();
}

FindNamedDeclsAnalysisResult::FindNamedDeclsAnalysisResult() = default;

/* static */
std::optional<FindNamedDeclsAnalysisResult> FindNamedDeclsAnalysisResult::deserialize(const std::string &data, const FindNamedDeclsAnalysisPass* astAnalysisPass) {
    return FindNamedDeclsAnalysisResult();
}

const FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisResult::getNamedDeclMap() const {
    return _namedDeclMap;
}
FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisResult::getNamedDeclMap() {
    return _namedDeclMap;
}

const FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisResult::getTypeMap() const {
    return _typeMap;
}
FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisResult::getTypeMap() {
    return _typeMap;
}

std::ostream& operator <<(std::ostream& os, const FindNamedDeclsAnalysisResult& obj) {
    return os << std::string(obj);
}


