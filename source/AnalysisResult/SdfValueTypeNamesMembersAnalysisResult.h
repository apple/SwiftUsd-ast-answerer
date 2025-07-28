//
//  SdfValueTypeNamesMembersAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#ifndef SdfValueTypeNamesMembersAnalysisResult_h
#define SdfValueTypeNamesMembersAnalysisResult_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <optional>
#include <vector>

class SdfValueTypeNamesMembersAnalysisPass;

struct SdfValueTypeNamesMembersAnalysisResult {
public:
    explicit operator std::string() const;
    
    SdfValueTypeNamesMembersAnalysisResult();
    static std::optional<SdfValueTypeNamesMembersAnalysisResult> deserialize(const std::string& data, const SdfValueTypeNamesMembersAnalysisPass* astAnalysisPass);
    
    void push_back(const std::string& s);
    
private:
    friend class SdfValueTypeNamesMembersCodeGen;
    std::vector<std::string> _data;
};
std::ostream& operator <<(std::ostream& os, const SdfValueTypeNamesMembersAnalysisResult& obj);

#endif /* SdfValueTypeNamesMembersAnalysisResult_h */
