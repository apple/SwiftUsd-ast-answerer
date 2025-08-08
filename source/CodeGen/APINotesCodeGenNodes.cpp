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

#include "APINotesCodeGenNodes.h"
#include "APINotesCodeGen.h"

// MARK: APINotesNode
APINotesNode::APINotesNode(Kind kind) : kind(kind) {}

void APINotesNode::_writeName(std::vector<std::string> &lines, int indentation) const {
    // pass
}
void APINotesNode::_writeDeclContextChildren(std::vector<std::string> &lines, int indentation) const {
    // pass
}

void APINotesNode::write(std::vector<std::string> &lines, int indentation) const {
    _writeName(lines, indentation);
    _write(lines, indentation);
    _writeDeclContextChildren(lines, indentation);
}

void APINotesNode::walk(std::function<void (const APINotesNode *)> f) const {
    bool isDeclContext = false;
    
    f(this);
    switch (kind) {
        case Kind::NameField: // fallthrough
        case Kind::SwiftImportAsField: // fallthrough
        case Kind::UnavailableField: // fallthrough
        case Kind::SwiftNameField: // fallthrough
        case Kind::TfRemnantAsUnavailableImmortalFrtSpecialCaseField: break;
            
        case Kind::MethodItem:
            {
                const MethodItem& it = this->dyn_cast<MethodItem>();
                if (it.unavailable) {
                    it.unavailable->walk(f);
                }
            }
            isDeclContext = true;
            break;
            
        case Kind::TagItem:
            {
                const TagItem& it = this->dyn_cast<TagItem>();
                if (it.swiftImportAs) {
                    it.swiftImportAs->walk(f);
                }
                if (it.tfRemnantAsUnavailableImmortalFrtSpecialCaseField) {
                    it.tfRemnantAsUnavailableImmortalFrtSpecialCaseField->walk(f);
                }
            }
            isDeclContext = true;
            break;
            
        case Kind::NamespaceItem:
            isDeclContext = true;
            break;
    }
    
    if (isDeclContext) {
        const DeclContext* dc = static_cast<const DeclContext*>(this);
        dc->name->walk(f);
        if (dc->swiftName) { dc->swiftName->walk(f); }
        for (const auto& x : dc->methods) {
            x.second->walk(f);
        }
        for (const auto& x : dc->tags) {
            x.second->walk(f);
        }
        for (const auto& x : dc->namespaces) {
            x.second->walk(f);
        }
    }
}

// MARK: DeclContext

DeclContext::DeclContext(std::string name, const clang::Decl* decl, APINotesNode::Kind kind) :
APINotesNode(kind),
name(std::make_unique<NameField>(name)),
decl(decl)
{}

void DeclContext::_writeName(std::vector<std::string> &lines, int indentation) const {
    name->write(lines, indentation);
    if (swiftName) {
        swiftName->write(lines, indentation);
    }
}

template <typename T>
std::vector<const T*> _sortDeclContextChildren(const std::map<std::string, std::unique_ptr<T>>& children) {
    std::vector<const T*> result;
    for (const auto& it : children) {
        result.push_back(it.second.get());
    }
    
    std::sort(result.begin(), result.end(), [](const T* a, const T* b){
        return ASTHelpers::DeclComparator()(a->decl, b->decl);
    });
    return result;
}

void DeclContext::_writeDeclContextChildren(std::vector<std::string>& lines, int indentation) const {
    if (!methods.empty()) {
        lines.push_back(indent(indentation) + "Methods:");
        for (const auto& x : _sortDeclContextChildren(methods)) {
            x->write(lines, indentation + 1);
            lines.push_back("");
        }
    }
    
    if (!tags.empty()) {
        lines.push_back(indent(indentation) + "Tags:");
        for (const auto& x : _sortDeclContextChildren(tags)) {
            x->write(lines, indentation + 1);
            lines.push_back("");
        }
    }
    
    if (!namespaces.empty()) {
        lines.push_back(indent(indentation) + "Namespaces:");
        for (const auto& x : _sortDeclContextChildren(namespaces)) {
            x->write(lines, indentation + 1);
            lines.push_back("");
        }
    }
}

// MARK: NameField

NameField::NameField(std::string value) :
APINotesNode(Kind::NameField),
value(value) {}

void NameField::_write(std::vector<std::string>& lines, int indentation) const {
    if (withoutLeadingHyphen) {
        lines.push_back(indent(indentation) + "Name: " + value);
    } else {
        lines.push_back(indent(indentation - 1) + "- Name: " + value);
    }
}


