//
//  CustomStringConvertibleAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/30/24.
//

#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/CustomStringConvertibleAnalysisPass.h"
#include "AnalysisPass/FindEnumsAnalysisPass.h"

CustomStringConvertibleAnalysisPass::CustomStringConvertibleAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
    ASTAnalysisPass<CustomStringConvertibleAnalysisPass, CustomStringConvertibleAnalysisResult>(astAnalysisRunner) {}

std::string CustomStringConvertibleAnalysisPass::serializationFileName() const {
    return "CustomStringConvertible.txt";
}

std::string CustomStringConvertibleAnalysisPass::testFileName() const {
    return "testCustomStringConvertible.txt";
}

// Unfortunately, we need to visit decls not from Usd,
// because types like GfHalf might define implicit conversions
// to e.g. stdlib types like float with an <<
bool CustomStringConvertibleAnalysisPass::shouldOnlyVisitDeclsFromUsd() const {
    return false;
}

void CustomStringConvertibleAnalysisPass::finalize(const clang::NamedDecl* namedDecl) {
    const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl);
    // We only need to do checking on this tagDecl and not worry about templates,
    // because specializations of a class template are all visited in VisitCXXRecord,
    // so we'll know we need to finalize them
    
    auto it = find(tagDecl);
    if (it->second._kind != CustomStringConvertibleAnalysisResult::unknown) {
        return;
    }
    
    const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
    if (!cxxRecordDecl) {
        // Could be an enum with an <<
        clang::QualType q = tagDecl->getTypeForDecl()->getCanonicalTypeUnqualified();
        if (_lessThanLessThanFunctionTypes.contains(q)) {
            insert_or_assign(tagDecl, CustomStringConvertibleAnalysisResult::available);
        } else {
            insert_or_assign(tagDecl, CustomStringConvertibleAnalysisResult::blockedByNoCandidate);
        }
        
        return;
    }
    
    // Given `std::ostream& operator<<(std::ostream&, const A&)` with `B` implicitly convertible to `A`,
    // we want to generate CustomStringConvertible for B.
    
    std::vector<clang::QualType> convertibleTypes;
    for (const clang::CXXRecordDecl* subtype : ASTHelpers::allAccessibleSupertypes(cxxRecordDecl)) {
        convertibleTypes.push_back(subtype->getTypeForDecl()->getCanonicalTypeUnqualified());
    }
    for (const clang::Type* convertibleType : ASTHelpers::allAccessibleImplicitNoArgConstConversions(cxxRecordDecl)) {
        convertibleTypes.push_back(convertibleType->getCanonicalTypeUnqualified());
    }
    
    clang::QualType templateType = ASTHelpers::getInjectedClassNameSpecialization(cxxRecordDecl->getTypeForDecl()->getCanonicalTypeInternal());
    if (!templateType.isNull()) {
        convertibleTypes.push_back(templateType);
    }
    
    // We just need to find one candidate. If there are multiple candidates by our heuristics,
    // we're going to assume the C++ compiler can decide on the right one to use,
    // and make this type CustomStringConvertible.
    int nCandidates = 0;
    for (uint64_t i = 0; i < convertibleTypes.size(); i++) {
        if (_lessThanLessThanFunctionTypes.contains(convertibleTypes[i].getCanonicalType())) {
            nCandidates += 1;
            break;
        } else if (convertibleTypes[i]->isArithmeticType()) {
            // Arithmetic types use a builtin operator<< that isn't in the AST.
            // Since we can't discover it by traversing the AST, we have to have a special case for it
#warning This might be missing conversions to pointers
            nCandidates += 1;
            break;
        }
    }
    
    insert_or_assign(cxxRecordDecl, nCandidates >= 1 ? CustomStringConvertibleAnalysisResult::available : CustomStringConvertibleAnalysisResult::blockedByNoCandidate);
}

bool CustomStringConvertibleAnalysisPass::VisitEnumDecl(clang::EnumDecl* enumDecl) {
    const FindEnumsAnalysisPass* findEnumsAnalysisPass = getASTAnalysisRunner().getFindEnumsAnalysisPass();
    if (findEnumsAnalysisPass->find(enumDecl) != findEnumsAnalysisPass->end()) {
        const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
        const auto& it = importAnalysisPass->find(enumDecl);
        if (it != importAnalysisPass->end() && it->second.isImportedSomehow() && !it->second.isImportedAsNonCopyable()) {
            insert_or_assign(enumDecl, CustomStringConvertibleAnalysisResult::availableEnum);
        }
    }
    return true;
}

