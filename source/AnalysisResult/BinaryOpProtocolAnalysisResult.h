//
//  BinaryOpProtocolAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/4/24.
//

#ifndef BinaryOpProtocolAnalysisResult_h
#define BinaryOpProtocolAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

template <typename Derived>
class BinaryOpProtocolAnalysisPassBase;

struct BinaryOpProtocolAnalysisResult {
public:
    enum Kind {
        unknown,
        noAnalysisBecauseBlockedByImport,
        noAnalysisBecauseNonCopyable,
        unavailable,
        unavailableBlockedByEquatable,
        availableFoundBySwift,
        availableShouldBeFoundBySwiftButIsnt,
        availableImportedAsReference,
        availableClassTemplateSpecialization,
        availableFriendFunction,
        availableInlineMethodDefinedAfterDeclaration,
        availableDifferentArgumentTypes,
    };
    
    bool isAvailable() const;
    
    std::string witnessFirstType;
    std::string witnessSecondType;

private:
    template <typename Derived>
    friend class BinaryOpProtocolAnalysisPassBase;
    friend class EquatableCodeGen;
    friend class ComparableCodeGen;
    
    Kind _kind;
    
public:
    static std::string getAsString(Kind kind);
    explicit operator std::string() const;
    static std::vector<Kind> allCases();

    BinaryOpProtocolAnalysisResult();
    BinaryOpProtocolAnalysisResult(Kind kind);
    BinaryOpProtocolAnalysisResult(Kind kind, const std::string& witnessFirstType, const std::string& witnessSecondType);
    template <typename Derived>
    static std::optional<BinaryOpProtocolAnalysisResult> deserialize(const std::string& data, const BinaryOpProtocolAnalysisPassBase<Derived>*) {
        return deserializeImpl(data);
    }
    
    static std::optional<BinaryOpProtocolAnalysisResult> deserializeImpl(const std::string& data);

};
std::ostream& operator <<(std::ostream& os, const BinaryOpProtocolAnalysisResult& obj);


#endif /* BinaryOpProtocolAnalysisResult_h */
