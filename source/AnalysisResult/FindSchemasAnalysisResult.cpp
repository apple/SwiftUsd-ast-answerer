//
//  FindSchemasAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#include "AnalysisResult/FindSchemasAnalysisResult.h"
#include "AnalysisPass/FindSchemasAnalysisPass.h"

FindSchemasAnalysisResult::FindSchemasAnalysisResult() {}

FindSchemasAnalysisResult::operator std::string() const { return "."; }

/* static */
std::optional<FindSchemasAnalysisResult> FindSchemasAnalysisResult::deserialize(const std::string& data, const FindSchemasAnalysisPass* astAnalysisPass) {
    return FindSchemasAnalysisResult();
}

std::ostream& operator <<(std::ostream& os, const FindSchemasAnalysisResult& obj) {
    return os << std::string(obj);
}
