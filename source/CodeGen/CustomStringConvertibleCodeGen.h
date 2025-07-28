//
//  CustomStringConvertibleCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/24/24.
//

#ifndef CustomStringConvertibleCodeGen_h
#define CustomStringConvertibleCodeGen_h

#include "CodeGen/CodeGenBase.h"

// Code gen for conformances to `CustomStringConvertible`
class CustomStringConvertibleCodeGen: public CodeGenBase<CustomStringConvertibleCodeGen> {
public:
    CustomStringConvertibleCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    virtual Data extraSpecialCaseFiltering(const Data& data) const override;
    void writeHeaderFile(const Data& data) override;
    void writeMmFile(const Data& data) override;
    void writeSwiftFile(const Data& data) override;
    std::vector<std::pair<std::string, Data>> writeDocCFile(std::string* outTitle,
                                                            std::string* outOverview,
                                                            const Data& processedData) override;

};

#endif /* CustomStringConvertibleCodeGen_h */

