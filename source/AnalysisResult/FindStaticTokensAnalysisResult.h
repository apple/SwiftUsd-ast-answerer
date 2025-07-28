//
//  FindStaticTokensAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 3/21/25.
//

#ifndef FindStaticTokensAnalysisResult_h
#define FindStaticTokensAnalysisResult_h

#include <vector>
#include <string>
#include <optional>

class FindStaticTokensAnalysisPass;

struct FindStaticTokensAnalysisResult {
public:
    enum Kind {
        importedAsValue,
        blockedByNonPublicHeaderDefinition
    };
    
    explicit operator std::string() const;
    FindStaticTokensAnalysisResult(Kind kind);
    static std::optional<FindStaticTokensAnalysisResult> deserialize(const std::string& data, const FindStaticTokensAnalysisPass*);
    static std::vector<Kind> allCases();

    Kind kind;
};
std::ostream& operator <<(std::ostream& os, const FindStaticTokensAnalysisResult& obj);

#endif /* FindStaticTokensAnalysisResult_h */
