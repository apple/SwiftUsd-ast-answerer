//
//  CustomStringConvertibleAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/30/24.
//

#ifndef CustomStringConvertibleAnalysisResult_h
#define CustomStringConvertibleAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class CustomStringConvertibleAnalysisPass;

struct CustomStringConvertibleAnalysisResult {
public:
    enum Kind {
        unknown,
        noAnalysisBecauseBlockedByImport,
        noAnalysisBecauseNonCopyable,
        blockedByNoCandidate,
        available,
        availableEnum,
        availableUsdObjectSubclass,
        availableSdfSpecSubclass,
        availableSdfSpecHandleSubclass,
        availableUsdMetadataValueMap,
        availableUsdGeomXformOp,
    };
    
    bool isAvailable() const;

private:
    friend class CustomStringConvertibleAnalysisPass;
    friend class CustomStringConvertibleCodeGen;
    Kind _kind;

public:
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    
    CustomStringConvertibleAnalysisResult();
    CustomStringConvertibleAnalysisResult(Kind kind);
    static std::optional<CustomStringConvertibleAnalysisResult> deserialize(const std::string& data, const CustomStringConvertibleAnalysisPass* ASTAnalysisPass);
};
std::ostream& operator <<(std::ostream& os, const CustomStringConvertibleAnalysisResult& obj);

#endif /* CustomStringConvertibleAnalysisResult_h */