// MARK: SwiftImportAsField
SwiftImportAsField::SwiftImportAsField(std::string importAs, std::string retainOp, std::string releaseOp) :
APINotesNode(Kind::SwiftImportAsField),
importAs(importAs),
retainOp(retainOp),
releaseOp(releaseOp) {}

void SwiftImportAsField::_write(std::vector<std::string>& lines, int indentation) const {
    lines.push_back(indent(indentation) + "SwiftImportAs: " + importAs);
    if (!retainOp.empty()) {
        lines.push_back(indent(indentation) + "SwiftRetainOp: " + retainOp);
    }
    if (!releaseOp.empty()) {
        lines.push_back(indent(indentation) + "SwiftReleaseOp: " + releaseOp);
    }
}

// MARK: UnavilableField

UnavailableField::UnavailableField() :
APINotesNode(Kind::UnavailableField) {}

void UnavailableField::_write(std::vector<std::string> &lines, int indentation) const {
    lines.push_back(indent(indentation) + "Availability: nonswift");
}

SwiftNameField::SwiftNameField(std::string value) :
APINotesNode(Kind::SwiftNameField),
value(value) {}

void SwiftNameField::_write(std::vector<std::string> &lines, int indentation) const {
    lines.push_back(indent(indentation) + "SwiftName: " + value);
}

TfRemnantAsUnavailableImmortalFrtSpecialCaseField::TfRemnantAsUnavailableImmortalFrtSpecialCaseField() :
APINotesNode(Kind::TfRemnantAsUnavailableImmortalFrtSpecialCaseField) {}

void TfRemnantAsUnavailableImmortalFrtSpecialCaseField::_write(std::vector<std::string> &lines, int indentation) const {
    UnavailableField().write(lines, indentation);
    SwiftImportAsField("reference", "immortal", "immortal").write(lines, indentation);
}

// MARK: MethodItem

MethodItem::MethodItem(std::string name, const clang::Decl* decl) : DeclContext(name, decl, Kind::MethodItem) {}

void MethodItem::addUnavailable() {
    unavailable = std::make_unique<UnavailableField>();
}

void MethodItem::addSwiftNameTfNoticeRegisterSpecialCaseField() {
    swiftName = std::make_unique<SwiftNameField>("__RegisterSwift(_:_:)");
}

void MethodItem::addRename(const clang::NamedDecl* target, APINotesAnalysisResult result) {
    addUnavailable();
    renamedMethods.insert({target, result});
}

void MethodItem::_write(std::vector<std::string> &lines, int indentation) const {
    if (unavailable) {
        unavailable->write(lines, indentation);
    }
}

// MARK: TagItem

TagItem::TagItem(std::string name, const clang::Decl* decl) : DeclContext(name, decl, Kind::TagItem) {}

void TagItem::addSwiftImportAs(std::string importAs, std::string retainOp, std::string releaseOp) {
    if (swiftImportAs) {
        std::cerr << "Error! Adding SwiftImportAs on a Tag that already has it" << std::endl;
        __builtin_trap();
    }
    swiftImportAs = std::make_unique<SwiftImportAsField>(importAs, retainOp, releaseOp);
}

void TagItem::addTfRemnantAsUnavailableImmortalFrtSpecialCaseField() {
    tfRemnantAsUnavailableImmortalFrtSpecialCaseField = std::make_unique<TfRemnantAsUnavailableImmortalFrtSpecialCaseField>();
}

void TagItem::addSwiftNameSdfZipFileIteratorSpecialCase() {
    swiftName = std::make_unique<SwiftNameField>("pxrIterator");
}

void TagItem::_write(std::vector<std::string> &lines, int indentation) const {
    if (swiftImportAs) {
        swiftImportAs->write(lines, indentation);
    }
    if (tfRemnantAsUnavailableImmortalFrtSpecialCaseField) {
        tfRemnantAsUnavailableImmortalFrtSpecialCaseField->write(lines, indentation);
    }
}

// MARK: NamespaceItem

NamespaceItem::NamespaceItem(std::string name, const clang::Decl* decl) : DeclContext(name, decl, Kind::NamespaceItem) {
    if (decl == nullptr) {
        this->name->withoutLeadingHyphen = true;
    }
}

void NamespaceItem::_write(std::vector<std::string> &lines, int indentation) const {
    // pass
}

