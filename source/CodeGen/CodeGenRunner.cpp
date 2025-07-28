//
//  CodeGenRunner.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/2/24.
//

#include "CodeGen/CodeGenRunner.h"
#include "CodeGen/ReferenceTypeConformanceCodeGen.h"
#include "CodeGen/EquatableCodeGen.h"
//#include "CodeGen/OpenUSDSwiftModuleCodeGen.h"
//#include "CodeGen/ModulemapCodeGen.h"
#include "CodeGen/EnumsCodeGen.h"
#include "CodeGen/StaticTokensCodeGen.h"
#include "CodeGen/TfNoticeProtocolCodeGen.h"
#include "CodeGen/CustomStringConvertibleCodeGen.h"
#include "CodeGen/SdfValueTypeNamesMembersCodeGen.h"
#include "CodeGen/SchemaGetPrimCodeGen.h"
#include "CodeGen/HashableCodeGen.h"
#include "CodeGen/ComparableCodeGen.h"
#include "CodeGen/SendableCodeGen.h"
#include "CodeGen/APINotesCodeGen.h"

// MARK: Entry

CodeGenRunner::~CodeGenRunner() {}

CodeGenRunner::CodeGenRunner(const Driver* driver) :
_driver(driver) {
    
    _referenceTypeConformanceCodeGen = CodeGenFactory::makeCodeGen<ReferenceTypeConformanceCodeGen>(this);
    _equatableCodeGen = CodeGenFactory::makeCodeGen<EquatableCodeGen>(this);
//    _openUSDSwiftModuleCodeGen = CodeGenFactory::makeCodeGen<OpenUSDSwiftModuleCodeGen>(this);
//    _modulemapCodeGen = CodeGenFactory::makeCodeGen<ModulemapCodeGen>(this);
    _enumsCodeGen = CodeGenFactory::makeCodeGen<EnumsCodeGen>(this);
    _staticTokensCodeGen = CodeGenFactory::makeCodeGen<StaticTokensCodeGen>(this);
    _tfNoticeProtocolCodeGen = CodeGenFactory::makeCodeGen<TfNoticeProtocolCodeGen>(this);
    _customStringConvertibleCodeGen = CodeGenFactory::makeCodeGen<CustomStringConvertibleCodeGen>(this);
    _sdfValueTypeNamesMembersCodeGen = CodeGenFactory::makeCodeGen<SdfValueTypeNamesMembersCodeGen>(this);
    _schemaGetPrimCodeGen = CodeGenFactory::makeCodeGen<SchemaGetPrimCodeGen>(this);
    _hashableCodeGen = CodeGenFactory::makeCodeGen<HashableCodeGen>(this);
    _comparableCodeGen = CodeGenFactory::makeCodeGen<ComparableCodeGen>(this);
    _sendableCodeGen = CodeGenFactory::makeCodeGen<SendableCodeGen>(this);
    _apiNotesCodeGen = CodeGenFactory::makeCodeGen<APINotesCodeGen>(this);
}

const Driver* CodeGenRunner::getDriver() const {
    return _driver;
}

// MARK: Accessors
const FileSystemInfo& CodeGenRunner::getFileSystemInfo() const {
    return getASTAnalysisRunner().getFileSystemInfo();
}

const ASTAnalysisRunner& CodeGenRunner::getASTAnalysisRunner() const {
    return *_driver->getASTAnalysisRunner();
}
const EnumsCodeGen* CodeGenRunner::getEnumsCodeGen() const {
    return _enumsCodeGen.get();
}
