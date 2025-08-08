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

#include "AnalysisPass/ASTAnalysisRunner.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/FindTfNoticeSubclassesAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/PublicInheritanceAnalysisPass.h"
#include "Util/CMakeParser.h"
#include <fstream>

FindTfNoticeSubclassesAnalysisPass::FindTfNoticeSubclassesAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<FindTfNoticeSubclassesAnalysisPass, FindTfNoticeSubclassesAnalysisResult>(astAnalysisRunner) {}

std::string FindTfNoticeSubclassesAnalysisPass::serializationFileName() const {
    return "FindTfNoticeSubclasses.txt";
}

std::string FindTfNoticeSubclassesAnalysisPass::testFileName() const {
    return "testFindTfNoticeSubclasses.txt";
}

bool FindTfNoticeSubclassesAnalysisPass::VisitTagDecl(clang::TagDecl* _) {
    // We can leverage the result of PublicInheritanceAnalysisPass
    // to make things go very fast, instead of traversing the whole AST
    
    const PublicInheritanceAnalysisPass* publicInheritanceAnalysisPass = getASTAnalysisRunner().getPublicInheritanceAnalysisPass();
    
    const clang::TagDecl* tfNotice = publicInheritanceAnalysisPass->findTagDecl("class " PXR_NS"::TfNotice");
    if (!tfNotice) {
        __builtin_trap();
    }
    _cache.insert({tfNotice, true});
    insert_or_assign(tfNotice, {});
    
    for (const auto& it : publicInheritanceAnalysisPass->getData()) {
        _checkInheritance(clang::dyn_cast<clang::TagDecl>(it.first));
    }
    
    // Return false to instantly stop traversal.
    return false;
}

bool FindTfNoticeSubclassesAnalysisPass::_checkInheritance(const clang::TagDecl* x) {
    // Check cache
    {
        const auto& it = _cache.find(x);
        if (it != _cache.end()) {
            return it->second;
        }
    }
    
    // Must be imported
    {
        const ImportAnalysisPass* importAnalysisPass = getASTAnalysisRunner().getImportAnalysisPass();
        const auto& it = importAnalysisPass->getData().find(x);
        if (it == importAnalysisPass->getData().end() || !it->second.isImportedSomehow()) {
            _cache.insert({x, false});
            return false;
        }
    }
    
    // If any public base inherits (directly or indirectly) from TfNotice,
    // then we do.
    const PublicInheritanceAnalysisPass* publicInheritanceAnalysisPass = getASTAnalysisRunner().getPublicInheritanceAnalysisPass();
    const auto& it = publicInheritanceAnalysisPass->getData().find(x);
    if (it != publicInheritanceAnalysisPass->getData().end()) {
        for (const clang::CXXRecordDecl* base : it->second.getPublicBases()) {
            if (_checkInheritance(base)) {
                _cache.insert({x, true});
                insert_or_assign(x, {});
                return true;
            }
        }
    }
    if (_cache.find(x) == _cache.end()) {
        // None of our public bases inherited from TfNotice,
        // so neither do we
        _cache.insert({x, false});
        return false;
    }
    
    __builtin_trap();
}
