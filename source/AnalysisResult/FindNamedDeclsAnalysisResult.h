//
//  FindNamedDeclsAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#ifndef FindNamedDeclsAnalysisResult_h
#define FindNamedDeclsAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class FindNamedDeclsAnalysisPass;

struct FindNamedDeclsAnalysisResult {
    explicit operator std::string() const;
    
    typedef std::map<std::string, const clang::NamedDecl*> NamedDeclMap;
    typedef std::map<std::string, const clang::Type*> TypeMap;

    
    FindNamedDeclsAnalysisResult();
    static std::optional<FindNamedDeclsAnalysisResult> deserialize(const std::string& data, const FindNamedDeclsAnalysisPass* astAnalysisPass);
    
    const NamedDeclMap& getNamedDeclMap() const;
    NamedDeclMap& getNamedDeclMap();
    
    const TypeMap& getTypeMap() const;
    TypeMap& getTypeMap();
private:
    NamedDeclMap _namedDeclMap;
    TypeMap _typeMap;
};
std::ostream& operator <<(std::ostream& os, const FindNamedDeclsAnalysisResult& obj);



#endif /* FindNamedDeclsAnalysisResult_h */
