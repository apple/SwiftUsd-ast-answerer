//
//  SendableAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/14/24.
//

#include "AnalysisPass/SendableAnalysisPass.h"
#include "AnalysisPass/FindSendableDependenciesAnalysisPass.h"
#include "Util/Graph.h"

SendableAnalysisPass::SendableAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) : ASTAnalysisPass<SendableAnalysisPass, SendableAnalysisResult>(astAnalysisRunner) {}

std::string SendableAnalysisPass::serializationFileName() const {
    return "Sendable.txt";
}

std::string SendableAnalysisPass::testFileName() const {
    return "testSendable.txt";
}

bool SendableAnalysisPass::VisitNamedDecl(clang::NamedDecl *namedDecl) {
    const FindSendableDependenciesAnalysisPass* findSendableDependenciesAnalysisPass = getASTAnalysisRunner().getFindSendableDependenciesAnalysisPass();
        
    DirectedGraph<const clang::Type*> dependencyGraph;
    
    std::cout << "Will build dependency graph" << std::endl;
    for (const auto& it : findSendableDependenciesAnalysisPass->getData()) {
        const clang::Type* node = clang::dyn_cast<clang::TagDecl>(it.first)->getTypeForDecl();
        
        // Make sure this node exists in the graph, even if it doesn't have any outgoing edges
        if (node->isCanonicalUnqualified()) {
            dependencyGraph.addNode(node);
        }
        
        std::vector<FindSendableDependenciesAnalysisResult::Dependency> edges = it.second.dependencies;
        for (const auto& edge : edges) {
            switch (edge.kind) {
                case FindSendableDependenciesAnalysisResult::inheritance: // fallthrough
                case FindSendableDependenciesAnalysisResult::field: // fallthrough
                    dependencyGraph.addEdge(node, edge.type);
                    break;
                    
                case FindSendableDependenciesAnalysisResult::specialAvailable:
                    insert_or_assign(it.first, SendableAnalysisResult::available);
                    break;
                    
                case FindSendableDependenciesAnalysisResult::specialImportedAsReference:
                    insert_or_assign(it.first, SendableAnalysisResult(SendableAnalysisResult::unavailable, it.second));
                    break;
                                     
                case FindSendableDependenciesAnalysisResult::specialConditional:
                    dependencyGraph.addEdge(node, edge.type);
                    break;
            }
        }
    }
    
    std::cout << "Will build SCCs" << std::endl;
    std::vector<std::unique_ptr<std::set<const clang::Type*>>> outSCCs;
    std::map<const clang::Type*, std::set<const clang::Type*>*> outToSCCMapping;
    DirectedGraph<std::set<const clang::Type*>*> outDirectedGraph;
    dependencyGraph.findStronglyConnectedComponents(outSCCs, outToSCCMapping, outDirectedGraph);
    
    std::cout << "Analyzing SCCs" << std::endl;
    
    // From Tarjan, outSCCs are in reverse topo sort order (sinks to sources)
    for (const auto& scc : outSCCs) {
        // First, figure out if we have any unavailable dependencies.
        SendableAnalysisResult result(SendableAnalysisResult::available);
        
        bool hasAlreadyBeenDecidedBySpecialCase = false;
        for (const auto& node : *scc) {
            if (find(node->getAsTagDecl()) != end()) {
                hasAlreadyBeenDecidedBySpecialCase = true;
            }
        }
        if (hasAlreadyBeenDecidedBySpecialCase) {
            continue;
        }
                
        // Look at all the SCCs we depend on, our neighborSCCs
        auto neighborSCCs = outDirectedGraph.neighbors(scc.get());
        for (const auto& neighborSCC : neighborSCCs) {
            // This particular neighborSCC is Sendable iff all the individual clang::Type* nodes
            // in it are Sendable, so check all those individual nodes.
            for (const clang::Type* neighborNode : *neighborSCC) {
                if (!_isSendable(neighborNode)) {
                    // Our neighbor isn't Sendable, so we won't be either.
                    // Prepare result so we can say we're blocked by this neighbor
                    if (result._kind == SendableAnalysisResult::available) {
                        result._unavailableDependencies.dependencies = {};
                    }
                    result._kind = SendableAnalysisResult::unavailable;
                    
                    
                    // We don't know why we have a dependency on this neighbor yet.
                    // (It might be a path, not just a single edge.)
                    // But, there must be someone in our own SCC that has an edge
                    // to the individual neighborNode that isn't Sendable. So,
                    // we want that to be part of the unavailable reasoning
                    for (const clang::Type* nodeInThisScc : *scc) {
                        if (dependencyGraph.hasEdge(nodeInThisScc, neighborNode)) {
                            // We found a pair of nodes with an edge between them,
                            // where the first node is in our SCC,
                            // and the second node is in the non-Sendable neighboring SCC.
                            const auto& it = findSendableDependenciesAnalysisPass->find(nodeInThisScc->getAsTagDecl());
                            for (const auto& dependency : it->second.dependencies) {
                                if (dependency.type == neighborNode) {
                                    // Okay, we finally have the actual dependency edge. Add it to the result.
                                    result._unavailableDependencies.dependencies.push_back(dependency);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Now, assign to all the nodes in our sccNode
        for (const clang::Type* node : *scc) {
            if (node->isCanonicalUnqualified() && node->getAsTagDecl()) {
                insert_or_assign(node->getAsTagDecl(), result);
            }
        }
    }
    
    
    // Return false to stop the AST traversal immediately, because we don't need to do it
    return false;
}


bool SendableAnalysisPass::_isSendable(const clang::TagDecl *tagDecl) const {
    if (find(tagDecl) != end()) {
        return find(tagDecl)->second.isAvailable();
    }
    
    const FindSendableDependenciesAnalysisPass* findSendableDependenciesAnalysisPass = getASTAnalysisRunner().getFindSendableDependenciesAnalysisPass();
    const auto& it = findSendableDependenciesAnalysisPass->find(tagDecl);
    if (it == findSendableDependenciesAnalysisPass->end()) {
        std::cerr << "Warning! Skipping a tag we can't find: '";
        std::cerr << ASTHelpers::getAsString(tagDecl) << "'" << std::endl;
        return false;
    }
    FindSendableDependenciesAnalysisResult findSendableDependenciesAnalysisResult = it->second;
    
    bool result = true;
    for (const auto& dependency : findSendableDependenciesAnalysisResult.dependencies) {
        switch (dependency.kind) {
            case FindSendableDependenciesAnalysisResult::inheritance: // fallthrough
            case FindSendableDependenciesAnalysisResult::field: // fallthrough
                result &= _isSendable(dependency.type);
                
            case FindSendableDependenciesAnalysisResult::specialAvailable:
                return true;
                
            case FindSendableDependenciesAnalysisResult::specialImportedAsReference:
                return false;
                
            case FindSendableDependenciesAnalysisResult::specialConditional:
                result &= _isSendable(dependency.type);
        }
    }
    
    return result;
}

bool SendableAnalysisPass::_isSendable(const clang::Type *type) const {
    if (const clang::AdjustedType* adjustedType = clang::dyn_cast<clang::AdjustedType>(type)) {
        if (const clang::DecayedType* decayedType = clang::dyn_cast<clang::DecayedType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::ArrayType* arrayType = clang::dyn_cast<clang::ArrayType>(type)) {
        if (const clang::ConstantArrayType* constantArrayType = clang::dyn_cast<clang::ConstantArrayType>(type)) {
            if (const clang::ArrayParameterType* arrayParameterType = clang::dyn_cast<clang::ArrayParameterType>(type)) {
                type->dump();
                __builtin_trap();
            }
            return _isSendable(constantArrayType->getElementType().getTypePtr());
        }
        if (const clang::DependentSizedArrayType* dependentSizedArrayType = clang::dyn_cast<clang::DependentSizedArrayType>(type)) {
            
        }
        if (const clang::IncompleteArrayType* incompleteArrayType = clang::dyn_cast<clang::IncompleteArrayType>(type)) {
            return _isSendable(incompleteArrayType->getElementType().getTypePtr());
        }
        if (const clang::VariableArrayType* variableArrayType = clang::dyn_cast<clang::VariableArrayType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::AtomicType* atomicType = clang::dyn_cast<clang::AtomicType>(type)) {
        return _isSendable(atomicType->getValueType().getTypePtr());
    }
    if (const clang::AttributedType* attributedType = clang::dyn_cast<clang::AttributedType>(type)) {
        return _isSendable(attributedType->getEquivalentType().getTypePtr());
    }
    if (const clang::BTFTagAttributedType* btfTagAttributedType = clang::dyn_cast<clang::BTFTagAttributedType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::BitIntType* bitIntType = clang::dyn_cast<clang::BitIntType>(type)) {
        type->dump();
        __builtin_trap();
        return true;
    }
    if (const clang::BlockPointerType* blockPointerType = clang::dyn_cast<clang::BlockPointerType>(type)) {
#warning "Is this correct?"
        return false;
    }
    if (const clang::BoundsAttributedType* boundsAttributedType = clang::dyn_cast<clang::BoundsAttributedType>(type)) {
        if (const clang::CountAttributedType* countAttributedType = clang::dyn_cast<clang::CountAttributedType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::BuiltinType* builtinType = clang::dyn_cast<clang::BuiltinType>(type)) {
        if (builtinType->isFloatingPoint() || builtinType->isInteger()) {
            return true;
        }
        if (builtinType->getKind() == clang::BuiltinType::NullPtr) {
            return true;
        }
        
        if (builtinType->getKind() == clang::BuiltinType::ObjCId) {
            return false;
        }
        
        type->dump();
        __builtin_trap();
    }
    if (const clang::ComplexType* complexType = clang::dyn_cast<clang::ComplexType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::DecltypeType* decltypeType = clang::dyn_cast<clang::DecltypeType>(type)) {
        if (const clang::DependentDecltypeType* dependentDecltypeType = clang::dyn_cast<clang::DependentDecltypeType>(type)) {
        }
        return _isSendable(decltypeType->getUnderlyingType().getTypePtr());
    }
    if (const clang::DeducedType* deducedType = clang::dyn_cast<clang::DeducedType>(type)) {
        if (const clang::AutoType* autoType = clang::dyn_cast<clang::AutoType>(type)) {
            return _isSendable(deducedType->getDeducedType().getTypePtr());
        }
        if (const clang::DeducedTemplateSpecializationType* deducedTemplateSpecializationType = clang::dyn_cast<clang::DeducedTemplateSpecializationType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::DependentAddressSpaceType* dependentAddressSpaceType = clang::dyn_cast<clang::DependentAddressSpaceType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::DependentBitIntType* dependentBitIntType = clang::dyn_cast<clang::DependentBitIntType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::DependentSizedExtVectorType* dependentSizedExtVectorType = clang::dyn_cast<clang::DependentSizedExtVectorType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::DependentVectorType* dependentVectorType = clang::dyn_cast<clang::DependentVectorType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::FunctionType* functionType = clang::dyn_cast<clang::FunctionType>(type)) {
        if (const clang::FunctionNoProtoType* functionNoProtoType = clang::dyn_cast<clang::FunctionNoProtoType>(type)) {
            
        }
        if (const clang::FunctionProtoType* functionProtoType = clang::dyn_cast<clang::FunctionProtoType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::InjectedClassNameType* injectedClassNameType = clang::dyn_cast<clang::InjectedClassNameType>(type)) {
        type->dump();
        __builtin_trap();
    }
    // clang::LocInfoType omitted, because it can't be dynamically cast to. See documentation
    if (const clang::MacroQualifiedType* macroQualifiedType = clang::dyn_cast<clang::MacroQualifiedType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::MatrixType* matrixType = clang::dyn_cast<clang::MatrixType>(type)) {
        if (const clang::ConstantMatrixType* constantMatrixType = clang::dyn_cast<clang::ConstantMatrixType>(type)) {
            
        }
        if (const clang::DependentSizedMatrixType* dependentSizedMatrixType = clang::dyn_cast<clang::DependentSizedMatrixType>(type)) {
            
        }
        type->dump();
        __builtin_trap();
        return true;
    }
    if (const clang::MemberPointerType* memberPointerType = clang::dyn_cast<clang::MemberPointerType>(type)) {
        #warning "Is this correct?"
        return true;
    }
    if (const clang::ObjCObjectPointerType* objcObjectPointerType = clang::dyn_cast<clang::ObjCObjectPointerType>(type)) {
        return false;
    }
    if (const clang::ObjCObjectType* objcObjectType = clang::dyn_cast<clang::ObjCObjectType>(type)) {
        if (const clang::ObjCInterfaceType* objcInterfaceType = clang::dyn_cast<clang::ObjCInterfaceType>(type)) {
            
        }
        if (const clang::ObjCObjectTypeImpl* objcObjectTypeImpl = clang::dyn_cast<clang::ObjCObjectTypeImpl>(type)) {
            
        }
        if (objcObjectType->isObjCId()) {
            return false;
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::ObjCTypeParamType* objcTypeParamType = clang::dyn_cast<clang::ObjCTypeParamType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::PackExpansionType* packExpansionType = clang::dyn_cast<clang::PackExpansionType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::PackIndexingType* packIndexingType = clang::dyn_cast<clang::PackIndexingType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::ParenType* parenType = clang::dyn_cast<clang::ParenType>(type)) {
        type->dump();
        __builtin_trap();
        return _isSendable(parenType->getInnerType().getTypePtr());
    }
    if (const clang::PipeType* pipeType = clang::dyn_cast<clang::PipeType>(type)) {
        type->dump();
        __builtin_trap();
        return false;
    }
    if (const clang::PointerType* pointerType = clang::dyn_cast<clang::PointerType>(type)) {
        return false;
    }
    if (const clang::ReferenceType* referenceType = clang::dyn_cast<clang::ReferenceType>(type)) {
        if (const clang::LValueReferenceType* lvalueReferenceType = clang::dyn_cast<clang::LValueReferenceType>(type)) {
            
        }
        if (const clang::RValueReferenceType* rvalueReferenceType = clang::dyn_cast<clang::RValueReferenceType>(type)) {
            
        }
        return false;
    }
    if (const clang::SubstTemplateTypeParmPackType* substTemplateTypeParmPackType = clang::dyn_cast<clang::SubstTemplateTypeParmPackType>(type)) {
        type->dump();
        __builtin_trap();
        return _isSendable(substTemplateTypeParmPackType->desugar().getTypePtr());
    }
    if (const clang::SubstTemplateTypeParmType* substTemplateTypeParmType = clang::dyn_cast<clang::SubstTemplateTypeParmType>(type)) {
        return _isSendable(substTemplateTypeParmType->desugar().getTypePtr());
    }
    if (const clang::TagType* tagType = clang::dyn_cast<clang::TagType>(type)) {
        if (const clang::EnumType* enumType = clang::dyn_cast<clang::EnumType>(type)) {
            return _isSendable(enumType->getDecl());
        }
        if (const clang::RecordType* recordType = clang::dyn_cast<clang::RecordType>(type)) {
            return _isSendable(recordType->getDecl());
        }
        type->dump();
        __builtin_trap();
    }
    if (const clang::TemplateSpecializationType* templateSpecializationType = clang::dyn_cast<clang::TemplateSpecializationType>(type)) {
        return _isSendable(templateSpecializationType->desugar().getTypePtr());
    }
    if (const clang::TemplateTypeParmType* templateTypeParmType = clang::dyn_cast<clang::TemplateTypeParmType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::TypeOfExprType* typeOfExprType = clang::dyn_cast<clang::TypeOfExprType>(type)) {
        type->dump();
        __builtin_trap();
        return _isSendable(typeOfExprType->desugar().getTypePtr());
    }
    if (const clang::TypeOfType* typeOfType = clang::dyn_cast<clang::TypeOfType>(type)) {
        type->dump();
        __builtin_trap();
        return _isSendable(typeOfType->desugar().getTypePtr());
    }
    // TypeWithKeyword cannot be cast to, so handle its three subclasses without casting
    {
        if (const clang::DependentNameType* dependentNameType = clang::dyn_cast<clang::DependentNameType>(type)) {
            type->dump();
            __builtin_trap();
        }
        if (const clang::DependentTemplateSpecializationType* dependentTemplateSpecializationType = clang::dyn_cast<clang::DependentTemplateSpecializationType>(type)) {
            type->dump();
            __builtin_trap();
        }
        if (const clang::ElaboratedType* elaboratedType = clang::dyn_cast<clang::ElaboratedType>(type)) {
            return _isSendable(elaboratedType->getNamedType().getTypePtr());
        }
    }
    if (const clang::TypedefType* typedefType = clang::dyn_cast<clang::TypedefType>(type)) {
        return _isSendable(typedefType->desugar().getTypePtr());
    }
    if (const clang::UnaryTransformType* unaryTransformType = clang::dyn_cast<clang::UnaryTransformType>(type)) {
        if (const clang::DependentUnaryTransformType* dependentUnaryTransformType = clang::dyn_cast<clang::DependentUnaryTransformType>(type)) {
        }
        return _isSendable(unaryTransformType->getUnderlyingType().getTypePtr());
    }
    if (const clang::UnresolvedUsingType* unresolvingUsingType = clang::dyn_cast<clang::UnresolvedUsingType>(type)) {
        type->dump();
        __builtin_trap();
    }
    if (const clang::UsingType* usingType = clang::dyn_cast<clang::UsingType>(type)) {
        return _isSendable(usingType->getUnderlyingType().getTypePtr());
    }
    if (const clang::VectorType* vectorType = clang::dyn_cast<clang::VectorType>(type)) {
        if (const clang::ExtVectorType* extVectorType = clang::dyn_cast<clang::ExtVectorType>(type)) {
            return _isSendable(vectorType->getElementType().getTypePtr());
        }
        return _isSendable(vectorType->getElementType().getTypePtr());
    }
    
    
    
    
    
    
    std::cerr << "We missed a type in our switching over clang::Type subclasses" << std::endl;
    type->dump();
    __builtin_trap();
}

bool SendableAnalysisPass::_isSendable(const clang::TemplateArgument& templateArg) const {
    const clang::QualType elementType = templateArg.getAsType();
    return !elementType.isNull() && _isSendable(elementType.getTypePtr());
}

bool SendableAnalysisPass::comparesEqualWhileTesting(const SendableAnalysisResult& expected, const SendableAnalysisResult& actual) const {
    if (expected._kind != actual._kind) {
        return false;
    }
    
    std::set<int> actualIndices;
    for (const auto& x : expected._unavailableDependencies.dependencies) {
        bool foundMatch = false;
        for (int i = 0; i < actual._unavailableDependencies.dependencies.size(); i++) {
            if (actualIndices.contains(i)) {
                continue;
            }
            
            if (std::string(x) == std::string(actual._unavailableDependencies.dependencies[i])) {
                actualIndices.insert(i);
                foundMatch = true;
                break;
            }
        }
        
        if (!foundMatch) {
            return false;
        }
    }
    
    return true;
}
