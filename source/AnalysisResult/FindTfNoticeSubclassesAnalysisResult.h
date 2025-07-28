//
//  FindTfNoticeSubclassesAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 1/7/25.
//

#ifndef FindTfNoticeSubclassesAnalysisResult_h
#define FindTfNoticeSubclassesAnalysisResult_h

#include "AnalysisPass/ASTAnalysisPass.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindTfNoticeSubclassesAnalysisPass;

struct FindTfNoticeSubclassesAnalysisResult {
    friend class FindTfNoticeSubclassesAnalysisPass;
    
public:
    explicit operator std::string() const;
    FindTfNoticeSubclassesAnalysisResult() = default;
    static std::optional<FindTfNoticeSubclassesAnalysisResult> deserialize(const std::string& data, const FindTfNoticeSubclassesAnalysisPass* astAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const FindTfNoticeSubclassesAnalysisResult& obj);


#endif /* FindTfNoticeSubclassesAnalysisResult_h */
