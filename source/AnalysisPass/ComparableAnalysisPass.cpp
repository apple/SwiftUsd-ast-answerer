//
//  ComparableAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/30/24.
//

#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/EquatableAnalysisPass.h"
#include "AnalysisPass/ComparableAnalysisPass.h"

ComparableAnalysisPass::ComparableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
BinaryOpProtocolAnalysisPassBase<ComparableAnalysisPass>(astAnalysisRunner) {}

std::string ComparableAnalysisPass::serializationFileName() const {
    return "Comparable.txt";
}

std::string ComparableAnalysisPass::testFileName() const {
    return "testComparable.txt";
}