bool CustomStringConvertibleAnalysisPass::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) {
    // We need to traverse the entire AST, even things not in Usd,
    // because we might need functions that are not defined in Usd for << for some types.
    // But, we want to only generate CustomStringConvertible conformances for types in Usd that are
    // imported into Swift, and ignore all others
    if (!isEarliestDeclLocFromUsd(cxxRecordDecl)) {
        return true;
    }
    if (!areAllUsdDeclsFromPublicHeaders(cxxRecordDecl)) {
        return true;
    }

    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    const auto& importIt = importAnalysisPass->find(cxxRecordDecl);
    if (importIt == importAnalysisPass->end()) {
        return true;
    }
    
    if (!importIt->second.isImportedSomehow()) {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::noAnalysisBecauseBlockedByImport);
        return true;
    }
    if (importIt->second.isImportedAsNonCopyable()) {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::noAnalysisBecauseNonCopyable);
        return true;
    }
    
    if (find(cxxRecordDecl) == end()) {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::unknown);
    }
    
    // Special case: UsdObject and its subclasses, which have GetDescription()
    const clang::CXXRecordDecl* usdObjectCxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(findTagDecl("class " PXR_NS"::UsdObject"));
    if (ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(cxxRecordDecl, usdObjectCxxRecordDecl)) {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::availableUsdObjectSubclass);
    }
    
    // Special case: SdfSpec and its subclasses, which we manually add for debugging purposes
    const clang::CXXRecordDecl* sdfSpecCxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(findTagDecl("class " PXR_NS"::SdfSpec"));
    if (ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(cxxRecordDecl, sdfSpecCxxRecordDecl)) {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::availableSdfSpecSubclass);
    }
    
    // Special case: SdfHandle specializations where the specialization argument is an SdfSpec or one of its subclass,
    // which we manually add for debugging purposes
    const clang::ClassTemplateSpecializationDecl* classTemplateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl);
    if (classTemplateSpecializationDecl) {
        const clang::ClassTemplateDecl* classTemplateDecl = classTemplateSpecializationDecl->getSpecializedTemplate();
        const clang::TemplateArgumentList& templateArgumentList = classTemplateSpecializationDecl->getTemplateInstantiationArgs();
        if (ASTHelpers::getAsString(classTemplateDecl) == "template <class T> class " PXR_NS"::SdfHandle" &&
            templateArgumentList.size() == 1) {
            clang::TemplateArgument templateArgument = templateArgumentList[0];
            if (templateArgument.getKind() == clang::TemplateArgument::ArgKind::Type) {
                const clang::CXXRecordDecl* templateArgumentAsCxxRecordDecl = templateArgument.getAsType()->getAsCXXRecordDecl();
                if (templateArgumentAsCxxRecordDecl && ASTHelpers::isEqualToOrDerivedFromClassVisibleToSwift(templateArgumentAsCxxRecordDecl, sdfSpecCxxRecordDecl)) {
                    insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::availableSdfSpecHandleSubclass);
                }
            }
        }
    }
    
    // Special case: UsdMetadataValueMap, which uses a custom comparator
    const clang::TagDecl* usdMetadataValueMapTagDecl = findTagDecl("class std::map<class " PXR_NS"::TfToken, class " PXR_NS"::VtValue, struct " PXR_NS"::TfDictionaryLessThan>");
    if (find(usdMetadataValueMapTagDecl) == end()) {
        insert_or_assign(usdMetadataValueMapTagDecl, CustomStringConvertibleAnalysisResult::availableUsdMetadataValueMap);
    }
    
    // Special case: UsdGeomXformOp, which we manually add for debugging purposes
    if (ASTHelpers::getAsString(cxxRecordDecl) == "class " PXR_NS"::UsdGeomXformOp") {
        insert_or_assign(cxxRecordDecl, CustomStringConvertibleAnalysisResult::availableUsdGeomXformOp);
    }

    // Don't set to unavailable yet, because CustomStringConvertible conformance
    // can be provided arbitrarily late, and it can be inherited
    
    return true;
}

bool CustomStringConvertibleAnalysisPass::VisitFunctionDecl(clang::FunctionDecl *functionDecl) {
    // Important: Don't disallow functions not from Usd,
    // because we might want to use `<<` functions from stdlib
    // (e.g., GfHalf)
    if (isEarliestDeclLocFromUsd(functionDecl) && !areAllUsdDeclsFromPublicHeaders(functionDecl)) {
#warning suspect use of declLocFromUsd
        return true;
    }
    
    // Must be <<
    if (functionDecl->getOverloadedOperator() != clang::OO_LessLess) {
        return true;
    }
    
    // Ignore if Swift can't see it
    if (functionDecl->isDeleted() || ASTHelpers::isNotVisibleToSwift(functionDecl->getAccess())) {
        return true;
    }
    
    if (functionDecl->getNumParams() != 2) {
        return true;
    }
    
    // Pull out the types of the arguments...
    clang::QualType lhsType = functionDecl->parameters()[0]->getType();
    clang::QualType rhsType = functionDecl->parameters()[1]->getType();
    clang::QualType retType = functionDecl->getReturnType();
    
    
    // Not sure this is the best way to do a check,
    // but I don't know a better way
    if (lhsType.getAsString() != "std::ostream &") {
        return true;
    }
    if (retType.getAsString() != "std::ostream &") {
        return true;
    }
    if (!rhsType->isBuiltinType()) {
        rhsType = ASTHelpers::removingRefConst(rhsType);
        if (rhsType.isNull()) {
            return true;
        }
    }
    
    
    // Arguments from Usd must be imported into swift and be copyable
    const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
    
    if (const clang::TagDecl* rhsTagDecl = rhsType->getAsTagDecl()) {
        if (doesTypeContainUsdTypes(rhsTagDecl)) {
            const auto& it = importAnalysisPass->find(rhsTagDecl);
            if (it == importAnalysisPass->end() || !it->second.isImportedSomehow() || it->second.isImportedAsNonCopyable()) {
                return true;
            }
        }
    }
    
    // We used to support templated types here for efficiency,
    // but that caused problems because we didn't handle things correctly
    std::vector<clang::QualType> rhsTypes = {rhsType};
    
    for (clang::QualType r : rhsTypes) {
        _lessThanLessThanFunctionTypes.insert(r.getCanonicalType());
    }
    
    return true;
}

