//
//  FindEnumsAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#ifndef FindEnumsAnalysisResult_h
#define FindEnumsAnalysisResult_h

#include <vector>
#include <string>
#include <optional>

class FindEnumsAnalysisPass;

struct FindEnumsAnalysisResult {
public:
    bool isScoped;
    std::vector<std::string> caseNames;
    std::vector<int64_t> caseValues;
    
    explicit operator std::string() const;
    
    FindEnumsAnalysisResult(bool isScoped);
    void addCase(const std::string& name, int64_t value);
    static std::optional<FindEnumsAnalysisResult> deserialize(const std::string& data, const FindEnumsAnalysisPass*);
};
std::ostream& operator <<(std::ostream& os, const FindEnumsAnalysisResult& obj);

#endif /* FindEnumsAnalysisResult_h */
