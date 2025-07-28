//
//  SdfValueTypeNamesMembersAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/31/24.
//

#include "Util/TestDataLoader.h"
#include "AnalysisResult/SdfValueTypeNamesMembersAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h"
#include <fstream>


SdfValueTypeNamesMembersAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    uint64_t i = 0;
    for (const auto& x : _data) {
        ss << x;
        if (i + 1 < _data.size()) {
            ss << ",, ";
        }
        i += 1;
    }
    ss << "]";
    return ss.str();
}

SdfValueTypeNamesMembersAnalysisResult::SdfValueTypeNamesMembersAnalysisResult() {}
/* static */
std::optional<SdfValueTypeNamesMembersAnalysisResult> SdfValueTypeNamesMembersAnalysisResult::deserialize(const std::string& _data, const SdfValueTypeNamesMembersAnalysisPass* astAnalysisPass) {
    std::string data = _data;
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    
    data = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(data, std::regex(",, "));
    SdfValueTypeNamesMembersAnalysisResult result;
    result._data = components;
    return result;
}

void SdfValueTypeNamesMembersAnalysisResult::push_back(const std::string &s) {
    _data.push_back(s);
}

std::ostream& operator<<(std::ostream& os, const SdfValueTypeNamesMembersAnalysisResult& obj) {
    return os << std::string(obj);
}
