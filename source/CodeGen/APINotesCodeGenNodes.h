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

#ifndef APINotesCodeGenNodes_h
#define APINotesCodeGenNodes_h

#include <memory>
#include <string>
#include <vector>
#include "clang/AST/RecursiveASTVisitor.h"
#include "AnalysisResult/APINotesAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisPass.h"

// An abstract node used for writing API Notes files
// and their associated synthesized source code
struct APINotesNode {
    // Note: This project builds without RTTI, so we
    // need to build our own dynamic casting meachanisms.
    // Nodes aren't bitfields, but using bitfields for
    // the enum makes it easier to spot obvious bugs
    enum class Kind {
        NameField = 1 << 0,
        SwiftImportAsField = 1 << 1,
        UnavailableField = 1 << 2,
        SwiftNameField = 1 << 3,
        TfRemnantAsUnavailableImmortalFrtSpecialCaseField = 1 << 4,
        MethodItem = 1 << 5,
        TagItem = 1 << 6,
        NamespaceItem = 1 << 7,
    };
    
    APINotesNode(Kind kind);
    
    // Non-copyable and non-movable
    APINotesNode(const APINotesNode&) = delete;
    APINotesNode& operator=(const APINotesNode&) = delete;
    APINotesNode(APINotesNode&&) = delete;
    APINotesNode& operator=(APINotesNode&&) = delete;
    virtual ~APINotesNode() = default;
    
    // Visits this node and all its children, recursively
    void walk(std::function<void(const APINotesNode*)> f) const;
    
protected:
    // DeclContext overrides these.
    virtual void _writeName(std::vector<std::string>& lines, int indentation) const;
    virtual void _writeDeclContextChildren(std::vector<std::string>& lines, int indentation) const;
    // Concrete subclasses override this method, appending their lines to
    // `lines` with the given amount of `indentation`
    virtual void _write(std::vector<std::string>& lines, int indentation) const = 0;
public:
    void write(std::vector<std::string>& lines, int indentation) const;
    
    // Produces the proper whitespace to indent a line by a given amount
    std::string indent(int indentation) const;
    
    // Dynamically casts `this` to a concrete subclass. Returns
    // nullptr to indicate that casting failed
    template <typename T>
    T* dyn_cast_opt();
    
    template <typename T>
    const T* dyn_cast_opt() const;
    
    // Like dyn_cast_opt, but traps instead of returning nullptr
    // on failed casts
    template <typename T>
    T& dyn_cast() {
        T* x = dyn_cast_opt<T>();
        if (x) {
            return *x;
        } else {
            __builtin_trap();
        }
    }
    template <typename T>
    const T& dyn_cast() const {
        const T* x = dyn_cast_opt<T>();
        if (x) {
            return *x;
        } else {
            __builtin_trap();
        }
    }
    
    const Kind kind;
};

// Forward declares for DeclContext
struct NameField;
struct SwiftNameField;
struct MethodItem;
struct TagItem;
struct NamespaceItem;

// An item in a list. Holds a Decl for sorting purposes.
// Can hold nested methods, tags, and namespaces.
struct DeclContext: APINotesNode {
    std::unique_ptr<NameField> name;
    std::unique_ptr<SwiftNameField> swiftName;
    std::map<std::string, std::unique_ptr<MethodItem>> methods;
    std::map<std::string, std::unique_ptr<TagItem>> tags;
    std::map<std::string, std::unique_ptr<NamespaceItem>> namespaces;
    const clang::Decl* decl;

    DeclContext(std::string name, const clang::Decl* decl, APINotesNode::Kind kind);
    void _writeName(std::vector<std::string>& lines, int indentation) const override final;
    void _writeDeclContextChildren(std::vector<std::string>& lines, int indentation) const override final;
};


// MARK: Simple fields

// The `Name:` field
struct NameField: APINotesNode {
    std::string value;
    bool withoutLeadingHyphen = false;
    
    NameField(std::string value);
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// The `SwiftImportAs:` field, along with the `SwiftRetainOp:` and `SwiftReleaseOp:` fields
// when importing as reference
struct SwiftImportAsField: APINotesNode {
    std::string importAs;
    std::string retainOp;
    std::string releaseOp;
    
