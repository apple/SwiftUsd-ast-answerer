//
//  SendableCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/26/24.
//

#ifndef SendableCodeGen_h
#define SendableCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for conformances to `Sendable`.
// Note: all the complicated analysis happens during SendableAnalysisPass.
// This code gen pass just works by rote. 
class SendableCodeGen: public CodeGenBase<SendableCodeGen> {
public:
    SendableCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeSwiftFile(const Data& data) override;
    Data extraSpecialCaseFiltering(const Data& data) const override;
    
    bool _isExcludedDuringSpecialCaseFiltering(const clang::TagDecl* tagDecl) const;
    
    std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                            std::string* outOverview,
                                                            const Data& processedData) override;
};

#endif /* SendableCodeGen_h */
