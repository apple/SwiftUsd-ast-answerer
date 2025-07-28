//
//  HashableAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#include "AnalysisResult/HashableAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

HashableAnalysisResult::operator std::string() const {
    switch (_kind) {
        case unknown: return "unknown";
        case blockedByImport: return "blockedByImport";
        case noAnalysisBecauseNonCopyable: return "noAnalysisBecauseNonCopyable";
        case foundCandidateButBlockedByEquatable: return "foundCandidateButBlockedByEquatable";
        case blockedByNoCandidate: return "blockedByNoCandidate";
        case available: return "available";
        default: return "__errCase";
    }
}

bool HashableAnalysisResult::isAvailable() const {
    switch (_kind) {
        case unknown: return false;
        case blockedByImport: return false;
        case noAnalysisBecauseNonCopyable: return false;
        case foundCandidateButBlockedByEquatable: return false;
        case blockedByNoCandidate: return false;
        case available: return true;
    }
}

/* static */
std::vector<HashableAnalysisResult::Kind> HashableAnalysisResult::allCases() {
    return {
        unknown,
        blockedByImport,
        noAnalysisBecauseNonCopyable,
        foundCandidateButBlockedByEquatable,
        blockedByNoCandidate,
        available
    };
}

HashableAnalysisResult::HashableAnalysisResult() : HashableAnalysisResult(unknown) {}

HashableAnalysisResult::HashableAnalysisResult(HashableAnalysisResult::Kind kind) : _kind(kind) {}

/* static */
std::optional<HashableAnalysisResult> HashableAnalysisResult::deserialize(const std::string &data, const HashableAnalysisPass* astAnalysisPass) {
    for (auto kind : allCases()) {
        if (std::string(HashableAnalysisResult(kind)) == data) {
            return HashableAnalysisResult(kind);
        }
    }
    
    return std::nullopt;
}

std::ostream& operator <<(std::ostream& os, const HashableAnalysisResult& obj) {
    return os << std::string(obj);
}

