//
//  TestDataLoader.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/16/24.
//

#include "Util/TestDataLoader.h"
#include "Util/FileSystemInfo.h"
#include <fstream>
#include <regex>

void verifyFieldCount(const std::vector<std::vector<std::string>>& lines, int expected, const std::string& filename) {
    for (const auto& x : lines) {
        if (x.size() != expected) {
            std::cerr << filename << ": Expected ";
            if (expected == 1) {
                std::cerr << "1 field";
            } else {
                std::cerr << expected << " fields";
            }
            std::cerr << ", but got " << x.size() << std::endl;
            std::cerr << "[";
            for (uint64_t i = 0; i < x.size(); i++) {
                std::cerr << x[i];
                if (i + 1 < x.size()) {
                    std::cerr << ", ";
                }
            }
            std::cerr << "]" << std::endl;
            __builtin_trap();
        }
    }
}

/* static */
std::vector<std::vector<std::string>> TestDataLoader::load(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement) {
    return load(info.resourcesDirectoryPath / name, replacement);
}

/* static */
std::vector<std::string> TestDataLoader::loadOneField(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement) {
    std::vector<std::vector<std::string>> loaded = load(info, name, replacement);
    verifyFieldCount(loaded, 1, name);
    
    std::vector<std::string> result;
    for (const auto& x : loaded) {
        result.push_back(x[0]);
    }
    return result;
}

/* static */
std::vector<std::pair<std::string, std::string>> TestDataLoader::loadTwoFields(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement) {
    std::vector<std::vector<std::string>> loaded = load(info, name, replacement);
    verifyFieldCount(loaded, 2, name);
    
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& x : loaded) {
        result.push_back({x[0], x[1]});
    }
    
    return result;
}

std::vector<std::string> splitStringByDelimeter(std::string str, const std::string& delim) {
    std::vector<std::string> result;
    
    while (str.size() != 0) {
        std::string token = str.substr(0, str.find(delim));
        result.push_back(token);
        if (token.size() == str.size()) {
            break;
        }
        str = str.substr(token.size() + delim.size());
    }
    
    return result;
}

std::vector<std::string> splitStringByRegex(const std::string& str, const std::regex& regex) {
    std::vector<std::string> result;
    std::sregex_token_iterator it = std::sregex_token_iterator(str.begin(), str.end(), regex, -1);
    return {it, {}};
}

std::string trimWhitespace(std::string str) {
    // Trim from start
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch){
        return !std::isspace(ch);
    }));
    
    // Trim from end
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch){
        return !std::isspace(ch);
    }).base(), str.end());
    
    return str;
}

std::vector<std::string> _splitLineBySemicolonNotSurroundedBySingleQuotes(const std::string& s) {
    std::vector<std::string> result;
    int startCopy = 0;
    for (int endCopy = 0; endCopy < s.size(); endCopy++) {
        if (s[endCopy] == ';') {
            if (endCopy != 0 && s[endCopy - 1] == '\'' && endCopy + 1 < s.size() && s[endCopy + 1] == '\'') {
                continue;
            }
            
            result.push_back(s.substr(startCopy, endCopy - startCopy));
            endCopy += 1;
            startCopy = endCopy;
        }
    }
    if (startCopy < s.size()) {
        result.push_back(s.substr(startCopy, s.size() - startCopy));
    }
    return result;
}


/* static */
std::vector<std::vector<std::string>> TestDataLoader::load(const std::filesystem::path& f, PxrNsReplacement replacement) {
    if (!std::filesystem::exists(f)) {
        std::cerr << "Error! No test file " << f.string() << " to load" << std::endl;
        __builtin_trap();
    }
    
    std::vector<std::vector<std::string>> result;
    
    std::ifstream infile(f);
    std::string line;
    while (std::getline(infile, line)) {
        if (replacement == PxrNsReplacement::replace) {
            line = std::regex_replace(line, std::regex("PXR_NS"), PXR_NS);
        }
        if (line.size() == 0) {
            continue;
        }
        std::string stripComment = splitStringByDelimeter(line, "//")[0];
        if (stripComment.size() == 0) {
            continue;
        }
        
        // Don't use a regex, because it's really slow. Changing to purpose built parser saves 2 minutes.
        std::vector<std::string> untrimmedFields = _splitLineBySemicolonNotSurroundedBySingleQuotes(stripComment);
        for (auto& x : untrimmedFields) {
            x = trimWhitespace(x);
        }
        for (long i = untrimmedFields.size() - 1; i >= 0; i--) {
            if (untrimmedFields[i].size() == 0) {
                untrimmedFields.erase(untrimmedFields.begin() + i);
            }
        }
        if (untrimmedFields.size() != 0) {
            result.push_back(untrimmedFields);
        }
    }
    
    return result;
}
