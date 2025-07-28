//
//  ComparableAnalysisPass.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/30/24.
//

#ifndef ComparableAnalysisPass_h
#define ComparableAnalysisPass_h

#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisPass/BinaryOpProtocolAnalysisPassBase.h"

// Analysis for the Swift `Comparable` protocol, based on BinaryOpProtocolAnalysisPassBase
class ComparableAnalysisPass final: public BinaryOpProtocolAnalysisPassBase<ComparableAnalysisPass> {
public:
    ComparableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner);
    
    std::string serializationFileName() const override;
    std::string testFileName() const override;
};

#endif /* ComparableAnalysisPass_h */
