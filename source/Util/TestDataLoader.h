//
//  TestDataLoader.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/16/24.
//

#ifndef TestDataLoader_h
#define TestDataLoader_h

#include "Util/ClangToolHelper.h"
#include <filesystem>
#include <vector>
#include <string>
#include <regex>

std::vector<std::string> splitStringByRegex(const std::string& str, const std::regex& regex);
std::string trimWhitespace(std::string str);

// Helps load test data from the `resources` directory.
class TestDataLoader {
public:
    enum class PxrNsReplacement {
        replace,
        dontReplace
    };
    
    static std::vector<std::vector<std::string>> load(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement);
    static std::vector<std::string> loadOneField(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement);
    static std::vector<std::pair<std::string, std::string>> loadTwoFields(const FileSystemInfo& info, const std::string& name, PxrNsReplacement replacement);
    static std::vector<std::vector<std::string>> load(const std::filesystem::path& f, PxrNsReplacement replacement);
};

#endif /* TestDataLoader_h */
