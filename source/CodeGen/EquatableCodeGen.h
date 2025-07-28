//
//  EquatableCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/2/24.
//

#ifndef EquatableCodeGen_h
#define EquatableCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

#include "AnalysisPass/EquatableAnalysisPass.h"
#include "AnalysisPass/TypedefAnalysisPass.h"

// Code gen for conformances to `Equatable`
class EquatableCodeGen: public CodeGenBase<EquatableCodeGen> {
public:
    EquatableCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    virtual Data extraSpecialCaseFiltering(const Data& data) const override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                            std::string* outOverview,
                                                            const Data& processedData) override;
};

#endif /* EquatableCodeGen_h */
