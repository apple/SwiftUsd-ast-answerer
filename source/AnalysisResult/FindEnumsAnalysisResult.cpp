//
//  FindEnumsAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/6/24.
//

#include "AnalysisResult/FindEnumsAnalysisResult.h"
#include "Util/TestDataLoader.h"

FindEnumsAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    ss << (isScoped ? "scoped" : "unscoped") << ",, ";
    for (int i = 0; i < caseNames.size(); i++) {
        ss << caseNames[i] << ",, " << caseValues[i];
        if (i + 1 < caseNames.size()) {
            ss << ",, ";
        }
    }
    ss << "]";
    return ss.str();
}

FindEnumsAnalysisResult::FindEnumsAnalysisResult(bool isScoped) : isScoped(isScoped) {}

void FindEnumsAnalysisResult::addCase(const std::string &name, int64_t value) {
    const auto& it = std::find(caseNames.begin(), caseNames.end(), name);
    if (it != caseNames.end()) {
        if (caseValues[it - caseNames.begin()] != value) {
            std::cerr << "Got redeclaration of case with different value:" << std::endl;
            std::cerr << "'" << name << "': " << caseValues[it - caseNames.begin()] << " -> " << value << std::endl;
            __builtin_trap();
        }
        return;
    }
    
    caseNames.push_back(name);
    caseValues.push_back(value);
}

/* static */
std::optional<FindEnumsAnalysisResult> FindEnumsAnalysisResult::deserialize(const std::string& data, const FindEnumsAnalysisPass*) {
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    std::string dataTrimBrackets = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
    if (components.size() % 2 == 0) {
        return std::nullopt;
    }
    if (components[0] != "scoped" && components[0] != "unscoped") {
        return std::nullopt;
    }
    
    FindEnumsAnalysisResult result(components[0] == "scoped");
    for (int i = 1; i + 1 < components.size(); i += 2) {
        result.addCase(components[i], std::stoll(components[i + 1]));
    }
    return result;
}

std::ostream& operator <<(std::ostream& os, const FindEnumsAnalysisResult& obj) {
    return os << std::string(obj);
}
