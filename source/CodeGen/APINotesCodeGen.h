//
//  APINotesCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 7/12/24.
//

#ifndef APINotesCodeGen_h
#define APINotesCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "AnalysisPass/APINotesAnalysisPass.h"
#include "AnalysisResult/APINotesAnalysisResult.h"

struct NamespaceItem;
struct APINotesNode;

// Code gen for the API notes file
class APINotesCodeGen: public CodeGenBase<APINotesCodeGen> {
public:
    APINotesCodeGen(const CodeGenRunner* codeGenRunner);
    ~APINotesCodeGen();
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeAPINotesFile() override;
    
private:
    struct ReplacedMethod {
        const clang::CXXMethodDecl* method; // method->getParent() may not be owningType
        const clang::CXXRecordDecl* owningType; // may be a type that publicly inherits (directly or indirectly) from method->getParent()
        APINotesAnalysisResult analysisResult;
        
        bool operator<(const ReplacedMethod& other) const;
    };
    
    std::vector<ReplacedMethod> getReplacedMethods(const APINotesNode*);
    
    void writeReplaceMethod(ReplacedMethod replacedMethod,
                            bool isHeader,
                            std::map<const clang::CXXRecordDecl*, std::string>& importAsMemberTypedefs);
    
    std::unique_ptr<NamespaceItem> _root;

};

#endif /* APINotesCodeGen_h */
