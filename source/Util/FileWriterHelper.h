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

#ifndef FileWriterHelper_h
#define FileWriterHelper_h

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

// Helper class for writing files. Automatically generates prelude and epilogues based on the file type. 
class FileWriterHelper {
public:
    FileWriterHelper(const std::string& fileName);
    FileWriterHelper(const std::filesystem::path& directory,
                     const std::string& fileName);
    
    ~FileWriterHelper();
    
    void setFileName(const std::string& fileName);
    void setDirectory(const std::filesystem::path& directory);
    
    void openHeaderFile(const std::string& headerGuardToken = "");
    void openCppFile();
    void openMmFile();
    void openSwiftFile();
    void openModulemapFile();
    void openAPINotesFile();
    void openDocCFile();
    
    
    void addLine(const std::string& line);
    void addLines(const std::vector<std::string>& lines);
    void closeFile();
    
    bool hasOpenFile() const;
    std::string getOpenFileSuffix() const;
    
    bool getWritesPrologue() const;
    void setWritesPrologue(bool newValue);
    
private:
    void _openFile(const std::string& suffix);
    std::string _fileNameWithSuffix() const;
    
    std::vector<std::string> _getPrologue() const;
    std::vector<std::string> _getEpilogue() const;
    
    std::string _fileName;
    std::string _openFileSuffix;
    std::filesystem::path _directory;
    std::string _headerFileName;
    std::string _headerGuardToken;
    std::ofstream _stream;
    bool _writesPrologue;
    
    std::vector<std::string> __lines;
};

#endif /* FileWriterHelper_h */
