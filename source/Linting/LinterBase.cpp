//===----------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer project authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#include "LinterBase.h"

void DefaultLinterResult::dumpLinterOutput(std::string msg) const {
    std::cout << std::endl;
    std::cout << "Linter results from " << msg << std::endl;
    std::cout << serialize() << std::endl;
    size_t nDenies = 0;
    for (const auto& x : _entries) {
        if (!x.allow) { nDenies += 1; }
    }
    std::cout << "Had " << nDenies << " den" << (nDenies == 1 ? "y" : "ies") << std::endl;
}

bool DefaultLinterResult::test(std::string allowDeny, std::string filePath, std::string line) const {
    std::optional<Entry> entry = Entry::deserialize(allowDeny, filePath, line);
    if (!entry) {
        std::cerr << "Failed to parse linter test line '" << allowDeny << "' '" << filePath << "' '" << line << "'" << std::endl;
        return false;
    }
    
    for (const auto& x : _entries) {
        if (x == *entry) {
            return true;
        }
    }
    
    return false;
}

bool DefaultLinterResult::deserialize(std::string allowDeny, std::string filePath, std::string line) {
    std::optional<Entry> entry = Entry::deserialize(allowDeny, filePath, line);
    if (!entry) {
        std::cerr << "Failed to deserialize linter line '" << allowDeny << "' '" << filePath << "' '" << line << "'" << std::endl;
        return false;
    }
    addEntry(*entry);
    return true;
}

std::string DefaultLinterResult::serialize() const {
    std::vector<Entry> sortedEntries;
    
    sortedEntries.insert(sortedEntries.end(), _entries.begin(), _entries.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), Entry::Comparator());
    
    std::stringstream ss;
    
    for (const auto& x : sortedEntries) {
        ss << x << std::endl;
    }
    return ss.str();
}

void DefaultLinterResult::addEntry(Entry entry) {
    _entries.insert(entry);
}

DefaultLinterResult::Entry::Entry(bool allow, std::string filePath, int lineNumber) :
allow(allow), filePath(filePath), lineNumber(lineNumber) {}

DefaultLinterResult::Entry::Entry(const ASTHelpers* helper, bool allow, const clang::Decl* decl) :
    allow(allow),
    filePath(helper->latestDeclLocFilePath(decl)),
    lineNumber(helper->expansionLineNumber(decl))
{
}

DefaultLinterResult::Entry::Entry(const ASTHelpers* helper, bool allow, const clang::Stmt* stmt) :
    allow(allow),
    filePath(helper->makePathRelativeForUsd(helper->stmtFilePath(stmt))),
    lineNumber(helper->expansionLineNumber(stmt))
{
}



bool DefaultLinterResult::Entry::operator==(const Entry & other) const {
    return allow == other.allow &&
           filePath == other.filePath &&
           lineNumber == other.lineNumber;
}

size_t DefaultLinterResult::Entry::HashFunction::operator()(const DefaultLinterResult::Entry& x) const {
    size_t result = 0;
    result ^= std::hash<bool>()(x.allow);
    result ^= std::hash<std::string>()(x.filePath);
    result ^= std::hash<int>()(x.lineNumber);
    return result;
}

bool DefaultLinterResult::Entry::Comparator::operator()(const DefaultLinterResult::Entry& a, const DefaultLinterResult::Entry& b) const {
    // allow before deny,
    // then filePath,
    // then line number
    
    if (a.allow && !b.allow) { return true; }
    if (!a.allow && b.allow) { return false; }
    if (a.filePath < b.filePath) { return true; }
    if (b.filePath < a.filePath) { return false; }
    return a.lineNumber < b.lineNumber;
}

std::optional<DefaultLinterResult::Entry> DefaultLinterResult::Entry::deserialize(std::string allowDeny, std::string filePath, std::string lineNumber) {
    DefaultLinterResult::Entry result{false, "", -1};
    
    if (allowDeny == "allow") {
        result.allow = true;
    } else if (allowDeny == "deny") {
        result.allow = false;
    } else {
        std::cerr << "Couldn't deserialize allowDeny '" << allowDeny << "'" << std::endl;
        return std::nullopt;
    }
    
    result.filePath = filePath;
    
    try {
        result.lineNumber = std::stoi(lineNumber);
    } catch (std::invalid_argument const&) {
        std::cerr << "Couldn't deserialize line number '" << lineNumber << "'" << std::endl;
        return std::nullopt;
    } catch (std::out_of_range const&) {
        std::cerr << "Couldn't deserialize line number '" << lineNumber << "'" << std::endl;
        return std::nullopt;
    }

    return result;
}

std::ostream& operator<<(std::ostream & os, DefaultLinterResult::Entry const& x)  {
    os << (x.allow ? "allow" : "deny") << "; ";
    os << x.filePath << "; ";
    os << x.lineNumber << ";";
    return os;
}

