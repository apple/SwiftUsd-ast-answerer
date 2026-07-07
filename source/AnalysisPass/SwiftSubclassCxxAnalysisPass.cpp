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

#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/SwiftSubclassCxxAnalysisPass.h"

SwiftSubclassCxxAnalysisPass::SwiftSubclassCxxAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<SwiftSubclassCxxAnalysisPass, SwiftSubclassCxxAnalysisResult>(astAnalysisRunner) {}

std::string SwiftSubclassCxxAnalysisPass::serializationFileName() const {
    return "SwiftSubclassCxx.txt";
}

std::string SwiftSubclassCxxAnalysisPass::testFileName() const {
    return "testSwiftSubclassCxx.txt";
}

template <typename T>
T* canonicalize(T* x) {
    // Pure-virtual methods don't have definitions,
    // but all others do, and we want methods
    // to be canonicalized so we can just do pointer
    // comparison during comparesEqualWhileTesting
    if (x->getDefinition()) {
        return clang::dyn_cast<T>(x->getDefinition());
    } else {
        return x;
    }
}

void SwiftSubclassCxxAnalysisPass::process(const clang::CXXRecordDecl* cxxRecord) {
    if (find(cxxRecord) != end()) {
        return;
    }
    
    SwiftSubclassCxxAnalysisResult result;
    
    // bases, destructor, constructors, methods, fields
    
    for (const auto& it : cxxRecord->bases()) {
        const clang::CXXRecordDecl* baseRecord = clang::dyn_cast<clang::CXXRecordDecl>(it.getType()->getAsTagDecl());
        
        result.bases.push_back({ it.getAccessSpecifier(), canonicalize(baseRecord) });
        process(baseRecord);
    }
    result.destructor = canonicalize(cxxRecord->getDestructor());
    for (const clang::CXXConstructorDecl* ctor : cxxRecord->ctors()) {
        if (ctor->isCopyConstructor() ||
            ctor->isCopyAssignmentOperator() ||
            ctor->isMoveConstructor() ||
            ctor->isMoveAssignmentOperator() ||
            ctor->isOverloadedOperator()) {
            continue;
        }
        
        result.constructors.push_back(canonicalize(ctor));
    }
    
    for (const clang::CXXMethodDecl* method : cxxRecord->methods()) {
        if (method->isOverloadedOperator() ||
            clang::dyn_cast<clang::CXXConstructorDecl>(method) ||
            clang::dyn_cast<clang::CXXDestructorDecl>(method)) {
            continue;
        }
        result.methods.push_back(canonicalize(method));
    }
    // Important: `cxxRecord->methods()` only gives non-templated methods, but we also want to record templated methods,
    // so walk all the decls in the immediate context
    for (clang::Decl* decl : cxxRecord->decls()) {
        clang::FunctionTemplateDecl* functionTemplateDecl = clang::dyn_cast<clang::FunctionTemplateDecl>(decl);
        if (!functionTemplateDecl) { continue; }
        clang::CXXMethodDecl* method = clang::dyn_cast<clang::CXXMethodDecl>(functionTemplateDecl->getTemplatedDecl());

        if (method->isOverloadedOperator() ||
            clang::dyn_cast<clang::CXXConstructorDecl>(method) ||
            clang::dyn_cast<clang::CXXDestructorDecl>(method)) {
            continue;
        }
        result.methods.push_back(canonicalize(method));
    }
    
    for (const clang::FieldDecl* field : cxxRecord->fields()) {
        result.fields.push_back(field);
    }
    
    insert_or_assign(cxxRecord, result);
}

bool SwiftSubclassCxxAnalysisPass::VisitNamedDecl(clang::NamedDecl* namedDecl) {
    std::vector<std::string> toProcess = {
        "class " PXR_NS"::HioImage",
        "class " PXR_NS"::TfRefBase",
        "class " PXR_NS"::TfWeakBase",
        "class " PXR_NS"::SdfFileFormat"
    };
    
    for (const std::string& s : toProcess) {
        const clang::CXXRecordDecl* cxxRecord = clang::dyn_cast<clang::CXXRecordDecl>(findTagDecl(s));
        if (!cxxRecord) {
            std::cerr << "Error! Could not find CXXRecordDecl " << s << std::endl;
            __builtin_trap();
        }
        process(cxxRecord);
    }
    
    // Return false to stop the AST traversal immediately, because we don't need to do it
    return false;
}


void SwiftSubclassCxxAnalysisPass::insert_or_assign_while_deserializing(const clang::NamedDecl* namedDecl, const SwiftSubclassCxxAnalysisResult& analysisResult) {
    auto it = find(namedDecl);
    if (it == end()) {
        insert_or_assign(namedDecl, analysisResult);
    } else {
        it->second.merge(analysisResult);
    }
}

bool SwiftSubclassCxxAnalysisPass::comparesEqualWhileTesting(const SwiftSubclassCxxAnalysisResult& expected, const SwiftSubclassCxxAnalysisResult& actual) const {
    for (const auto& expectedBase : expected.bases) {
        bool hadAMatch = false;
        for (const auto& actualBase : actual.bases) {
            if (expectedBase.first == actualBase.first &&
                expectedBase.second == actualBase.second) {
                hadAMatch = true;
                break;
            }
        }
        
        if (!hadAMatch) { return false; }
    }

    for (const auto& expectedCtor : expected.constructors) {
        if (std::find(actual.constructors.begin(), actual.constructors.end(), expectedCtor) == actual.constructors.end()) {
            return false;
        }
    }
    
    if (expected.destructor != nullptr && expected.destructor != actual.destructor) {
        return false;
    }
    
    for (const auto& expectedMethod : expected.methods) {
        if (std::find(actual.methods.begin(), actual.methods.end(), expectedMethod) == actual.methods.end()) {
            return false;
        }
    }
    
    for (const auto& expectedField : expected.fields) {
        if (std::find(actual.fields.begin(), actual.fields.end(), expectedField) == actual.fields.end()) {
            return false;
        }
    }

    return true;
}
