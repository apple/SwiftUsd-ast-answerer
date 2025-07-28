//
//  TfNoticeProtocolCodeGen.h
//  ast-answerer
//
//  Created by Maddy Adams on 1/6/25.
//

#ifndef TfNoticeProtocolCodeGen_h
#define TfNoticeProtocolCodeGen_h

#include "CodeGen/CodeGenBase.h"

class TfNoticeProtocolCodeGen: public CodeGenBase<TfNoticeProtocolCodeGen> {
public:
    TfNoticeProtocolCodeGen(const CodeGenRunner* codeGenRunner);
    
    std::string fileNamePrefix() const override;
    Data preprocess() override;
    void writeSwiftFile(const Data& data) override;
};

#endif /* TfNoticeProtocolCodeGen_h */
