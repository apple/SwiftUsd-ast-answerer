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

#ifndef ImportAnalysisResult_h
#define ImportAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class ImportAnalysisPass;

struct ImportAnalysisResult {
public:
    enum Kind {
        unknown,
        blockedByNonPublicHeaderDefinition,
        blockedByTemplatedRecordDecl,
        blockedByTemplateInstantiationArgs,
        blockedByParent,
        blockedByAccess,
        blockedByInaccessibleMove,
        blockedByInaccessibleDtor,
        importedAsValue,
        importedAsNonCopyable,
        importedAsSharedReference,
        importedAsImmortalReference
    };
    
    /// Returns true if a type is imported in some form, making
    /// no guarantees about what that form is. 
    bool isImportedSomehow() const;
    /// Returns true if a type is imported as a Copyable type
    bool isImportedAsValue() const;
    /// Returns true if a type is imported as a ~Copyable type
    bool isImportedAsNonCopyable() const;
    /// Returns true if a type is imported as a reference type in some form,
    /// making no guarantees about what that form is
    bool isImportedAsAnyReference() const;
    /// Returns true if a type is imported as a shared (retainable) reference
    bool isImportedAsSharedReference() const;
    /// Returns true if a type is imported as an immortal reference
    bool isImportedAsImmortalReference() const;
    
private:
    friend class ImportAnalysisPass;
    Kind _kind;

public:
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    
    ImportAnalysisResult();
    ImportAnalysisResult(Kind kind);
    static std::optional<ImportAnalysisResult> deserialize(const std::string& data, const ImportAnalysisPass* astAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const ImportAnalysisResult& obj);


#endif /* ImportAnalysisResult_h */
