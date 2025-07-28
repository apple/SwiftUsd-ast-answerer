//
//  TypedefAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/24.
//

#ifndef TypedefAnalysisResult_h
#define TypedefAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include <set>

class TypedefAnalysisPass;

struct TypedefAnalysisResult {
public:
    typedef std::pair<const clang::NamedDecl*, std::string> Pair;
    
    explicit operator std::string() const;
    
    TypedefAnalysisResult();
    static std::optional<TypedefAnalysisResult> deserialize(const std::string& data, const TypedefAnalysisPass* astAnalysisPass);
    
    void insert(const clang::NamedDecl* namedDecl, const std::string& typedefSpelling);
    
    bool isSubset(const TypedefAnalysisResult& other) const;
    
    const std::set<Pair>& getTypedefSpellings() const;
private:
    std::set<Pair> _typedefSpellings;
};
std::ostream& operator <<(std::ostream& os, const TypedefAnalysisResult& obj);

#endif /* TypedefAnalysisResult_h */
