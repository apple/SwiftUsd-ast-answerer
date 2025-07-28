//
//  FindNamedDeclsAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>

FindNamedDeclsAnalysisResult::operator std::string() const {
    std::stringstream s;
    for (const auto& it : _namedDeclMap) {
        s << it.first << ";" << std::endl;
    }
    for (const auto& it : _typeMap) {
        s << it.first << ";" << std::endl;
    }
    return s.str();
}

FindNamedDeclsAnalysisResult::FindNamedDeclsAnalysisResult() = default;

/* static */
std::optional<FindNamedDeclsAnalysisResult> FindNamedDeclsAnalysisResult::deserialize(const std::string &data, const FindNamedDeclsAnalysisPass* astAnalysisPass) {
    return FindNamedDeclsAnalysisResult();
}

const FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisResult::getNamedDeclMap() const {
    return _namedDeclMap;
}
FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisResult::getNamedDeclMap() {
    return _namedDeclMap;
}

const FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisResult::getTypeMap() const {
    return _typeMap;
}
FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisResult::getTypeMap() {
    return _typeMap;
}

std::ostream& operator <<(std::ostream& os, const FindNamedDeclsAnalysisResult& obj) {
    return os << std::string(obj);
}


