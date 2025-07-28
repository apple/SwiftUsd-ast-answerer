//
//  HashableCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/3/24.
//

#ifndef HashableCodeGen_h
#define HashableCodeGen_h

#include "CodeGen/CodeGenBase.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"

#include "AnalysisPass/HashableAnalysisPass.h"

// Code gen for conformances to `Hashable`
class HashableCodeGen: public CodeGenBase<HashableCodeGen> {
public:
    HashableCodeGen(const CodeGenRunner* codeGenRunner);
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    Data extraSpecialCaseFiltering(const Data& data) const override;
    void writeHeaderFile(const Data& data) override;
    void writeCppFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                            std::string* outOverview,
                                                            const Data& processedData) override;

};

#endif /* HashableCodeGen_h */
