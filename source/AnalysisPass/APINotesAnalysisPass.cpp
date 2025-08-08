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

#include "AnalysisPass/APINotesAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"

APINotesAnalysisPass::APINotesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
    ASTAnalysisPass<APINotesAnalysisPass, APINotesAnalysisResult>(astAnalysisRunner) {}

std::string APINotesAnalysisPass::serializationFileName() const {
    return "APINotes.txt";
}

std::string APINotesAnalysisPass::testFileName() const {
    return "testAPINotes.txt";
}

std::vector<const clang::NamedDecl*> APINotesAnalysisPass::getHardCodedOwnedTypes() const {
    std::vector<std::string> names = {
        "class " PXR_NS"::UsdNotice::StageNotice",
        "class " PXR_NS"::UsdNotice::StageContentsChanged",
        "class " PXR_NS"::UsdNotice::ObjectsChanged",
        "class " PXR_NS"::UsdNotice::ObjectsChanged::PathRange",
        "class " PXR_NS"::UsdNotice::StageEditTargetChanged",
        "class " PXR_NS"::UsdNotice::LayerMutingChanged",
        "class " PXR_NS"::TfEnum",
        "class " PXR_NS"::TfToken",
        "class " PXR_NS"::TfType",
        "class " PXR_NS"::GfBBox3d",
        "class " PXR_NS"::GfQuatd",
        "class " PXR_NS"::GfQuaternion",
        "class " PXR_NS"::GfQuatf",
        "class " PXR_NS"::GfQuath",
        "class " PXR_NS"::GfRange2d",
        "class " PXR_NS"::GfRange3d",
        "class " PXR_NS"::GfRange2f",
        "class " PXR_NS"::GfRange3f",
        "class " PXR_NS"::GfRect2i",
        "class " PXR_NS"::GfRotation",
        "class " PXR_NS"::SdfValueTypeName",
        "template <class T> class " PXR_NS"::SdfHandle",
        "class " PXR_NS"::UsdAttribute",
        "class " PXR_NS"::UsdEditTarget",
        "class " PXR_NS"::UsdObject",
        "class " PXR_NS"::UsdPrim",
        "class " PXR_NS"::UsdPrimRange",
        "class " PXR_NS"::UsdProperty",
        "class " PXR_NS"::UsdRelationship",
        "class " PXR_NS"::UsdStagePopulationMask",
        "class " PXR_NS"::UsdGeomPrimvar",
        "class " PXR_NS"::UsdGeomXformOp",
        "template <class T> class " PXR_NS"::HgiHandle",
    };
    std::vector<const clang::NamedDecl*> result;
    for (const auto& s : names) {
        const clang::NamedDecl* namedDecl = findNamedDecl(s);
        if (!namedDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        result.push_back(namedDecl);
    }
    return result;
}
std::vector<const clang::FunctionDecl*> APINotesAnalysisPass::getHardCodedReplaceConstRefFunctionsWithCopy() const {
    std::vector<std::string> names = {
        "static const class " PXR_NS"::TfType & " PXR_NS"::TfType::FindByName(const std::string & name)",
        "static const class " PXR_NS"::SdfPath & " PXR_NS"::SdfPath::EmptyPath()",
        "static const class " PXR_NS"::SdfPath & " PXR_NS"::SdfPath::AbsoluteRootPath()",
        "static const class " PXR_NS"::SdfPath & " PXR_NS"::SdfPath::ReflexiveRelativePath()",
        "const std::string & " PXR_NS"::ArResolvedPath::GetPathString() const",
        "const class " PXR_NS"::TfToken & " PXR_NS"::UsdObject::GetName() const",
        "const std::string & " PXR_NS"::SdfPrimSpec::GetName() const",
        "const std::string & " PXR_NS"::SdfPropertySpec::GetName() const",
        "const class " PXR_NS"::TfToken & " PXR_NS"::SdfFileFormat::GetFormatId() const",
        "const class " PXR_NS"::TfWeakPtr<const class " PXR_NS"::SdfFileFormat> & " PXR_NS"::SdfLayer::GetFileFormat() const",
        "const std::string & " PXR_NS"::SdfLayer::GetRealPath() const",
        "const class " PXR_NS"::UsdAttribute & " PXR_NS"::UsdShadeInput::GetAttr() const",
        "const class " PXR_NS"::UsdAttribute & " PXR_NS"::UsdShadeOutput::GetAttr() const",
        "const class " PXR_NS"::SdfPath & " PXR_NS"::UsdShadeMaterialBindingAPI::DirectBinding::GetMaterialPath() const",
        "const class " PXR_NS"::SdfPath & " PXR_NS"::UsdShadeMaterialBindingAPI::CollectionBinding::GetMaterialPath() const",
        "const class " PXR_NS"::SdfPath & " PXR_NS"::UsdShadeMaterialBindingAPI::CollectionBinding::GetCollectionPath() const",
        
        "const class " PXR_NS"::GfRange3d & " PXR_NS"::GfBBox3d::GetBox() const",
        "const class " PXR_NS"::GfMatrix4d & " PXR_NS"::GfBBox3d::GetMatrix() const",
        "const class " PXR_NS"::GfQuatd & " PXR_NS"::GfDualQuatd::GetReal() const",
        "const class " PXR_NS"::GfQuatd & " PXR_NS"::GfDualQuatd::GetDual() const",
        "const class " PXR_NS"::GfQuatf & " PXR_NS"::GfDualQuatf::GetReal() const",
        "const class " PXR_NS"::GfQuatf & " PXR_NS"::GfDualQuatf::GetDual() const",
        "const class " PXR_NS"::GfQuath & " PXR_NS"::GfDualQuath::GetReal() const",
        "const class " PXR_NS"::GfQuath & " PXR_NS"::GfDualQuath::GetDual() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfFrustum::GetPosition() const",
        "const class " PXR_NS"::GfRotation & " PXR_NS"::GfFrustum::GetRotation() const",
        "const class " PXR_NS"::GfRange2d & " PXR_NS"::GfFrustum::GetWindow() const",
        "const class " PXR_NS"::GfRange1d & " PXR_NS"::GfFrustum::GetNearFar() const",
        "const class " PXR_NS"::GfRotation & " PXR_NS"::GfTransform::GetPivotOrientation() const",
        "const class " PXR_NS"::GfRotation & " PXR_NS"::GfTransform::GetRotation() const",
        "const class " PXR_NS"::GfRotation & " PXR_NS"::GfTransform::GetScaleOrientation() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfTransform::GetCenter() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfTransform::GetPivotPosition() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfTransform::GetScale() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfTransform::GetTranslation() const",
        "const class std::vector<class " PXR_NS"::GfVec4f> & " PXR_NS"::GfCamera::GetClippingPlanes() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfLine::GetDirection() const",
        "const class " PXR_NS"::GfVec2d & " PXR_NS"::GfLine2d::GetDirection() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfPlane::GetNormal() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfRay::GetStartPoint() const",
        "const class " PXR_NS"::GfVec3d & " PXR_NS"::GfRay::GetDirection() const",

    };
    std::vector<const clang::FunctionDecl*> result;
    for (const auto& s : names) {
        const clang::FunctionDecl* functionDecl = findFunctionDecl(s);
        if (!functionDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        result.push_back(functionDecl);
    }
    return result;
}
std::vector<const clang::FunctionDecl*> APINotesAnalysisPass::getHardCodedReplaceMutatingFunctionsWithNonmutating() const {
    std::vector<std::string> names = {
        "_Bool " PXR_NS"::UsdPayloads::AddPayload(const class " PXR_NS"::SdfPayload & payload, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdPayloads::AddPayload(const std::string & identifier, const class " PXR_NS"::SdfPath & primPath, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdPayloads::AddPayload(const std::string & identifier, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdPayloads::AddInternalPayload(const class " PXR_NS"::SdfPath & primPath, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdPayloads::RemovePayload(const class " PXR_NS"::SdfPayload & ref)",
        "_Bool " PXR_NS"::UsdPayloads::ClearPayloads()",
        "_Bool " PXR_NS"::UsdPayloads::SetPayloads(const class std::vector<class " PXR_NS"::SdfPayload> & items)",
        "_Bool " PXR_NS"::UsdReferences::AddReference(const class " PXR_NS"::SdfReference & ref, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdReferences::AddReference(const std::string & identifier, const class " PXR_NS"::SdfPath & primPath, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdReferences::AddReference(const std::string & identifier, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdReferences::AddInternalReference(const class " PXR_NS"::SdfPath & primPath, const class " PXR_NS"::SdfLayerOffset & layerOffset, enum " PXR_NS"::UsdListPosition position)",
        "_Bool " PXR_NS"::UsdReferences::RemoveReference(const class " PXR_NS"::SdfReference & ref)",
        "_Bool " PXR_NS"::UsdReferences::ClearReferences()",
        "_Bool " PXR_NS"::UsdReferences::SetReferences(const class std::vector<class " PXR_NS"::SdfReference> & items)",
    };
    std::vector<const clang::FunctionDecl*> result;
    for (const auto& s : names) {
        const clang::FunctionDecl* functionDecl = findFunctionDecl(s);
        if (!functionDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        result.push_back(functionDecl);
    }
    return result;
}

bool APINotesAnalysisPass::VisitNamedDecl(clang::NamedDecl *namedDecl) {
    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    for (const auto& it : importAnalysisPass->getData()) {
        if (it.second.isImportedAsSharedReference()) {
            insert_or_assign(it.first, APINotesAnalysisResult::Kind::importTagAsShared);
        } else if (it.second.isImportedAsImmortalReference()) {
            insert_or_assign(it.first, APINotesAnalysisResult::Kind::importTagAsImmortal);
        }
    }
    
    for (const clang::NamedDecl* namedDecl : getHardCodedOwnedTypes()) {
        insert_or_assign(namedDecl, APINotesAnalysisResult::Kind::importTagAsOwned);
    }
    for (const clang::FunctionDecl* functionDecl : getHardCodedReplaceConstRefFunctionsWithCopy()) {
        insert_or_assign(functionDecl, APINotesAnalysisResult::Kind::replaceConstRefFunctionWithCopyingWrapper);
    }
    for (const clang::FunctionDecl* functionDecl : getHardCodedReplaceMutatingFunctionsWithNonmutating()) {
        insert_or_assign(functionDecl, APINotesAnalysisResult::Kind::replaceMutatingFunctionWithNonmutatingWrapper);
    }
    
    {
        // Making TfNotice::Register(_:_:) unavailable doesn't stop Swift from
        // trying to resolve to it, but using SwiftName on it in API Notes does,
        // even though SwiftName on templates doesn't work properly and renaming overloads
        // doesn't work properly. But, this lets us get rid of some invasive changes
        // to pxr/base/tf/notice.h
        std::string s = "template <class LPtr, class MethodPtr> static class " PXR_NS"::TfNotice::Key " PXR_NS"::TfNotice::Register(const type-parameter-0-0 & listener, type-parameter-0-1 method)";
        const clang::FunctionDecl* functionDecl = findFunctionDecl(s);
        if (!functionDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        insert_or_assign(functionDecl, APINotesAnalysisResult::Kind::renameTfNoticeRegisterFunctionSpecialCase);
    }
    
    {
        // To avoid rdar://151640018 (crash when Tf_Remnant is not marked as FRT), we mark Tf_Remnant as an immortal,
        // unavailable FRT, instead of as a shared FRT. (If it weren't blocked by name printing, it would be
        // imported as a shared FRT, but that causes the compiler to seemingly spend forever compiling Swift code
        // that uses it.)
        std::string s = "class " PXR_NS"::Tf_Remnant";
        const clang::TagDecl* tagDecl = findTagDecl(s);
        if (!tagDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        insert_or_assign(tagDecl, APINotesAnalysisResult::Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase);
    }
    
    {
        // pxr::SdfZipFile::Iterator can't conform to Sequence because it's ~Copyable.
        // (Also, we want to expose something with the full functionality of the iterator,
        // not just its operator->().) So, we create Overlay.SdfZipFileIteratorWrapper,
        // and add `public typealias Iterator = Overlay.SdfZipFileIteratorWrapper` in
        // an extension on pxr.SdfZipFile for the Sequence conformance. But, that means
        // that `pxr.SdfZipFile.Iterator` is now an ambiguous type lookup between the
        // typealias and the C++ class. So, rename pxr::SdfZipFile::Iterator in Swift.
        std::string s = "class " PXR_NS"::SdfZipFile::Iterator";
        const clang::TagDecl* tagDecl = findTagDecl(s);
        if (!tagDecl) {
            std::cerr << "Could not find " << s << std::endl;
            __builtin_trap();
        }
        insert_or_assign(tagDecl, APINotesAnalysisResult::Kind::renameSdfZipFileIteratorSpecialCase);
    }
    
    // Return false to stop the AST traversal immediately, because we don't need to do it
    return false;
}
