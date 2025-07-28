//
//  HashableAnalysis.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef HashableAnalysisResult_h
#define HashableAnalysisResult_h

#include "AnalysisPass/ASTAnalysisPass.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class HashableAnalysisPass;

struct HashableAnalysisResult {
public:
    enum Kind {
        unknown,
        blockedByImport,
        noAnalysisBecauseNonCopyable,
        foundCandidateButBlockedByEquatable,
        blockedByNoCandidate,
        available
    };
    
    bool isAvailable() const;
    
private:
    friend class HashableAnalysisPass;
    Kind _kind;

public:
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    
    HashableAnalysisResult();
    HashableAnalysisResult(Kind kind);
    static std::optional<HashableAnalysisResult> deserialize(const std::string& data, const HashableAnalysisPass* astAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const HashableAnalysisResult& obj);



#endif /* HashableAnalysisResult_h */
