//
//  SendableCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/26/24.
//

#include "CodeGen/SendableCodeGen.h"
#include "AnalysisPass/SendableAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include <iostream>

SendableCodeGen::SendableCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<SendableCodeGen>(codeGenRunner) {}

std::string SendableCodeGen::fileNamePrefix() const {
    return "Sendable";
}

SendableCodeGen::Data SendableCodeGen::preprocess() {
    const ImportAnalysisPass* importAnalysisPass = getImportAnalysisPass();
    Data result;
    int i = 0;
    for (const auto& it : getSendableAnalysisPass()->getData()) {
        i += 1;
        if (i % 10000 == 0) {
            std::cout << "SendableCodeGen::preprocess(): " << i << " of " << getSendableAnalysisPass()->getData().size() << std::endl;
        }
        
        if (importAnalysisPass->find(it.first) == importAnalysisPass->end() || !importAnalysisPass->find(it.first)->second.isImportedSomehow()) {
            continue;
        }
        const clang::TagDecl* itFirst = clang::dyn_cast<clang::TagDecl>(it.first);
        if (!doesTypeContainUsdTypes(itFirst)) {
            continue;
        }
        
        if (hasTypeName<CppNameInCpp>(itFirst)) {
            result.push_back(itFirst);
        }
    }
    std::cout << "SendableCodeGen::preprocess(): Done (" << result.size() << " types returned)" << std::endl;
    
    return result;
}

void SendableCodeGen::writeSwiftFile(const SendableCodeGen::Data& data) {
    writeLine("// Conformance blocked by no Swift type name:");
    writeLine("");
    writeLine("// MARK: Available (blocked by no Swift type name)");
    writeLine("");
    clang::PrintingPolicy printingPolicy((clang::LangOptions()));
    printingPolicy.adjustForCPlusPlus();
    printingPolicy.SuppressTagKeyword = 1;
    
    for (const clang::TagDecl* tagDecl : data) {
        if (!getSendableAnalysisPass()->find(tagDecl)->second.isAvailable()) { continue; }
        writeThatTagDeclHasNoSwiftNameInSwiftIfNeeded(tagDecl);
    }
    
    writeLine("");
    writeLine("// MARK: Unavailable (blocked by no Swift type name)");
    writeLine("");
    for (const clang::TagDecl* tagDecl : data) {
        if (getSendableAnalysisPass()->find(tagDecl)->second.isAvailable()) { continue; }
        writeThatTagDeclHasNoSwiftNameInSwiftIfNeeded(tagDecl);
    }


    writeLine("");
    writeLine("");
    writeLine("// Conformance available:");
    writeLine("");
    writeLine("// MARK: Available");
    writeLine("");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInSwift>(tagDecl)) { continue; }
        if (!getSendableAnalysisPass()->find(tagDecl)->second.isAvailable()) { continue; }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string name = getTypeName<SwiftNameInSwift>(printer);
        writeLine("extension " + name + ": @unchecked Sendable {}");
    }
    
    writeLine("");
    writeLine("// MARK: Unavailable");
    writeLine("");
    for (const clang::TagDecl* tagDecl : data) {
        if (!hasTypeName<SwiftNameInSwift>(tagDecl)) { continue; }
        if (getSendableAnalysisPass()->find(tagDecl)->second.isAvailable()) { continue; }
        
        auto printer = typeNamePrinter(tagDecl);
        std::string name = getTypeName<SwiftNameInSwift>(printer);
        writeLine("@available(*, unavailable) extension " + name + ": @unchecked Sendable {}");
    }
}

