//
//  PublicInheritanceAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 11/8/24.
//

#ifndef PublicInheritanceAnalysisResult_h
#define PublicInheritanceAnalysisResult_h

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class PublicInheritanceAnalysisPass;


struct PublicInheritanceAnalysisResult {
    explicit operator std::string() const;
    
    PublicInheritanceAnalysisResult(std::vector<const clang::CXXRecordDecl*> publicBases);
    
    static std::optional<PublicInheritanceAnalysisResult> deserialize(const std::string& data, const PublicInheritanceAnalysisPass* astAnalysisPass);
    
    const std::vector<const clang::CXXRecordDecl*>& getPublicBases() const;
    
private:
    std::vector<const clang::CXXRecordDecl*> _publicBases;
};
std::ostream& operator <<(std::ostream& os, const PublicInheritanceAnalysisResult& obj);

#endif /* PublicInheritanceAnalysisResult_h */
