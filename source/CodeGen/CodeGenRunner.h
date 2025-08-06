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

#ifndef CodeGenRunner_h
#define CodeGenRunner_h

#include "Driver/Driver.h"
#include "AnalysisPass/ASTAnalysisRunner.h"

class ReferenceTypeConformanceCodeGen;
class EquatableCodeGen;
//class OpenUSDSwiftModuleCodeGen;
//class ModulemapCodeGen;
class EnumsCodeGen;
class StaticTokensCodeGen;
class TfNoticeProtocolCodeGen;
class CustomStringConvertibleCodeGen;
class SdfValueTypeNamesMembersCodeGen;
class SchemaGetPrimCodeGen;
class HashableCodeGen;
class ComparableCodeGen;
class SendableCodeGen;
class APINotesCodeGen;

// Owns and coordinates running different code gen passes
class CodeGenRunner {
public:
    // MARK: Entry
    CodeGenRunner(const Driver* driver);
    ~CodeGenRunner();
    
public:
    // MARK: Accessors
    const Driver* getDriver() const;
    const FileSystemInfo& getFileSystemInfo() const;
    const ASTAnalysisRunner& getASTAnalysisRunner() const;
    const ReferenceTypeConformanceCodeGen* getReferenceTypeConformanceCodeGen() const;
    const EnumsCodeGen* getEnumsCodeGen() const;
    
private:
    // MARK: Fields
    const Driver* _driver;
    
    std::unique_ptr<ReferenceTypeConformanceCodeGen> _referenceTypeConformanceCodeGen;
    std::unique_ptr<EquatableCodeGen> _equatableCodeGen;
//    std::unique_ptr<OpenUSDSwiftModuleCodeGen> _openUSDSwiftModuleCodeGen;
//    std::unique_ptr<ModulemapCodeGen> _modulemapCodeGen;
    std::unique_ptr<EnumsCodeGen> _enumsCodeGen;
    std::unique_ptr<StaticTokensCodeGen> _staticTokensCodeGen;
    std::unique_ptr<TfNoticeProtocolCodeGen> _tfNoticeProtocolCodeGen;
    std::unique_ptr<CustomStringConvertibleCodeGen> _customStringConvertibleCodeGen;
    std::unique_ptr<SdfValueTypeNamesMembersCodeGen> _sdfValueTypeNamesMembersCodeGen;
    std::unique_ptr<SchemaGetPrimCodeGen> _schemaGetPrimCodeGen;
    std::unique_ptr<HashableCodeGen> _hashableCodeGen;
    std::unique_ptr<ComparableCodeGen> _comparableCodeGen;
    std::unique_ptr<SendableCodeGen> _sendableCodeGen;
    std::unique_ptr<APINotesCodeGen> _apiNotesCodeGen;
};


#endif /* CodeGenRunner_h */