    SwiftImportAsField(std::string importAs, std::string retainOp, std::string releaseOp);
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// The `Availability:` field, set to `nonswift`
struct UnavailableField: APINotesNode {
    UnavailableField();
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// The `SwiftName:` field
struct SwiftNameField: APINotesNode {
    std::string value;
    
    SwiftNameField(std::string value);
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// A combination of Unavailable and immortal FRT
struct TfRemnantAsUnavailableImmortalFrtSpecialCaseField: APINotesNode {
    TfRemnantAsUnavailableImmortalFrtSpecialCaseField();
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// MARK: Items

// An item in a `Methods:` list
// (Note: not a true DeclContext, doesn't support nesting under itself,
// but it's easier to check for this at runtime and pretend that it could support nesting)
struct MethodItem: DeclContext {
    std::unique_ptr<UnavailableField> unavailable;
    std::map<const clang::NamedDecl*, APINotesAnalysisResult> renamedMethods;
    
    MethodItem(std::string name, const clang::Decl* decl);
    void addRename(const clang::NamedDecl* target, APINotesAnalysisResult result);
    void addUnavailable();
    void addSwiftNameTfNoticeRegisterSpecialCaseField();
    void _write(std::vector<std::string>& lines, int indentation) const override;
};

// An item in a `Tags:` list
struct TagItem: DeclContext {
    std::unique_ptr<SwiftImportAsField> swiftImportAs;
    std::unique_ptr<TfRemnantAsUnavailableImmortalFrtSpecialCaseField> tfRemnantAsUnavailableImmortalFrtSpecialCaseField;
    
    TagItem(std::string name, const clang::Decl* decl);
    void addSwiftImportAs(std::string importAs, std::string retainOp, std::string releaseOp);
    void addTfRemnantAsUnavailableImmortalFrtSpecialCaseField();
    void addSwiftNameSdfZipFileIteratorSpecialCase();
    void _write(std::vector<std::string>& lines, int indentation) const;
};

// An item in a `Namespaces:` list
struct NamespaceItem: DeclContext {
    
    NamespaceItem(std::string name, const clang::Decl* decl);
    void _write(std::vector<std::string>& lines, int indentation) const;
    
    // Adds an annotation to a NamedDecl, creating fields and items
    // in the tree as needed. Should only be called on the root NamespaceItem
    void add(const clang::NamedDecl* target, APINotesAnalysisResult result);
    
private:
    // Adds the item to the map if no key with the given name is in the map,
    // then returns the value now associated with the given key
    template <typename T, class... Args >
    static DeclContext* _addIfNeeded(std::map<std::string, std::unique_ptr<T>>& map,
                                     std::string name, Args&&... args);
};

// MARK: Template implementations

template <typename T>
T* APINotesNode::dyn_cast_opt() {
#define DYN_CAST_IMPL(U) \
if constexpr(std::is_same_v<T, U>) {\
return kind == APINotesNode::Kind::U ? static_cast<T*>(this) : nullptr;\
} else
    DYN_CAST_IMPL(NameField)
    DYN_CAST_IMPL(SwiftImportAsField)
    DYN_CAST_IMPL(UnavailableField)
    DYN_CAST_IMPL(SwiftNameField)
    DYN_CAST_IMPL(TfRemnantAsUnavailableImmortalFrtSpecialCaseField)
    DYN_CAST_IMPL(MethodItem)
    DYN_CAST_IMPL(TagItem)
    DYN_CAST_IMPL(NamespaceItem)
    {
        static_assert(false, "Cannot dynamic cast APINotesNode to T");
    }
    
#undef DYN_CAST_IMPL
}

template <typename T>
const T* APINotesNode::dyn_cast_opt() const {
    return const_cast<const T*>(const_cast<APINotesNode*>(this)->dyn_cast_opt<T>());
}

template <typename T, class... Args >
DeclContext* NamespaceItem::_addIfNeeded(std::map<std::string, std::unique_ptr<T>>& map,
                                                      std::string name, Args&&... args) {
    const auto& it = map.find(name);
    if (it == map.end()) {
        map.insert({name, std::make_unique<T>(args...)});
    }
    return map.find(name)->second.get();
}

#endif // APINotesCodeGenNodes_h
