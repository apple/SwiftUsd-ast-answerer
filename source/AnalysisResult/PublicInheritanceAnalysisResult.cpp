//
//  PublicInheritanceAnalysisResult.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 11/8/24.
//

#include "AnalysisResult/PublicInheritanceAnalysisResult.h"
#include "AnalysisPass/PublicInheritanceAnalysisPass.h"
#include <sstream>
#include "Util/TestDataLoader.h"
#include "AnalysisPass/ASTAnalysisPass.h"


PublicInheritanceAnalysisResult::operator std::string() const {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < _publicBases.size(); i++) {
        ss << ASTHelpers::getAsString(_publicBases[i]);
        if (i + 1 < _publicBases.size()) {
            ss << ",, ";
        }
    }
    ss << "]";
    return ss.str();
}

PublicInheritanceAnalysisResult::PublicInheritanceAnalysisResult(std::vector<const clang::CXXRecordDecl*> publicBases) : _publicBases(publicBases) {
    if (publicBases.size() == 0) {
        std::cerr << "Can't create PublicInheritanceAnalysisResult with empty list of public bases" << std::endl;
        __builtin_trap();
    }
    for (const clang::CXXRecordDecl* x : publicBases) {
        if (x == nullptr) {
            std::cout << "Can't create PublicInheritanceAnalysisResult with nullptr in public bases" << std::endl;
            __builtin_trap();
        }
    }
}

/*static*/
std::optional<PublicInheritanceAnalysisResult> PublicInheritanceAnalysisResult::deserialize(const std::string& data, const PublicInheritanceAnalysisPass* astAnalysisPass) {
    if (!data.starts_with("[") || !data.ends_with("]")) {
        return std::nullopt;
    }
    std::string dataTrimBrackets = data.substr(1, data.size() - 2);
    std::vector<std::string> components = splitStringByRegex(dataTrimBrackets, std::regex(",, "));
    
    std::vector<const clang::CXXRecordDecl*> publicBases;
    for (const auto& component : components) {
        if (!component.size()) {
            continue;
        }
        
        const clang::TagDecl* tagDecl = astAnalysisPass->findTagDecl(component);
        if (!tagDecl) {
            std::cerr << "Got bad tag while deserializing: " << component << std::endl;
            return std::nullopt;
            __builtin_trap();
        }
        const clang::CXXRecordDecl* cxxRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(tagDecl);
        if (!cxxRecordDecl) {
            std::cerr << "Got tag that isn't a CXXRecord while deserializing: " << component << std::endl;
            return std::nullopt;
            __builtin_trap();
        }
        publicBases.push_back(cxxRecordDecl);
    }
    if (publicBases.size() == 0) {
        std::cerr << "Got no bases while deserializing" << std::endl;
        return std::nullopt;
        __builtin_trap();
    }
    return PublicInheritanceAnalysisResult(publicBases);
}

std::ostream& operator <<(std::ostream& os, const PublicInheritanceAnalysisResult& obj) {
    return os << std::string(obj);
}

const std::vector<const clang::CXXRecordDecl*>& PublicInheritanceAnalysisResult::getPublicBases() const {
    return _publicBases;
}
