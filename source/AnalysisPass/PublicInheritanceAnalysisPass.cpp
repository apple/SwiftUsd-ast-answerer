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

#include "AnalysisPass/PublicInheritanceAnalysisPass.h"

PublicInheritanceAnalysisPass::PublicInheritanceAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<PublicInheritanceAnalysisPass, PublicInheritanceAnalysisResult>(astAnalysisRunner) {}

std::string PublicInheritanceAnalysisPass::serializationFileName() const {
    return "PublicInheritance.txt";
}

std::string PublicInheritanceAnalysisPass::testFileName() const {
    return "testPublicInheritance.txt";
}

bool PublicInheritanceAnalysisPass::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl) {
    if (!cxxRecordDecl->isThisDeclarationADefinition()) {
        return true;
    }
    
    std::vector<const clang::CXXRecordDecl*> publicBases;
    
    for (const auto& base : cxxRecordDecl->bases()) {
        switch (base.getAccessSpecifier()) {
            case clang::AS_none:
                if (cxxRecordDecl->isStruct()) {
                    // fallthrough
                } else {
                    break;
                }
                
            case clang::AS_public:
                if (const clang::CXXRecordDecl* baseRecord = base.getType()->getAsCXXRecordDecl()) {
                    publicBases.push_back(baseRecord);
                }
                break;
                
            case clang::AS_private: // fallthrough
            case clang::AS_protected: break;
        }
    }
    
    if (publicBases.size()) {
        insert_or_assign(cxxRecordDecl, PublicInheritanceAnalysisResult(publicBases));
    }
    return true;
}

void PublicInheritanceAnalysisPass::analysisPassIsFinished() {
    _analysisPassIsFinished = true;
}

std::unordered_set<const clang::CXXRecordDecl*> PublicInheritanceAnalysisPass::getPublicSubtypes(const clang::CXXRecordDecl* base) const {
    if (!_analysisPassIsFinished) {
        std::cerr << "Internal logic error! Can't call getPublicSubtypes before analysisPassIsFinished!" << std::endl;
        __builtin_trap();
    }
    
    {
        const auto& it = _publicSubtypes.find(base);
        if (it != _publicSubtypes.end()) {
            return it->second;
        }
    }
    
    std::unordered_set<const clang::CXXRecordDecl*> result = {base};
    
    for (const auto& it : getData()) {
        std::vector<const clang::CXXRecordDecl*> itBases = it.second.getPublicBases();
        if (std::find(itBases.begin(), itBases.end(), base) != itBases.end()) {
            std::unordered_set<const clang::CXXRecordDecl*> recurseResult = getPublicSubtypes(clang::dyn_cast<clang::CXXRecordDecl>(it.first));
            result.insert(recurseResult.begin(), recurseResult.end());
        }
    }
    
    _publicSubtypes.insert({base, result});
    return result;
}