void NamespaceItem::add(const clang::NamedDecl *target, APINotesAnalysisResult result) {
    if (decl) {
        std::cerr << "Error! NamespaceItem::add can only be called on the root namespace!" << std::endl;
        __builtin_trap();
    }
    
    // First, work backwards from the innermost clang decl context (the annotation target)
    // to the outermost clang decl context (the translation unit)
    std::vector<const clang::NamedDecl*> declContexts = {target};
    while (true) {
        const clang::DeclContext* nextContext = declContexts.front()->getDeclContext();
        if (clang::dyn_cast<clang::TranslationUnitDecl>(nextContext)) {
            break;
        }
        const clang::NamedDecl* nextNamed = clang::dyn_cast<clang::NamedDecl>(nextContext);
        if (!nextNamed) {
            std::cerr << ASTHelpers::getAsString(declContexts.front()) << " in a non-named decl context!" << std::endl;
            __builtin_trap();
        }
        declContexts.insert(declContexts.begin(), nextNamed);
    }
    
    // Now, work forwards from the outermost decl to the innermost decl,
    // adding new nodes as needed to our representation
    DeclContext* currentNode = this;
    for (const clang::NamedDecl* namedDecl : declContexts) {
        if (currentNode->dyn_cast_opt<MethodItem>()) {
            std::cout << "Cannot have decl nexted under MethodItem!" << std::endl;
            __builtin_trap();
        }
        
        std::string name = namedDecl->getNameAsString();
        if (clang::dyn_cast<clang::NamespaceDecl>(namedDecl)) {
            currentNode = _addIfNeeded(currentNode->namespaces, name, name, namedDecl);
            
        } else if (clang::dyn_cast<clang::TagDecl>(namedDecl) || clang::dyn_cast<clang::ClassTemplateDecl>(namedDecl)) {
            currentNode = _addIfNeeded(currentNode->tags, name, name, namedDecl);
            
        } else if (clang::dyn_cast<clang::CXXMethodDecl>(namedDecl)) {
            currentNode = _addIfNeeded(currentNode->methods, name, name, namedDecl);
            
        } else {
            std::cerr << ASTHelpers::getAsString(namedDecl) << " is an invalid decl context for API Notes" << std::endl;
            __builtin_trap();
        }
    }
    
    // Okay, lets apply the annotation to the current node
    std::string swiftNameParameters;
    if (const clang::FunctionDecl* functionDecl = clang::dyn_cast<clang::FunctionDecl>(target)) {
        swiftNameParameters = "(";
        for (int i = 0; i < functionDecl->parameters().size(); i++) {
            // Note: don't need a `self:` parameter, because that's only for ImportAsMember attachment
            // of global functions as methods. Renaming a method on a type doesn't attach new a new decl context
            swiftNameParameters += "_:";
        }
        swiftNameParameters += ")";
    }
    
    switch (result.getKind()) {
        case APINotesAnalysisResult::Kind::importTagAsOwned:
            currentNode->dyn_cast<TagItem>().addSwiftImportAs("owned", "", "");
            break;
            
        case APINotesAnalysisResult::Kind::importTagAsShared:
            currentNode->dyn_cast<TagItem>().addSwiftImportAs("reference",
                                                              "__retain" + APINotesCodeGen::mangleName(target),
                                                              "__release" + APINotesCodeGen::mangleName(target));
            break;
            
        case APINotesAnalysisResult::Kind::importTagAsImmortal:
            currentNode->dyn_cast<TagItem>().addSwiftImportAs("reference", "immortal", "immortal");
            break;
            
        case APINotesAnalysisResult::Kind::replaceConstRefFunctionWithCopyingWrapper: // fallthrough
        case APINotesAnalysisResult::Kind::replaceMutatingFunctionWithNonmutatingWrapper:
            currentNode->dyn_cast<MethodItem>().addRename(target, result);
            break;
            
        case APINotesAnalysisResult::Kind::makeFunctionUnavailable:
            currentNode->dyn_cast<MethodItem>().addUnavailable();
            break;
            
        case APINotesAnalysisResult::Kind::renameTfNoticeRegisterFunctionSpecialCase:
            currentNode->dyn_cast<MethodItem>().addSwiftNameTfNoticeRegisterSpecialCaseField();
            break;
            
        case APINotesAnalysisResult::Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase:
            currentNode->dyn_cast<TagItem>().addTfRemnantAsUnavailableImmortalFrtSpecialCaseField();
            break;
            
        case APINotesAnalysisResult::Kind::renameSdfZipFileIteratorSpecialCase:
            currentNode->dyn_cast<TagItem>().addSwiftNameSdfZipFileIteratorSpecialCase();
            break;
    }
}