bool SendableCodeGen::_isExcludedDuringSpecialCaseFiltering(const clang::TagDecl* tagDecl) const {
    // Type traits and internal implementation things
    std::vector<std::string> stlLeadingExclusions = {
        "std::integral_constant", "std::bool_constant", "std::is_void", "std::is_null_pointer", "std::is_integral", "std::is_floating_point", "std::is_array", "std::is_enum", "std::is_union", "std::is_class", "std::is_function", "std::is_pointer", "std::is_lvalue_reference", "std::is_rvalue_reference", "std::is_member_object_pointer", "std::is_member_function_pointer", "std::is_fundamental", "std::is_arithmetic", "std::is_scalar", "std::is_object", "std::is_compound", "std::is_reference", "std::is_member_pointer", "std::is_const", "std::is_volatile", "std::is_trivial", "std::is_trivially_copyable", "std::is_standard_layout", "std::is_pod", "std::is_literal_type", "std::has_unique_object_representation", "std::is_empty", "std::is_polymorphic", "std::is_abstract", "std::is_final", "std::is_aggregate", "std::is_implicit_lifetime", "std::is_signed", "std::is_unsigned", "std::is_bounded_array", "std::is_unbounded_array", "std::is_scoped_enum", "std::is_constructible", "std::is_trivially_constructible", "std::is_nothrow_constructible", "std::is_default_constructible", "std::is_trivially_default_constructible", "std::is_nothrow_default_constructible", "std::is_copy_constructible", "std::is_trivially_copy_constructible", "std::is_nothrow_copy_constructible", "std::is_move_constructible", "std::is_trivially_move_constructible", "std::is_nothrow_move_constructible", "std::is_assignable", "std::is_trivially_assignable", "std::is_nothrow_assignable", "std::is_copy_assignable", "std::is_trivially_copy_assignable", "std::is_nothrow_copy_assignable", "std::is_move_assignable", "std::is_trivially_move_assignable", "std::is_nothrow_move_assignable", "std::is_destructible", "std::is_trivially_destructible", "std::is_nothrow_destructible", "std::has_virtual_destructor", "std::is_swappable_with", "std::is_swappable", "std::is_nothrow_swappable_with", "std::is_nothrow_swappable", "std::alignment_of", "std::rank", "std::extent", "std::is_same", "std::is_base_of", "std::is_convertible", "std::is_nothrow_convertible", "std::is_layout_compatible", "std::is_pointer_interconvertible_base_of", "std::is_invocable", "std::is_invocable_r", "std::is_nothrow_invocable", "std::is_nothrow_invocable_r", "std::remove_cv", "std::remove_const", "std::remove_volatile", "std::add_cv", "std::add_const", "std::add_volatile", "std::remove_reference", "std::add_lvalue_reference", "std::add_rvalue_reference", "std::remove_pointer", "std::add_pointer", "std::make_signed", "std::make_unsigned", "std::remove_extent", "std::remove_all_extents", "std::aligned_storage", "std::aligned_union", "std::decay", "std::remove_cvref", "std::enable_if", "std::conditional", "std::common_type", "std::common_reference", "std::basic_common_reference", "std::underlying_type", "std::result_of", "std::invoke_result", "std::void_t", "std::type_identity", "std::conjunction", "std::disjunction", "std::negation", "std::is_pointer_interconvertible_with_class", "std::is_corresponding_member", "std::is_constant_evaluated", "std::is_within_lifetime", "std::iterator_traits", "std::_", "std::allocator", "std::iterator", "std::numeric_limits", "std::pointer_traits", "std::initializer_list", "std::is_error_condition_enum", "std::is_error_code_enum", "std::is_placeholder", "std::is_bind_expression", "std::reverse_iterator", "std::less", "std::greater", "std::not_equal", "std::move_iterator", "std::hash", "std::default_delete", "std::tuple_element", "std::equal_to", "std::tuple_size", "std::integer_sequence", "std::placeholders::__ph"
    };
    std::vector<std::string> pixarLeadingExclusions = {
        "pxr::VtValue::_TypeInfoImpl", "pxr::VtValue::_LocalTypeInfo", "pxr::TfSizeofType", "pxr::TfDeclarePtrs", "pxr::TfIterator", "pxr::TfMetaList", "pxr::GfIsGfVec", "pxr::GfIsGfMatrix", "pxr::GfIsGfQuat", "pxr::GfIsGfDualQuat", "pxr::GfIsFloatingPoint", "pxr::GfIsArithmetic", "pxr::GfIsGfRange", "pxr::VtIsArray", "pxr::VtValueTypeHasCheapCopy", "pxr::VtIsTypedValueProxy", "pxr::VtGetProxiedType", "pxr::VtIsErasedValueProxy", "pxr::VtIsValueProxy", "pxr::VtIsKnownValueType_Workaround", "pxr::VtValue::_Counted", "pxr::VtValue::_ArrayHelper", "pxr::VtValue::_ProxyHelper", "pxr::VtValue::_TypeInfoFor", "pxr::ArIsContextObject", "pxr::TsTraits", "pxr::PcpIteratorTraits", "pxr::UsdLinearInterpolationTraits", "pxr::HdTypedSampledDataSource", "pxr::HdRetainedTypedSampledDataSource", "pxr::UsdImagingPrimAdapterFactory", "pxr::SdfValueTypeTraits", "pxr::SdfAbstractDataTypedValue", "pxr::SdfAbstractDataConstTypedValue", "pxr::HdRetainedTypedSampledDataSource", "pxr::UsdImagingDataSourceAttribute"
    };
    
    std::vector<std::string> stlContaingExclusions = {
        "::__shared_ptr_default_delete",
        "std::_"
    };

    auto printer = typeNamePrinter(tagDecl);
    std::string cppTypeName = getTypeName<CppNameInCpp>(printer);
    for (const auto& exclusion : stlLeadingExclusions) {
        if (cppTypeName.starts_with(exclusion)) {
            return true;
        }
    }
    for (const auto& exclusion : pixarLeadingExclusions) {
        if (cppTypeName.starts_with(exclusion)) {
            return true;
        }
    }
    for (const auto& exclusion : stlContaingExclusions) {
        if (cppTypeName.find(exclusion) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

SendableCodeGen::Data SendableCodeGen::extraSpecialCaseFiltering(const Data& data) const {
    Data result;
    
    for (const auto& it : data) {
        if (_isExcludedDuringSpecialCaseFiltering(it)) {
            continue;
        }
        result.push_back(it);
    }
    
    std::cout << "SendableCodeGen::extraSpecialCaseFiltering: " << data.size() << " -> " << result.size() << std::endl;
    return result;
}

std::vector<std::pair<std::string, SendableCodeGen::Data>> SendableCodeGen::writeDocCFile(std::string* outTitle,
                                    std::string* outOverview,
                                    const SendableCodeGen::Data& processedData) {
    *outTitle = "Sendable protocol conformances";
    *outOverview =
    "This page lists which Usd types are and are not Sendable.\n"
    "It is generated based on heuristics that look at the source code for OpenUSD, "
    "and may have errors. If you spot a mistake, please let me know!\n"
    "\n"
    "DocC does not automatically list that a type is explicitly non-Sendable, "
    "so use this page as the source of truth for whether a type is Sendable or not.\n"
    "\n"
    "In Swift 6 mode, you should get compiler errors if you try to use an explicitly non-Sendable "
    "type from OpenUSD as if it were Sendable. You can try using `@preconcurrency import OpenUSD` to "
    "downgrade the error to a warning. If that doesn't work, you can completely suppress the diagnostic with\n"
    "```\n"
    "extension SomeType: @unchecked Sendable {}\n"
    "```\n"
    "Use this extension method sparingly!";
    
    std::vector<const clang::TagDecl*> sendable;
    std::vector<const clang::TagDecl*> nonSendable;
    
    for (const auto& it : processedData) {
        if (getSendableAnalysisPass()->find(it)->second.isAvailable()) {
            sendable.push_back(it);
        } else {
            nonSendable.push_back(it);
        }
    }
    
    return {
        {"Sendable", sendable },
        {"Non-Sendable", nonSendable }
    };
}
