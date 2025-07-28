//
//  SendableAnalysisResult.h
//  ast-answerer
//
//  Created by Maddy Adams on 6/14/24.
//

#ifndef SendableAnalysisResult_h
#define SendableAnalysisResult_h

#include <string>
#include <fstream>
#include <optional>
#include <vector>

#include "AnalysisResult/FindSendableDependenciesAnalysisResult.h"

class SendableAnalysisPass;

struct SendableAnalysisResult {
public:
    enum Kind {
        unknown,
        unavailable,
        available,
    };
    
    bool isAvailable() const;
    
private:
    FindSendableDependenciesAnalysisResult _unavailableDependencies;
    Kind _kind;
    
public:
    static std::string getAsString(Kind kind);
    explicit operator std::string() const;
    static std::vector<Kind> allCases();
    
    SendableAnalysisResult();
    SendableAnalysisResult(Kind kind);
    SendableAnalysisResult(Kind kind, FindSendableDependenciesAnalysisResult _unavailableDependencies);
    
    static std::optional<SendableAnalysisResult> deserialize(const std::string& data, const SendableAnalysisPass* astAnalysisPass);
    
    friend class SendableAnalysisPass;
};

std::ostream& operator <<(std::ostream& os, const SendableAnalysisResult& obj);

#endif /* SendableAnalysisResult_h */
