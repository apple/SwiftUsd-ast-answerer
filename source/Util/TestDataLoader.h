// ===-------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer authors. All Rights Reserved. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at: 
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.     
// 
// SPDX-License-Identifier: Apache-2.0
// ===-------------------------------------------------------------------===//

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
