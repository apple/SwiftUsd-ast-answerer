//
//  FindTfNoticeSubclassesAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 1/7/25.
//

#include "AnalysisResult/FindTfNoticeSubclassesAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

FindTfNoticeSubclassesAnalysisResult::operator std::string() const {
    return ".";
}

std::optional<FindTfNoticeSubclassesAnalysisResult> FindTfNoticeSubclassesAnalysisResult::deserialize(const std::string& data, const FindTfNoticeSubclassesAnalysisPass* astAnalysisPass) {
    return FindTfNoticeSubclassesAnalysisResult();
}

std::ostream& operator <<(std::ostream& os, const FindTfNoticeSubclassesAnalysisResult& obj) {
    return os << std::string(obj);
}
