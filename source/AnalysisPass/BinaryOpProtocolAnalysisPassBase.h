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

#ifndef BinaryOpProtocolAnalysisPassBase_h
#define BinaryOpProtocolAnalysisPassBase_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisResult/BinaryOpProtocolAnalysisResult.h"

// Equatable and Comparable are a) complicated, b) nearly identical.
//
// A Type T is Equatable iff it is convertible to types U and V (which may be T or each other, but not Bool),
// such that `bool operator==(const U&, const V&)` is exposed to Swift.
// This function can be declared arbitrarily late, so we need more than a single pass.
// And, Equatability wants to be inherited, which makes things more complicated.
//
// Comparable behaves similarly, but uses `bool operator<(const U&, const V&)`

struct BinaryOpFunctionWitnessSet {
    BinaryOpFunctionWitnessSet();
    
    struct Properties {
        bool isFriendFunction;
        bool isInlineMethodDefinedAfterDeclaration;
    };
    
    void insert(clang::QualType a, clang::QualType b, Properties properties);
    bool find(clang::QualType a, clang::QualType b, Properties* properties);
        
private:
    std::map<std::pair<clang::QualType, clang::QualType>, Properties> _data;
};

template <typename Derived>
class BinaryOpProtocolAnalysisPassBase: public ASTAnalysisPass<BinaryOpProtocolAnalysisPassBase<Derived>, BinaryOpProtocolAnalysisResult> {
public:
    BinaryOpProtocolAnalysisPassBase(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<BinaryOpProtocolAnalysisPassBase<Derived>, BinaryOpProtocolAnalysisResult>(astAnalysisRunner)
    {}
    
    bool isComparablePass() const;
    
    // Unfortunately, we need to visit decls not from Usd,
    // because types like GfHalf might define implicit conversions
    // to e.g. stdlib type like float with an == or <
    bool shouldOnlyVisitDeclsFromUsd() const override {
        return false;
    }
    
    bool checkIfBlockedByEquatable(const clang::CXXRecordDecl* cxxRecordDecl);
        
    void finalize(const clang::NamedDecl* namedDecl) override {
        const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl);
        // We only need to do checking on this tagDecl and not worry about templates,
        // because specializations of a class template are all visited in VisitCXXRecord,
        // so we'll know we need to finalize them
        
        auto it = this->find(tagDecl);
        if (it->second._kind != BinaryOpProtocolAnalysisResult::unknown) {
            return;
        }
                
        if (ASTHelpers::getAsString(tagDecl) == "class " PXR_NS"::TfHashMap<const void *, struct " PXR_NS"::TfRefPtrTracker::Trace, class " PXR_NS"::TfHash>") {
            if (!isComparablePass()) {
                // TfHashMap's operator== uses a static_cast that assumes the key and value have operator==,
                // or else you get a substitution failure deep in the templated implementation. This analysis isn't
                // able to detect that, so special case it as unavailable
                it->second._kind = BinaryOpProtocolAnalysisResult::unavailable;
                return;
            }
        }
        
        if (ASTHelpers::getAsString(tagDecl) == "class " PXR_NS"::VdfIndexedWeightsOperand") {
            // VdfIndexedWeightsOperand derives from VdfIndexedData<float>, which has a `bool operator==(const& VdfIndexedData other)`.
            // But, VdfIndexedWeightsOperand adds a `This operator==(const& This other)`, and the compiler
            // selects that overload for `a == b`. Special case it as unavailable, because this is the only known case
            // of an inherited `operator==` being blocked due to an overload with an incompatible signature.
            it->second._kind = BinaryOpProtocolAnalysisResult::unavailable;
        }

        
        const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
        if (!cxxRecordDecl) {
            it->second._kind = BinaryOpProtocolAnalysisResult::unavailable;
            return;
        }

        const ImportAnalysisPass* importAnalysisPass = this->getASTAnalysisRunner().getImportAnalysisPass();
        const auto& importIt = importAnalysisPass->find(cxxRecordDecl);
        if (importIt == importAnalysisPass->end() || !importIt->second.isImportedSomehow()) {
            this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport);
            return;
        }
        if (importIt->second.isImportedAsNonCopyable()) {
            this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable);
            return;
        }
        
        // Unfortunately, we need to do quadratic checking of convertible types.
        // Given `bool operator==(const A&, const B&)`, with `C` implicitly convertible to `A` and `B`,
        // but neither `A` or `B` implicitly convertible to each other, we want to generate Equatable for C.
        //
        // Note: We want to order convertibleTypes so that we end up using the best possible conformance witness.
        // We build convertibleTypes from best to worst, then reverse it so we loop from reverse to best
        // and allow upgrading but not downgrading the witness
        
        std::vector<clang::QualType> convertibleTypes;
        for (const clang::CXXRecordDecl* subtype : ASTHelpers::allAccessibleSupertypes(cxxRecordDecl)) {
            convertibleTypes.push_back(subtype->getTypeForDecl()->getCanonicalTypeUnqualified());
        }
        if (const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl)) {
            clang::ClassTemplateDecl* classTemplateDecl = classTemplateSpecializationDecl->getSpecializedTemplate();
            convertibleTypes.push_back(classTemplateDecl->getInjectedClassNameSpecialization().getCanonicalType());
        }
        for (const clang::Type* convertibleType : ASTHelpers::allAccessibleImplicitNoArgConstConversions(cxxRecordDecl)) {
            convertibleTypes.push_back(convertibleType->getCanonicalTypeUnqualified());
        }
        std::reverse(convertibleTypes.begin(), convertibleTypes.end());
        
        for (uint64_t i = 0; i < convertibleTypes.size(); i++) {
            for (uint64_t j = i; j < convertibleTypes.size(); j++) {
                clang::QualType firstType = convertibleTypes[i].getCanonicalType();
                clang::QualType secondType = convertibleTypes[j].getCanonicalType();
                clang::QualType thisType = tagDecl->getTypeForDecl()->getCanonicalTypeUnqualified();
                
                if (firstType->isBooleanType() || secondType->isBooleanType()) {
                    // We don't want to use `operator bool()` conversions for Equatable conformance,
                    // because that's about validity in C++, not equality
                    continue;
                }
                
                // Important: Whenever we consider setting the witness, we want to make sure
                // we're not downgrading the witness to a worse version
                
                BinaryOpFunctionWitnessSet::Properties properties;
                if (_witnessSet.find(convertibleTypes[i], convertibleTypes[j], &properties)) {
                    if (clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(tagDecl)) {
                        it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization);
                    }
                    
                    if (firstType != thisType || secondType != thisType) {
                        // If the argument types aren't the same as the conforming type, Swift doesn't find an
                        // ==(Self, Self)
                        if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown) {
                            it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes,
                                                                 firstType.getAsString(),
                                                                 secondType.getAsString());
                        }
                        
                    } else if (properties.isFriendFunction) {
                        // If the argument types are the same but it's a friend function, Swift doesn't find an
                        // ==(Self, Self) unless its a friend function of a class template specialization
                        if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableClassTemplateSpecialization) {
                            it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableFriendFunction);
                        }
                        
                    } else if (properties.isInlineMethodDefinedAfterDeclaration) {
                        // If the argument types are the same but it's an inline method defined
                        // after the class definition, Swift doesn't find an ==(Self, Self)
                        if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableFriendFunction) {
                            it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration);
                        }
                    
                    } else {
                        if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableFriendFunction ||
                            it->second._kind == BinaryOpProtocolAnalysisResult::availableInlineMethodDefinedAfterDeclaration) {
                            
                            // If the argument types are the same and it's not a friend function
                            // and it's not an inline method defined after the class definition
                            // and it's not a class template specialization, Swift does find
                            // an ==(Self, Self). This includes free functions and methods
                            
                            it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableFoundBySwift);
                        }
                    }
                }
                
                
                if (firstType->isArithmeticType() && secondType->isArithmeticType()) {
                    // Arithmetic types use a builtin operator== that isn't in the AST.
                    // Since we can't discover it by traversing the AST, we have to have a special case for it
                    if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown) {
                        it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableDifferentArgumentTypes,
                                                             firstType.getAsString(),
                                                             secondType.getAsString());
                    }
                }
            }
        }
        
        if (it->second._kind == BinaryOpProtocolAnalysisResult::unknown) {
            it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::unavailable);
        } else {
            // Don't want to diagnose as blockedByEquatable if there isn't even a candidate
            checkIfBlockedByEquatable(cxxRecordDecl);
        }
        
        // Special case: some functions aren't visible to Swift for seemingly no reason.
        // Possibly related: rdar://138118008 (Spurious "warning: cycle detected while resolving" message (Usd interop))
        if (it->second._kind == BinaryOpProtocolAnalysisResult::availableFoundBySwift) {
            if (ASTHelpers::getAsString(namedDecl) == "class " PXR_NS"::HdPrimOriginSchema::OriginPath" ||
                ASTHelpers::getAsString(namedDecl) == "class " PXR_NS"::UsdNotice::ObjectsChanged::PathRange::iterator") {
                it->second = BinaryOpProtocolAnalysisResult(BinaryOpProtocolAnalysisResult::availableShouldBeFoundBySwiftButIsnt);
            }
        }
    }
    
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) override {
        // We need to traverse the entire AST, even things not in Usd,
        // because we might need functions that are not defined in Usd for == for some types.
        // But, we want to only generate Equatable conformances for types in Usd that are
        // imported into Swift, and ignore all others.
        if (!this->isEarliestDeclLocFromUsd(cxxRecordDecl)) {
            return true;
        }
        if (!this->areAllUsdDeclsFromPublicHeaders(cxxRecordDecl)) {
            return true;
        }
        
        const ImportAnalysisPass* importAnalysisPass = this->getASTAnalysisRunner().getImportAnalysisPass();
        const auto& it = importAnalysisPass->find(cxxRecordDecl);
        if (it == importAnalysisPass->end()) {
            return true;
        }
        if (!it->second.isImportedSomehow()) {
            this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::noAnalysisBecauseBlockedByImport);
            return true;
        }
        if (it->second.isImportedAsNonCopyable()) {
            this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::noAnalysisBecauseNonCopyable);
            return true;
        }
        
        if (this->find(cxxRecordDecl) == this->end()) {
            // Make sure we know we need to make a decision for this type
            this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::unknown);
        }
        
        if (it != importAnalysisPass->end()) {
            // Make reference types Equatable, so we can make them Hashable
            if (it->second.isImportedAsAnyReference()) {
                this->insert_or_assign(cxxRecordDecl, BinaryOpProtocolAnalysisResult::availableImportedAsReference);
            }
        }
        
        // Don't set to unavailable yet, because Equatable conformance
        // can be provided arbitrarily late, and it can be inherited.
        
        return true;
    }
    bool VisitFunctionDecl(clang::FunctionDecl* functionDecl) override {
        // Important: Don't disallow functions not from Usd,
        // because we might want to use `==` functions from stdlib
        // (e.g., GfHalf using `==(float, float)`
        if (this->isEarliestDeclLocFromUsd(functionDecl) && !this->areAllUsdDeclsFromPublicHeaders(functionDecl)) {
#warning suspect use of declLocFromUsd
            return true;
        }
        
        if (isComparablePass()) {
            // Must be <
            if (functionDecl->getOverloadedOperator() != clang::OO_Less) {
                return true;
            }
        } else {
            // Must be ==
            if (functionDecl->getOverloadedOperator() != clang::OO_EqualEqual) {
                return true;
            }
        }
        // Must return bool
        if (!functionDecl->getReturnType()->isBooleanType()) {
            return true;
        }
        
        // Ignore if Swift can't see it
        if (functionDecl->isDeleted() || ASTHelpers::isNotVisibleToSwift(functionDecl->getAccess())) {
            return true;
        }
        
        // Pull out the types of the arguments...
        clang::QualType lhsType;
        clang::QualType rhsType;
        
        if (functionDecl->getNumParams() == 1) {
            const clang::CXXMethodDecl* cxxMethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(functionDecl);
            if (!cxxMethodDecl) {
                return true;
            }
            
            // Go from (probably) `const This*` to `const This&`
            lhsType = cxxMethodDecl->getThisType()->getPointeeType();
            lhsType = functionDecl->getASTContext().getLValueReferenceType(lhsType);
            
            rhsType = cxxMethodDecl->parameters()[0]->getType();
            
        } else if (functionDecl->getNumParams() == 2) {
            lhsType = functionDecl->parameters()[0]->getType();
            rhsType = functionDecl->parameters()[1]->getType();
        } else {
            return true;
        }

        
        // Arguments must be const& types
        if (!lhsType->isBuiltinType()) {
            lhsType = ASTHelpers::removingRefConst(lhsType);
        }
        if (!rhsType->isBuiltinType()) {
            rhsType = ASTHelpers::removingRefConst(rhsType);
        }
        if (lhsType.isNull() || rhsType.isNull()) {
            return true;
        }
        
        // Arguments from Usd must be imported into swift and not-noncopyable
        const ImportAnalysisPass* importAnalysisPass = this->getASTAnalysisRunner().getImportAnalysisPass();
        if (const clang::TagDecl* lhsTagDecl = lhsType->getAsTagDecl()) {
            if (this->doesTypeContainUsdTypes(lhsTagDecl)) {
                const auto& it = importAnalysisPass->find(lhsTagDecl);
                if (it == importAnalysisPass->end() || !it->second.isImportedSomehow() || it->second.isImportedAsNonCopyable()) {
                    return true;
                }
            }
        }
        if (const clang::TagDecl* rhsTagDecl = rhsType->getAsTagDecl()) {
            if (this->doesTypeContainUsdTypes(rhsTagDecl)) {
                const auto& it = importAnalysisPass->find(rhsTagDecl);
                if (it == importAnalysisPass->end() || !it->second.isImportedSomehow() || it->second.isImportedAsNonCopyable()) {
                    return true;
                }
            }
        }
        
        // We also want to support templated types here for efficiency
        std::vector<clang::QualType> lhsTypes = {lhsType};
        std::vector<clang::QualType> rhsTypes = {rhsType};
        
        clang::QualType lhsInjected = ASTHelpers::getInjectedClassNameSpecialization(lhsType);
        if (!lhsInjected.isNull()) {
            lhsTypes.push_back(lhsInjected);
        }
        clang::QualType rhsInjected = ASTHelpers::getInjectedClassNameSpecialization(rhsType);
        if (!rhsInjected.isNull()) {
            rhsTypes.push_back(rhsInjected);
        }
        
        for (clang::QualType l : lhsTypes) {
            for (clang::QualType r : rhsTypes) {
                
                BinaryOpFunctionWitnessSet::Properties properties;
                properties.isFriendFunction = functionDecl->getFriendObjectKind() != clang::Decl::FriendObjectKind::FOK_None;
                if (clang::dyn_cast<clang::CXXMethodDecl>(functionDecl)) {
                    if (functionDecl->getCanonicalDecl() != functionDecl->getDefinition()) {
                        properties.isInlineMethodDefinedAfterDeclaration = functionDecl->getCanonicalDecl()->isInlineSpecified();
                    }
                }
                                        
                _witnessSet.insert(l, r, properties);
            }
        }
        
        return true;
    }

private:
    // Each element in the set corresponds to the function `bool operator OP(const U&, const V&)` or `U` method `bool operator OP(const V&) const`.
    // Additionally, the `pair` is sorted
    
    BinaryOpFunctionWitnessSet _witnessSet;
};


#endif /* BinaryOpProtocolAnalysisPassBase_h */
