//
//  APINotesAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 5/1/25.
//

#ifndef APINotesAnalysisResult_h
#define APINotesAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class APINotesAnalysisPass;

struct APINotesAnalysisResult {
public:
    enum class Kind {
        importTagAsShared,
        importTagAsImmortal,
        importTagAsOwned,
        makeFunctionUnavailable,
        replaceMutatingFunctionWithNonmutatingWrapper,
        replaceConstRefFunctionWithCopyingWrapper,
        renameTfNoticeRegisterFunctionSpecialCase,
        markTfRemnantAsUnavailableImmortalFrtSpecialCase,
        renameUsdZipFileIteratorSpecialCase
    };
        
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    APINotesAnalysisResult(Kind kind);
    
    static std::optional<APINotesAnalysisResult> deserialize(const std::string& data, const APINotesAnalysisPass* astAnalysisPass);
    
    Kind getKind() const;
    
private:
    Kind _kind;
};

std::ostream& operator <<(std::ostream& os, const APINotesAnalysisResult& obj);

#endif /* APINotesAnalysisResult_h */
