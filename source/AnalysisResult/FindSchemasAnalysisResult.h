//
//  FindSchemasAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef FindSchemasAnalysisResult_h
#define FindSchemasAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindSchemasAnalysisPass;

struct FindSchemasAnalysisResult {
    explicit operator std::string() const;
    
    FindSchemasAnalysisResult();
    static std::optional<FindSchemasAnalysisResult> deserialize(const std::string& data, const FindSchemasAnalysisPass* astAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const FindSchemasAnalysisResult& obj);

#endif /* FindSchemasAnalysisResult */
