//
//  CodeGenRunner.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/2/24.
//

#ifndef CodeGenRunner_h
#define CodeGenRunner_h

#include "Driver/Driver.h"
#include "AnalysisPass/ASTAnalysisRunner.h"

class ReferenceTypeConformanceCodeGen;
class EquatableCodeGen;
//class OpenUSDSwiftModuleCodeGen;
//class ModulemapCodeGen;
class EnumsCodeGen;
class StaticTokensCodeGen;
class TfNoticeProtocolCodeGen;
class CustomStringConvertibleCodeGen;
class SdfValueTypeNamesMembersCodeGen;
class SchemaGetPrimCodeGen;
class HashableCodeGen;
class ComparableCodeGen;
class SendableCodeGen;
class APINotesCodeGen;

// Owns and coordinates running different code gen passes
class CodeGenRunner {
public:
    // MARK: Entry
    CodeGenRunner(const Driver* driver);
    ~CodeGenRunner();
    
public:
    // MARK: Accessors
    const Driver* getDriver() const;
    const FileSystemInfo& getFileSystemInfo() const;
    const ASTAnalysisRunner& getASTAnalysisRunner() const;
    const ReferenceTypeConformanceCodeGen* getReferenceTypeConformanceCodeGen() const;
    const EnumsCodeGen* getEnumsCodeGen() const;
    
private:
    // MARK: Fields
    const Driver* _driver;
    
    std::unique_ptr<ReferenceTypeConformanceCodeGen> _referenceTypeConformanceCodeGen;
    std::unique_ptr<EquatableCodeGen> _equatableCodeGen;
//    std::unique_ptr<OpenUSDSwiftModuleCodeGen> _openUSDSwiftModuleCodeGen;
//    std::unique_ptr<ModulemapCodeGen> _modulemapCodeGen;
    std::unique_ptr<EnumsCodeGen> _enumsCodeGen;
    std::unique_ptr<StaticTokensCodeGen> _staticTokensCodeGen;
    std::unique_ptr<TfNoticeProtocolCodeGen> _tfNoticeProtocolCodeGen;
    std::unique_ptr<CustomStringConvertibleCodeGen> _customStringConvertibleCodeGen;
    std::unique_ptr<SdfValueTypeNamesMembersCodeGen> _sdfValueTypeNamesMembersCodeGen;
    std::unique_ptr<SchemaGetPrimCodeGen> _schemaGetPrimCodeGen;
    std::unique_ptr<HashableCodeGen> _hashableCodeGen;
    std::unique_ptr<ComparableCodeGen> _comparableCodeGen;
    std::unique_ptr<SendableCodeGen> _sendableCodeGen;
    std::unique_ptr<APINotesCodeGen> _apiNotesCodeGen;
};


#endif /* CodeGenRunner_h */
