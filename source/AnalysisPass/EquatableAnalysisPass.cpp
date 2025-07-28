//
//  EquatableAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/25/24.
//

#include "AnalysisPass/ImportAnalysisPass.h"
#include "AnalysisPass/EquatableAnalysisPass.h"


EquatableAnalysisPass::EquatableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
BinaryOpProtocolAnalysisPassBase<EquatableAnalysisPass>(astAnalysisRunner) {}

std::string EquatableAnalysisPass::serializationFileName() const {
    return "Equatable.txt";
}

std::string EquatableAnalysisPass::testFileName() const {
    return "testEquatable.txt";
}
