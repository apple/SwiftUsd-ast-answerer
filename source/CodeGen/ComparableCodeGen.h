//
//  ComparableCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/4/24.
//

#ifndef ComparableCodeGen_h
#define ComparableCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

#include "AnalysisPass/ComparableAnalysisPass.h"

// Code gen for conformances to `Comparable`
class ComparableCodeGen: public CodeGenBase<ComparableCodeGen> {
public:
    ComparableCodeGen(const CodeGenRunner* codeGenRunner);
    
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

#endif /* ComparableCodeGen_h */
