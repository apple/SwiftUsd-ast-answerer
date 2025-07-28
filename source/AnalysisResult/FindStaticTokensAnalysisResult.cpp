//
//  FindStaticTokensAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 3/21/25.
//

#include "AnalysisResult/FindStaticTokensAnalysisResult.h"
#include "Util/TestDataLoader.h"

FindStaticTokensAnalysisResult::operator std::string() const {
    switch (kind) {
        case importedAsValue: return "importedAsValue";
        case blockedByNonPublicHeaderDefinition: return "blockedByNonPublicHeaderDefinition";
    }
}

/* static */
std::vector<FindStaticTokensAnalysisResult::Kind> FindStaticTokensAnalysisResult::allCases() {
    return {
        importedAsValue,
        blockedByNonPublicHeaderDefinition
    };
}

FindStaticTokensAnalysisResult::FindStaticTokensAnalysisResult(Kind theKind) : kind(theKind) {}

/* static */
std::optional<FindStaticTokensAnalysisResult> FindStaticTokensAnalysisResult::deserialize(const std::string& data, const FindStaticTokensAnalysisPass*) {
    for (auto kind : allCases()) {
        if (std::string(FindStaticTokensAnalysisResult(kind)) == data) {
            return FindStaticTokensAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const FindStaticTokensAnalysisResult& obj) {
    return os << std::string(obj);
}

