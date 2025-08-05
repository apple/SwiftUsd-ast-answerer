//
//  APINotesAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/25.
//

#include "AnalysisResult/APINotesAnalysisResult.h"
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/APINotesAnalysisPass.h"

APINotesAnalysisResult::operator std::string() const {
    switch (_kind) {
        case Kind::importTagAsShared: return "importTagAsShared";
        case Kind::importTagAsImmortal: return "importTagAsImmortal";
        case Kind::importTagAsOwned: return "importTagAsOwned";
        case Kind::makeFunctionUnavailable: return "makeFunctionUnavailable";
        case Kind::replaceMutatingFunctionWithNonmutatingWrapper: return "replaceMutatingFunctionWithNonmutatingWrapper";
        case Kind::replaceConstRefFunctionWithCopyingWrapper: return "replaceConstRefFunctionWithCopyingWrapper";
        case Kind::renameTfNoticeRegisterFunctionSpecialCase: return "renameTfNoticeRegisterFunctionSpecialCase";
        case Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase: return "markTfRemnantAsUnavailableImmortalFrtSpecialCase";
        case Kind::renameSdfZipFileIteratorSpecialCase: return "renameSdfZipFileIteratorSpecialCase";
    }
}

/* static */
std::vector<APINotesAnalysisResult::Kind> APINotesAnalysisResult::allCases() {
    return {
        Kind::importTagAsShared,
        Kind::importTagAsImmortal,
        Kind::importTagAsOwned,
        Kind::makeFunctionUnavailable,
        Kind::replaceMutatingFunctionWithNonmutatingWrapper,
        Kind::replaceConstRefFunctionWithCopyingWrapper,
        Kind::renameTfNoticeRegisterFunctionSpecialCase,
        Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase,
        Kind::renameSdfZipFileIteratorSpecialCase,
    };
}

APINotesAnalysisResult::APINotesAnalysisResult(APINotesAnalysisResult::Kind kind) : _kind(kind) {}

/* static */
std::optional<APINotesAnalysisResult> APINotesAnalysisResult::deserialize(const std::string& data, const APINotesAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(APINotesAnalysisResult(kind)) == data) {
            return APINotesAnalysisResult(kind);
        }
    }
    return std::nullopt;
}

APINotesAnalysisResult::Kind APINotesAnalysisResult::getKind() const {
    return _kind;
}

std::ostream& operator <<(std::ostream& os, const APINotesAnalysisResult& obj) {
    return os << std::string(obj);
}
