//
//  EquatableAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/25/24.
//

#ifndef EquatableAnalysisPass_h
#define EquatableAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/BinaryOpProtocolAnalysisPassBase.h"

// Analysis for the Swift `Equatable` protocol, based on BinaryOpProtocolAnalysisPassBase
class EquatableAnalysisPass final: public BinaryOpProtocolAnalysisPassBase<EquatableAnalysisPass> {
public:
    EquatableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
};

#endif /* EquatableAnalysisPass_h */
