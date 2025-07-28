//
//  ImportAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

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
