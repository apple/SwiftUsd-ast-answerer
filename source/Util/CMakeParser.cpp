//
//  CMakeParser.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/3/24.
//

#include "Util/CMakeParser.h"
#include "Util/TestDataLoader.h"
#include "Util/FileSystemInfo.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <set>

// IMPORTANT: There are a few workarounds for slight issues with the
// build system. These are marked with "Workaround:". If the build system
// needs more workarounds, please mark them "Workaround:" in a comment,
// so they are easy to find in the future.

#define CMAKEPARSER_INTERPRETER_DEBUGPRINT 0
#define CMAKEPARSER_INTERPRETER_DEBUGPRINT_VARLOOKUP_NOTFOUND 1
#define CMAKEPARSER_PXRLIBRARY_DEBUGPRINT 0
#define CMAKEPARSER_PXRLIBRARY_DEBUGPRINT_STARTING_PARSE 0
#define CMAKEPARSER_CMAKEDRIVER_DEBUGPRINT 0

CMakeParser::CMakeParser(const Driver* driver) :
_driver(driver) {
    std::cout << "CMakeParser starting..." << std::endl;
    process(driver->getFileSystemInfo()->usdSourceRepoPath);
    test();
    serialize();
}

std::vector<std::filesystem::path> CMakeParser::getListOfSourceFiles() const {
    // Important! We need to put all of the public headers before
    // any source files, because for class template specializations of stdlib types
    // like std::vector<pxr::TfToken>, we can use the point of instantiation
    // to determine whether or not the specialization is visible in Swift,
    // but that requires the point of instantiation to get to be in all the headers
    // before the source files
    
    
    std::vector<std::filesystem::path> result;
    for (const auto& lib : _pxrLibraries) {
        // Workaround: swiftUsd shouldn't be exposed to analysis or code gen
        if (lib.libraryName == "swiftUsd") {
            continue;
        }
        
        result.insert(result.end(), lib.publicHeaders.begin(), lib.publicHeaders.end());
    }
    for (const auto& lib : _pxrLibraries) {
        // Workaround: swiftUsd shouldn't be exposed to analysis or code gen
        if (lib.libraryName == "swiftUsd") {
            continue;
        }
        
        result.insert(result.end(), lib.sourceFiles.begin(), lib.sourceFiles.end());
    }
    
    return result;
}

std::vector<std::filesystem::path> CMakeParser::getListOfPublicHeaders() const {
    std::vector<std::filesystem::path> result;
    for (const auto& lib : _pxrLibraries) {
        // Workaround: swiftUsd shouldn't be exposed to analysis or code gen
        if (lib.libraryName == "swiftUsd") {
            continue;
        }
        
        result.insert(result.end(), lib.publicHeaders.begin(), lib.publicHeaders.end());
    }
    return result;
}

std::vector<std::string> CMakeParser::getNamesOfPxrLibraries() const {
    std::vector<std::string> result;
    for (const auto& lib : _pxrLibraries) {
        result.push_back(lib.libraryName);
    }
    return result;
}

std::vector<std::filesystem::path> CMakeParser::getRelativePathsOfPxrLibraries() const {
    std::vector<std::filesystem::path> result;
    for (const auto& lib : _pxrLibraries) {
        if (!lib.dir.string().starts_with(_driver->getFileSystemInfo()->usdSourceRepoPath.string())) {
            std::cerr << "Bad library dir " << lib.dir.string() << std::endl;
            __builtin_trap();
        }
        
        std::filesystem::path toPushback = lib.dir.string().substr(1 + _driver->getFileSystemInfo()->usdSourceRepoPath.string().size());
        if (std::find(result.begin(), result.end(), toPushback) == result.end()) {
            result.push_back(toPushback);
        }
    }
    return result;
}

std::vector<std::filesystem::path> CMakeParser::getPublicRelativeHeadersInLibrary(const std::filesystem::path& dir) const {
    for (const auto& lib : _pxrLibraries) {
        if (lib.dir != _driver->getFileSystemInfo()->usdSourceRepoPath / dir) {
            continue;
        }
        
        std::vector<std::filesystem::path> result;
        for (const auto& header : lib.publicHeaders) {
            result.push_back(header.string().substr(1 + _driver->getFileSystemInfo()->usdSourceRepoPath.string().size()));
        }
                
        return result;
    }
        
    std::cerr << "No pixar library matching " << dir.string() << std::endl;
    __builtin_trap();
}


void CMakeParser::serialize() const {
    std::string serializationFileName = "CMakeParser.txt";
    std::cout << "Serializing " << serializationFileName << std::endl;
    
    const FileSystemInfo* f = _driver->getFileSystemInfo();
    std::filesystem::path filePath = f->getSerializedAnalysisPath(serializationFileName);
    
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream stream(filePath);
    
    std::function<std::string(std::filesystem::path)> makeRelative = [f](std::filesystem::path p){
        std::string s = p.string();
        if (s.starts_with(f->usdSourceRepoPath.string())) {
            return s.substr(f->usdSourceRepoPath.string().size() + 1);
        } else if (s.starts_with(f->usdInstalledHeaderPath.string())) {
            return s.substr(f->usdInstalledHeaderPath.string().size() + 1);
        } else {
            std::cerr << "Bad path! " << p.string() << std::endl;
            __builtin_trap();
        }
    };
    
    for (const auto& lib : _pxrLibraries) {
        for (const auto& p : lib.publicHeaders) {
            stream << makeRelative(p) << "; " << "publicHeader;" << std::endl;
        }
        for (const auto& p : lib.sourceFiles) {
            stream << makeRelative(p) << "; " << "sourceFile;" << std::endl;
        }
    }
    
    stream.close();
}

void CMakeParser::process(const std::filesystem::path &dir) {
    std::filesystem::path cmakeLists = dir / "CMakeLists.txt";
    if (CMAKEPARSER_CMAKEDRIVER_DEBUGPRINT) {
        std::cout << "CMAKEDRIVER: PROCESSING " << dir.string() << std::endl;
    }
    
    if (!std::filesystem::exists(cmakeLists)) {
        std::cerr << "Can't process " << cmakeLists << std::endl;
        __builtin_trap();
    }
    std::stringstream ss;
    ss << std::ifstream(cmakeLists).rdbuf();
    std::string s = ss.str();
    
    std::vector<Tokenizer::Token> tokens = Tokenizer::tokenize(s);
    std::vector<CommandInvocationBuilder::CommandInvocation> commands = CommandInvocationBuilder::build(tokens);
    BlockBuilder::Block block = BlockBuilder::buildBlock(commands);
    Interpreter interpreter;
    interpreter.interpretBlock(block);
    for (const auto& p : interpreter.pxrLibraries) {
        _pxrLibraries.push_back(PxrLibrary(dir, p));
    }
    
    for (const auto& subDir : interpreter.addedSubdirectories) {
        process(dir / subDir);
    }
}

void CMakeParser::test() {
    std::cout << "Testing CMakeParser..." << std::endl;
    std::vector<std::vector<std::string>> lines = TestDataLoader::load(*(_driver->getFileSystemInfo()), "testCMakeParser.txt", TestDataLoader::PxrNsReplacement::dontReplace);
    for (const std::vector<std::string>& line : lines) {
        if (line.size() != 3) {
            std::cerr << "Unexpected line format: [";
            for (const auto& x : line) {
                std::cerr << x << ", ";
            }
            std::cerr << "]" << std::endl;
            __builtin_trap();
        }
        
        std::string libPath = line[0];
        std::string fileKind = line[1];
        std::string fileName = line[2];
        
        bool foundLibrary = false;
        for (const auto& p : _pxrLibraries) {
            if (p.dir != _driver->getFileSystemInfo()->usdSourceRepoPath / libPath) {
                continue;
            }
            foundLibrary = true;
            
            std::vector<std::filesystem::path> toSearch;
            if (fileKind == "publicHeader") {
                toSearch = p.publicHeaders;
            } else if (fileKind == "sourceFile") {
                toSearch = p.sourceFiles;
            } else {
                std::cerr << "Unexpected file kind: [";
                for (const auto& x : line) {
                    std::cerr << x << ", ";
                }
                std::cerr << "]" << std::endl;
                __builtin_trap();
            }
            
            bool foundCandidate = false;
            for (const auto& candidate : toSearch) {
                if (candidate == p.dir / fileName) {
                    foundCandidate = true;
                    break;
                }
            }
            if (!foundCandidate) {
                std::cerr << "No file name: [";
                for (const auto& x : line) {
                    std::cerr << x << ", ";
                }
                std::cerr << "]" << std::endl;
                
                std::cout << "Public headers: [";
                for (const auto& x : p.publicHeaders) {
                    std::cout << x.string() << ", ";
                }
                std::cout << "]" << std::endl;
                std::cout << "Source files: [";
                for (const auto& x : p.sourceFiles) {
                    std::cout << x.string() << ", ";
                }
                std::cout << "]" << std::endl;
                __builtin_trap();
            }
        }
        
        if (!foundLibrary) {
            std::cerr << "No pxr_library: [";
            for (const auto& x : line) {
                std::cerr << x << ", ";
            }
            std::cerr << "]" << std::endl;
            __builtin_trap();
        }
    }
    std::cout << "CMakeParser passed" << std::endl;
}

std::vector<CMakeParser::Tokenizer::Token> CMakeParser::Tokenizer::tokenize(const std::string& s) {
    std::vector<Token> result;
    DFAState dfaState;
    
    uint64_t i = 0;
    while (i < s.size()) {
        unsigned char ch = s[i];
        // By default, advance
        i += 1;
        
        switch (dfaState.kind) {
            case DFAState::none:
                if (ch == ' ' || ch == '\t')  {
                    result.push_back(Token(Token::space, ch));
                    break;
                    
                } else if (ch == '\n') {
                    if (dfaState.withinLineComment) {
                        result.push_back(Token(Token::lineComment, dfaState.untokenizedText));
                        dfaState.withinLineComment = false;
                        dfaState.untokenizedText = "";
                    }
                    
                    result.push_back(Token(Token::newline, ch));
                    break;
                    
                } else if (ch == '_' || ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z')) {
                    dfaState.untokenizedText = ch;
                    dfaState.kind = DFAState::parsingIdentifier;
                    break;
                    
                } else if (ch == '(') {
                    result.push_back(Token(Token::lparen, ch));
                    break;
                    
                } else if (ch == ')') {
                    result.push_back(Token(Token::rparen, ch));
                    break;
                    
                } else if (ch == '[') {
                    dfaState.untokenizedText = std::string(1, ch);
                    dfaState.kind = DFAState::parsingBracketOpen;
                    break;
                    
                } else if (ch == ']') {
                    if (dfaState.withinLineComment) {
                        dfaState.untokenizedText.push_back(ch);
                    } else {
                        std::cerr << "Bare ']'" << std::endl;
                        __builtin_trap();
                    }
                                        
                } else if (ch == '#') {
                    result.push_back(Token(Token::pound, ch));
                    dfaState.withinLineComment = true;
                    break;
                    
                } else if (ch == '$') {
                    result.push_back(Token(Token::dollar, ch));
                    break;
                    
                } else if (ch == '{') {
                    result.push_back(Token(Token::lbrace, ch));
                    break;
                    
                } else if (ch == '}') {
                    result.push_back(Token(Token::rbrace, ch));
                    break;
                    
                } else if (ch == '"') {
                    dfaState.untokenizedText = ch;
                    dfaState.kind = DFAState::withinQuotedArgument;
                    break;
                    
                } else {
                    i -= 1;
                    dfaState.untokenizedText = "";
                    dfaState.kind = DFAState::withinUnquotedArgument;
                    break;
                }
                
                std::cerr << "Uncaught character '" << std::string(1, ch) << "'" << std::endl;
                std::cerr << "State: None" << std::endl;
                __builtin_trap();
                
            case DFAState::parsingIdentifier:
                if (ch == '_' || ('A' <= ch && ch <= 'Z') ||
                    ('a' <= ch && ch <= 'z') || ('0' <= ch && ch <= '9')) {
                    dfaState.untokenizedText.push_back(ch);
                    break;
                    
                } else {
                    // Don't advance so we process this char in `none` state
                    i -= 1;
                    result.push_back(Token(Token::identifier, dfaState.untokenizedText));
                    dfaState.untokenizedText = "";
                    dfaState.kind = DFAState::none;
                    break;
                }
                
            case DFAState::parsingBracketOpen:
                if (ch == '=') {
                    dfaState.n += 1;
                    dfaState.untokenizedText.push_back(ch);
                    break;
                    
                } else if (ch == '[') {
                    dfaState.untokenizedText.push_back(ch);
                    result.push_back(Token(Token::bracketOpen, dfaState.untokenizedText));
                    dfaState.untokenizedText = "";
                    dfaState.kind = DFAState::withinBracketArgument;
                    break;
                    
                } else {
                    // Rollback into unquoted argument
                    i -= 1;
                    dfaState.n = 0;
                    dfaState.kind = DFAState::withinUnquotedArgument;
                    break;
                }
                                
            case DFAState::parsingBracketClose:
                if (ch == ']' && dfaState.n == 0) {
                    // Commit
                    dfaState.partiallyParsedBracketClose.push_back(ch);
                    result.push_back(Token(Token::bracketArgument, dfaState.untokenizedText));
                    result.push_back(Token(Token::bracketClose, dfaState.partiallyParsedBracketClose));
                    dfaState.untokenizedText = "";
                    dfaState.partiallyParsedBracketClose = "";
                    dfaState.kind = DFAState::none;
                    break;
                    
                } else if (ch == '=') {
                    // This might go negative, but that's okay,
                    // it'll be handled  by the rollback on ] or non-]= character
                    dfaState.n -= 1;
                    dfaState.partiallyParsedBracketClose.push_back(ch);
                    break;
                    
                } else {
                    // Rollback changes
                    dfaState.n += dfaState.partiallyParsedBracketClose.size() - 1;
                    dfaState.partiallyParsedBracketClose.push_back(ch);
                    dfaState.untokenizedText += dfaState.partiallyParsedBracketClose;
                    dfaState.partiallyParsedBracketClose = "";
                    dfaState.kind = DFAState::withinBracketArgument;
                    break;
                }
                
            case DFAState::withinBracketArgument:
                if (ch == ']') {
                    // Can't commit until we see ']' in parsingBracketClose
                    dfaState.partiallyParsedBracketClose.push_back(ch);
                    dfaState.kind = DFAState::parsingBracketClose;
                    break;
                    
                } else {
                    dfaState.untokenizedText.push_back(ch);
                    break;
                }
                
            case DFAState::withinQuotedArgument:
                if (dfaState.justHadBackslash) {
                    dfaState.justHadBackslash = false;
                    if (ch == '\n') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\n');
                        break;
                        
                    } else if (ch == 't') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\t');
                        break;
                        
                    } else if (ch == 'r') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\r');
                        break;
                        
                    } else if (ch == 'n') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\n');
                        break;
                        
                    } else {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back(ch);
                        break;
                    }
                }
                
                if (ch == '\\') {
                    dfaState.justHadBackslash = true;
                    dfaState.untokenizedText.push_back(ch);
                    break;
                    
                } else if (ch == '"') {
                    dfaState.untokenizedText.push_back(ch);
                    result.push_back(Token(Token::quotedArgument, dfaState.untokenizedText));
                    dfaState.untokenizedText = "";
                    dfaState.kind = DFAState::none;
                    break;
                    
                } else {
                    dfaState.untokenizedText.push_back(ch);
                    break;
                }
                
            case DFAState::withinUnquotedArgument:
                if (dfaState.justHadBackslash) {
                    dfaState.justHadBackslash = false;
                    if (ch == '\n') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\n');
                        break;
                        
                    } else if (ch == 't') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\t');
                        break;
                        
                    } else if (ch == 'r') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\r');
                        break;
                        
                    } else if (ch == 'n') {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back('\n');
                        break;
                        
                    } else {
                        dfaState.untokenizedText.pop_back();
                        dfaState.untokenizedText.push_back(ch);
                        break;
                    }
                }
                
                if (std::isspace(ch) || std::string("()#\"").find(ch) != std::string::npos) {
                    i -= 1;
                    result.push_back(Token(Token::unquotedArgument, dfaState.untokenizedText));
                    dfaState.untokenizedText = "";
                    dfaState.kind = DFAState::none;
                    break;
                    
                } else if (ch == '\\') {
                    dfaState.justHadBackslash = true;
                    dfaState.untokenizedText.push_back(ch);
                    break;
                    
                } else {
                    dfaState.untokenizedText.push_back(ch);
                    break;
                }
        }
    }
    
    return result;
}

CMakeParser::Tokenizer::Token::Token(CMakeParser::Tokenizer::Token::Kind kind, const std::string& text) 
: text(text),
kind(kind)
{
}

CMakeParser::Tokenizer::Token::Token(CMakeParser::Tokenizer::Token::Kind kind, unsigned char ch)
: text(std::string(1, ch)),
kind(kind)
{
}

CMakeParser::Tokenizer::Token::operator std::string() const {
    switch (kind) {
        case space: return "SPACE '" + text + "'";
        case newline: return "NEWLINE '" + text + "'";
        case identifier: return "IDENTIFIER '" + text + "'";
        case lparen: return "LPAREN '" + text + "'";
        case rparen: return "RPAREN '" + text + "'";
        case bracketOpen: return "BRACKETOPEN '" + text + "'";
        case bracketClose: return "BRACKETCLOSE '" + text + "'";
        case pound: return "POUND '" + text + "'";
        case dollar: return "DOLLAR '" + text + "'";
        case lbrace: return "LBRACE '" + text + "'";
        case rbrace: return "RBRACE '" + text + "'";
        case bracketArgument: return "BRACKETARGUMENT '" + text + "'";
        case quotedArgument: return "QUOTEDARGUMENT '" + text + "'";
        case unquotedArgument: return "UNQUOTEDARGUMENT '" + text + "'";
        case lineComment: return "LINECOMMENT '" + text + "'";
    }
}

std::ostream& operator<<(std::ostream& os, const CMakeParser::Tokenizer::Token& obj) {
    return os << std::string(obj);
}

CMakeParser::Tokenizer::DFAState::DFAState() :
    n(0),
    justHadBackslash(false),
    withinLineComment(false),
    untokenizedText(""),
    partiallyParsedBracketClose(""),
    kind(CMakeParser::Tokenizer::DFAState::none) {
}

std::vector<CMakeParser::CommandInvocationBuilder::CommandInvocation> CMakeParser::CommandInvocationBuilder::build(std::vector<CMakeParser::Tokenizer::Token> tokens) {
    std::vector<CommandInvocation> result;
    
    DFAState dfaState;
    
    uint64_t i = 0;
    while (i < tokens.size()) {
        Tokenizer::Token token = tokens[i];
        // Advance by default
        i += 1;
        
        switch (dfaState.kindStack.back()) {
            case DFAState::none:
                switch (token.kind) {
                    case Tokenizer::Token::space: break;
                    case Tokenizer::Token::newline: break;
                    case Tokenizer::Token::identifier:
                        dfaState.commandInvocationName = token.text;
                        dfaState.commandInvocationArguments = {};
                        dfaState.kindStack.push_back(DFAState::startingCommandInvocation);
                        break;
                    case Tokenizer::Token::lparen: // fallthrough
                    case Tokenizer::Token::rparen: // fallthrough
                    case Tokenizer::Token::bracketOpen: // fallthrough
                    case Tokenizer::Token::bracketClose:
                        std::cerr << "Illegal token 1 " << token << std::endl;
                        __builtin_trap();
                        
                    case Tokenizer::Token::pound:
                        dfaState.justHadComment = true;
                        dfaState.kindStack.push_back(DFAState::inLineComment);
                        break;
                    case Tokenizer::Token::dollar: // fallthrough
                    case Tokenizer::Token::lbrace: // fallthrough
                    case Tokenizer::Token::rbrace: // fallthrough
                    case Tokenizer::Token::bracketArgument: // fallthrough
                    case Tokenizer::Token::quotedArgument: // fallthrough
                    case Tokenizer::Token::unquotedArgument: // fallthrough
                    case Tokenizer::Token::lineComment:
                        std::cerr << "Illegal token 2 " << token << std::endl;
                        __builtin_trap();
                }
                break;
                
            case DFAState::inLineComment:
                switch (token.kind) {
                    case Tokenizer::Token::space: 
                        dfaState.justHadComment = false;
                        break;
                    case Tokenizer::Token::newline:
                        dfaState.justHadComment = false;
                        dfaState.kindStack.pop_back();
                        break;
                    case Tokenizer::Token::identifier: // fallthrough
                    case Tokenizer::Token::lparen: // fallthrough
                    case Tokenizer::Token::rparen:
                        dfaState.justHadComment = false;
                        break;
                    case Tokenizer::Token::bracketOpen:
                        if (dfaState.justHadComment) {
                            dfaState.kindStack.back() = DFAState::inBracketComment;
                            break;
                        } else {
                            dfaState.justHadComment = false;
                            break;
                        }
                    case Tokenizer::Token::bracketClose: // fallthrough
                    case Tokenizer::Token::pound: // fallthrough
                    case Tokenizer::Token::dollar: // fallthrough
                    case Tokenizer::Token::lbrace: // fallthrough
                    case Tokenizer::Token::rbrace: // fallthrough
                    case Tokenizer::Token::bracketArgument: // fallthrough
                    case Tokenizer::Token::quotedArgument: // fallthrough
                    case Tokenizer::Token::unquotedArgument: // fallthrough
                    case Tokenizer::Token::lineComment:
                        dfaState.justHadComment = false;
                        break;
                }
                break;
                
            case DFAState::inBracketComment:
                switch (token.kind) {
                    case Tokenizer::Token::space: // fallthrough
                    case Tokenizer::Token::newline: // fallthrough
                    case Tokenizer::Token::identifier: // fallthrough
                    case Tokenizer::Token::lparen: // fallthrough
                    case Tokenizer::Token::rparen: // fallthrough
                    case Tokenizer::Token::bracketOpen:
                        std::cerr << "Illegal token 3 " << token << std::endl;
                        __builtin_trap();
                    case Tokenizer::Token::bracketClose:
                        dfaState.kindStack.pop_back();
                        break;
                    case Tokenizer::Token::pound: // fallthrough
                    case Tokenizer::Token::dollar: // fallthrough
                    case Tokenizer::Token::lbrace: // fallthrough
                    case Tokenizer::Token::rbrace: 
                        std::cerr << "Illegal token 4 " << token << std::endl;
                        __builtin_trap();
                    case Tokenizer::Token::bracketArgument: break;
                    case Tokenizer::Token::quotedArgument: // fallthrough
                    case Tokenizer::Token::unquotedArgument: // fallthrough
                    case Tokenizer::Token::lineComment:
                        std::cerr << "Illegal token 5 " << token << std::endl;
                        __builtin_trap();
                }
                break;
                
            case DFAState::startingCommandInvocation:
                switch (token.kind) {
                    case Tokenizer::Token::space: break;
                    case Tokenizer::Token::newline: // fallthrough
                    case Tokenizer::Token::identifier:
                        std::cerr << "Illegal token 6 " << token << std::endl;
                        __builtin_trap();
                    case Tokenizer::Token::lparen:
                        dfaState.commandInvocationLparenCount = 0;
                        dfaState.kindStack.pop_back();
                        dfaState.kindStack.push_back(DFAState::withinCommandInvocation);
                        break;
                    case Tokenizer::Token::rparen: // fallthrough
                    case Tokenizer::Token::bracketOpen: // fallthrough
                    case Tokenizer::Token::bracketClose: // fallthrough
                    case Tokenizer::Token::pound: // fallthrough
                    case Tokenizer::Token::dollar: // fallthrough
                    case Tokenizer::Token::lbrace: // fallthrough
                    case Tokenizer::Token::rbrace: // fallthrough
                    case Tokenizer::Token::bracketArgument: // fallthrough
                    case Tokenizer::Token::quotedArgument: // fallthrough
                    case Tokenizer::Token::unquotedArgument: // fallthrough
                    case Tokenizer::Token::lineComment:
                        std::cerr << "Illegal token 7 " << token << std::endl;
                        __builtin_trap();
                }
                break;
                
            case DFAState::withinCommandInvocation:
                switch (token.kind) {
                    case Tokenizer::Token::space: // fallthrough
                    case Tokenizer::Token::newline:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::unquoted(" "));
                        break;
                    case Tokenizer::Token::identifier:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::unquoted(token.text));
                        break;
                    case Tokenizer::Token::lparen:
                        dfaState.commandInvocationLparenCount += 1;
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::bracket("("));
                        break;
                    case Tokenizer::Token::rparen:
                        dfaState.commandInvocationLparenCount -= 1;
                        if (dfaState.commandInvocationLparenCount < 0) {
                            result.push_back(CommandInvocation(dfaState.commandInvocationName, dfaState.commandInvocationArguments));
                            dfaState.commandInvocationName = "";
                            dfaState.commandInvocationArguments = {};
                            dfaState.kindStack.pop_back();
                        } else {
                            dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::bracket(")"));
                        }
                        break;
                    case Tokenizer::Token::bracketOpen: // fallthrough
                    case Tokenizer::Token::bracketClose:
                        // Guaranteed to be matched by the tokenizer
                        break;
                    case Tokenizer::Token::pound:
                        dfaState.kindStack.push_back(DFAState::inLineComment);
                        break;
                    case Tokenizer::Token::dollar: // fallthrough
                    case Tokenizer::Token::lbrace: // fallthrough
                    case Tokenizer::Token::rbrace:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::unquoted(token.text));
                        break;
                    case Tokenizer::Token::bracketArgument:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::bracket(token.text));
                        break;
                    case Tokenizer::Token::quotedArgument:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::quoted(token.text));
                        break;
                    case Tokenizer::Token::unquotedArgument:
                        dfaState.commandInvocationArguments.push_back(CommandInvocation::Argument::unquoted(token.text));
                        break;
                    case Tokenizer::Token::lineComment:
                        std::cerr << "Illegal token 9 " << token << std::endl;
                        __builtin_trap();
                }
                break;
        }
    }
    
    return result;
}

CMakeParser::CommandInvocationBuilder::CommandInvocation::CommandInvocation(const std::string& name, std::vector<Argument> arguments) :
_rawName(name),
arguments(arguments) {
    lowercaseName = _rawName;
    std::transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), [](unsigned char ch){ return std::tolower(ch); });
}

CMakeParser::CommandInvocationBuilder::DFAState::DFAState() :
    justHadComment(false),
    commandInvocationName(""),
    commandInvocationArguments({}),
    kindStack({DFAState::none}) {
}

CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument::Argument(const std::string &text) : 
text(text),
isUnquoted(false),
isQuoted(false),
isBracket(false)
{}

CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument::quoted(const std::string &text) {
    Argument x(text);
    x.isQuoted = true;
    return x;
}

CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument::unquoted(const std::string &text) {
    Argument x(text);
    x.isUnquoted = true;
    return x;
}

CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument::bracket(const std::string &text) {
    Argument x(text);
    x.isBracket = true;
    return x;
}

CMakeParser::CommandInvocationBuilder::CommandInvocation::operator std::string() const {
    std::stringstream ss;
    ss << "NAME: '" << _rawName << "', ARGS: ";
    for (const auto& arg : arguments) {
        ss << arg << ", ";
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument& obj) {
    
    
    if (obj.isUnquoted) {
        return os << "unquoted '" << obj.text << "'";
    } else if (obj.isQuoted) {
        return os << "quoted '" << obj.text << "'";
    } else if (obj.isBracket) {
        return os << "bracket '" << obj.text << "'";
    } else {
        std::cerr << "Illegal argument state" << std::endl;
        __builtin_trap();
    }
}


std::ostream& operator<<(std::ostream& os, const CMakeParser::CommandInvocationBuilder::CommandInvocation& obj) {
    return os << std::string(obj);
}

CMakeParser::BlockBuilder::Block CMakeParser::BlockBuilder::buildBlock(const std::vector<CommandInvocationBuilder::CommandInvocation>& commands) {
    std::vector<BlockBuilder::Block> blockStack = {Block(Block::none, {})};
    
    for (const auto& command : commands) {
        
        if (command.lowercaseName == "if") {
            blockStack.push_back(Block(Block::ifBlock, {Block(command)}));
            
        } else if (command.lowercaseName == "elseif") {
            if (blockStack.back().kind != Block::ifBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            
        } else if (command.lowercaseName == "else") {
            if (blockStack.back().kind != Block::ifBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            
        } else if (command.lowercaseName == "endif") {
            if (blockStack.back().kind != Block::ifBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            Block tmp = blockStack.back();
            blockStack.pop_back();
            blockStack.back().blocks.push_back(tmp);
            
        } else if (command.lowercaseName == "foreach") {
            blockStack.push_back(Block(Block::foreachBlock, {Block(command)}));
            
        } else if (command.lowercaseName == "endforeach") {
            if (blockStack.back().kind != Block::foreachBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            Block tmp = blockStack.back();
            blockStack.pop_back();
            blockStack.back().blocks.push_back(tmp);

        } else if (command.lowercaseName == "while") {
            blockStack.push_back(Block(Block::whileBlock, {Block(command)}));
            
        } else if (command.lowercaseName == "endwhile") {
            if (blockStack.back().kind != Block::whileBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            Block tmp = blockStack.back();
            blockStack.pop_back();
            blockStack.back().blocks.push_back(tmp);

        } else if (command.lowercaseName == "macro") {
            blockStack.push_back(Block(Block::macroBlock, {Block(command)}));
            
        } else if (command.lowercaseName == "endmacro") {
            if (blockStack.back().kind != Block::macroBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            Block tmp = blockStack.back();
            blockStack.pop_back();
            blockStack.back().blocks.push_back(tmp);

        } else if (command.lowercaseName == "function") {
            blockStack.push_back(Block(Block::functionBlock, {Block(command)}));
            
        } else if (command.lowercaseName == "endfunction") {
            if (blockStack.back().kind != Block::functionBlock) {
                std::cerr << "Illegal command invocation: " << command << std::endl;
                __builtin_trap();
            }
            blockStack.back().blocks.push_back({Block(command)});
            Block tmp = blockStack.back();
            blockStack.pop_back();
            blockStack.back().blocks.push_back(tmp);

        } else {
            blockStack.back().blocks.push_back(Block(command));
        }
    }
    
    if (blockStack.size() != 1) {
        std::cerr << "Unbalanced blocks at end" << std::endl;
        __builtin_trap();
    }
    
    return blockStack.back();
}

CMakeParser::BlockBuilder::Block::Block(CMakeParser::BlockBuilder::Block::Kind kind, const std::vector<CMakeParser::BlockBuilder::Block>& blocks)
    : kind(kind),
    blocks(blocks),
    command(std::nullopt)
{}


CMakeParser::BlockBuilder::Block::Block(const CMakeParser::CommandInvocationBuilder::CommandInvocation& command)
: kind(CMakeParser::BlockBuilder::Block::none),
  blocks({}),
  command(command)
{}

std::string CMakeParser::BlockBuilder::Block::to_string(int indentation) const {
    std::stringstream ss;
    for (int x = 0; x < indentation; x++) {
        ss << "  ";
    }
    switch (kind) {
        case Block::none:
            ss << "None ";
            break;
        case Block::ifBlock:
            ss << "If ";
            break;
        case Block::foreachBlock:
            ss << "Foreach ";
            break;
        case Block::whileBlock:
            ss << "While ";
            break;
        case Block::macroBlock:
            ss << "Macro ";
            break;
        case Block::functionBlock:
            ss << "Function ";
            break;
    }
    
    if (command) {
        ss << *command << " [";
    } else {
        ss << "null [";
    }
    
    if (blocks.size() != 0) {
        ss << std::endl;
    }
    for (const auto& child : blocks) {
        ss << child.to_string(indentation + 1) << ", " << std::endl;
    }
    if (blocks.size() != 0) {
        for (int x = 0; x < indentation; x++) {
            ss << "  ";
        }
    }
    ss << "]";
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const CMakeParser::BlockBuilder::Block& obj) {
    return os << obj.to_string(0);
}

CMakeParser::Interpreter::Interpreter() :
variables({
    // Compiler features
    {"CMAKE_Swift_COMPILER_VERSION", "6"},
    {"CMAKE_CXX_COMPILER_ID", "AppleClang"},
    {"APPLE", "1"}, // LINUX
    {"UNIX", "1"}, // LINUX
    
    
    // Usd feature flags
    {"PXR_BUILD_IMAGING", "yes"},
    {"PXR_BUILD_USD_IMAGING", "yes"},
    {"PXR_BUILD_GPU_SUPPORT", "1"},
    {"PXR_ENABLE_MATERIALX_SUPPORT", "yes"},
    {"PXR_ENABLE_GL_SUPPORT", "1"},
    {"PXR_ENABLE_METAL_SUPPORT", "1"}, // LINUX
    {"PXR_ENABLE_VULKAN_SUPPORT", "0"}, // LINUX
    {"PXR_ENABLE_OPENVDB_SUPPORT", "0"},
    {"PXR_ENABLE_PTEX_SUPPORT", "0"},
    {"PXR_APPLE_EMBEDDED", "1"}, // LINUX
    {"PXR_BUILD_USD_VALIDATION", "0"},
    
    // Usd plugins
    {"PXR_BUILD_ALEMBIC_PLUGIN", "0"},
    {"PXR_BUILD_DRACO_PLUGIN", "0"},
    {"PXR_BUILD_EMBREE_PLUGIN", "0"},
    {"PXR_BUILD_OPENCOLORIO_PLUGIN", "0"},
    {"PXR_BUILD_PRMAN_PLUGIN", "0"},
    
    // Usd tests
    {"PXR_BUILD_TESTS", "0"},
    {"PXR_BUILD_ANIMX_TESTS", "0"},
    {"PXR_BUILD_MAYAPY_TESTS", "0"},
    
    // Usd artifacts
    {"PXR_BUILD_DOCUMENTATION", "0"},
    
    // Usd misc
    {"_PXR_CXX_FLAGS", "0"},
    {"PXR_HEADLESS_TEST_MODE", "0"},
    {"PXR_VALIDATE_GENERATED_CODE", "0"},
    
    // CMake
    {"CMAKE_CXX_FLAGS", "0"},
    {"CMAKE_DL_LIBS", "0"},
    {"PROJECT_SOURCE_DIR", "0"},
            
    // Boost
    {"Boost_PYTHON_LIBRARY", "0"},
    {"Boost_INCLUDE_DIRS", "0"},
    
    // OpenEXR
    {"OPENEXR_INCLUDE_DIRS", "0"},
    {"OPENEXR_LIBRARIES", "0"},
    
    // OpenSubdiv
    {"OPENSUBDIV_INCLUDE_DIR", "0"},
    {"OPENSUBDIV_LIBRARIES", "0"},
    {"OPENSUBDIV_OSDCPU_LIBRARY", "0"},
    
    // Python
    {"PYTHON_EXECUTABLE", "0"},
    {"PYTHON_INCLUDE_DIRS", "0"},
    {"PYTHON_INCLUDE_PATH", "0"},
    {"PYTHON_LIBRARIES", "0"},

    // TBB
    {"TBB_tbb_LIBRARY", "0"},
    {"TBB_INCLUDE_DIRS", "0"},
    
    // Misc
    {"APPKIT_LIBRARY", "0"},
    {"M_LIB", "0"},
    {"OPENGL_gl_LIBRARY", "0"},
    {"RENDERMAN_VERSION_MAJOR", "0"},
    {"VULKAN_LIBS", "0"}, // LINUX
    {"WINLIBS", "0"},
    {"X11_LIBRARIES", "0"}, // LINUX
})
{}

std::string CMakeParser::Interpreter::interpretBlock(const CMakeParser::BlockBuilder::Block &block) {
    switch (block.kind) {
        case CMakeParser::BlockBuilder::Block::none:
            if (block.command) {
                return interpretCommand(*block.command);
            } else {
                for (const auto& subBlock : block.blocks) {
                    std::string blockResult = interpretBlock(subBlock);
                    if (blockResult != "") {
                        return blockResult;
                    }
                }
            }
            return "";
            
        case CMakeParser::BlockBuilder::Block::ifBlock:
        {
            int instructionIndex = 0;
            
            while (true) {
                BlockBuilder::Block predicateBlock = block.blocks[instructionIndex];
                
                bool shouldDoPredicate = false;
                
                if (predicateBlock.command->lowercaseName == "else") {
                    shouldDoPredicate = true;
                } else if (predicateBlock.command->lowercaseName == "endif") {
                    break; // while loop
                } else {
                    std::vector<CommandInvocationBuilder::CommandInvocation::Argument> predicateArguments = predicateBlock.command->arguments;
                    shouldDoPredicate = evaluateBooleanExpression(variableExpandArguments(predicateArguments));
                }
                
                int nextControlFlow = findControlFlowPointAfter(instructionIndex, block);
                if (shouldDoPredicate) {
                    for (int i = instructionIndex + 1; i < nextControlFlow; i++) {
                        std::string blockResult = interpretBlock(block.blocks[i]);
                        if (blockResult != "") {
                            return blockResult;
                        }
                    }
                    break; // while loop
                } else {
                    instructionIndex = nextControlFlow;
                }
            }
        }
            return "";
            
        case CMakeParser::BlockBuilder::Block::foreachBlock:
        {
            BlockBuilder::Block foreachBlock = block.blocks[0];
            if (foreachBlock.command->lowercaseName != "foreach") {
                std::cerr << "Illegal foreach block 0 " << block << std::endl;
                __builtin_trap();
            }
            if (block.blocks.back().command->lowercaseName != "endforeach") {
                std::cerr << "Illegal foreach block -1 " << block << std::endl;
                __builtin_trap();
            }
                        
            std::vector<std::string> splitExpr = variableExpandArguments(foreachBlock.command->arguments);
            if (splitExpr.size() < 1) {
                std::cerr << "Illegal foreach block 1 " << block << std::endl;
                __builtin_trap();
            }
            
            std::string loopVar = splitExpr[0];
            std::vector<std::string> itemsValue = splitExpr;
            itemsValue.erase(itemsValue.begin());
            
            std::string blockResult;
            for (const auto& item : itemsValue) {
                variables[loopVar] = item;
                
                for (int instructionIndex = 1; instructionIndex < block.blocks.size() - 1; instructionIndex += 1) {
                    blockResult = interpretBlock(block.blocks[instructionIndex]);
                    if (blockResult != "") {
                        break;
                    }
                }
                
                if (blockResult != "continue" && blockResult != "") {
                    return blockResult;
                }
            }
        }
            return "";
            
        case CMakeParser::BlockBuilder::Block::whileBlock:
            std::cout << "WARNING: Skipping while block " << *block.blocks[0].command << std::endl;
            return "";
            
        case CMakeParser::BlockBuilder::Block::macroBlock:
            std::cout << "WARNING: Skipping macro block " << *block.blocks[0].command << std::endl;
            return "";
            
        case CMakeParser::BlockBuilder::Block::functionBlock:
            std::cout << "WARNING: Skipping function block " << *block.blocks[0].command << std::endl;
            return "";
    }
}

std::string CMakeParser::Interpreter::interpretCommand(const CMakeParser::CommandInvocationBuilder::CommandInvocation &command) {
    
    std::vector<std::string> noopts = {
        "cmake_minimum_required", "project", "include",
        "pxr_toplevel_prologue", "pxr_toplevel_epilogue",
        "add_definitions", "pxr_create_apple_framework",
        "pxr_core_prologue", "pxr_core_epilogue",
        "export", "configure_file", "install",
        "_swift_make_symbol_graph_only", "_swift_change_symbol_graph_module",
        "pxr_build_test_shared_lib", "pxr_build_test", "pxr_register_test",
        "pxr_test_scripts", "pxr_install_test_dir", "add_custom_command", "pxr_create_test_module",
        "pxr_python_bin", "pxr_plugin", "find_library", "add_compile_options"
    };
    for (const auto& noopt : noopts) {
        if (command.lowercaseName == noopt) {
            if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
                std::cout << "noop " << command.lowercaseName << std::endl;
            }
            return "";
        }
    }
    
    std::vector<std::string> args = variableExpandArguments(command.arguments);
    
    if (command.lowercaseName == "return") {
        if (args.size() != 0) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        return "return";
    }
    
    if (command.lowercaseName == "continue") {
        if (args.size() != 0) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        return "continue";
    }
    
    if (command.lowercaseName == "break") {
        if (args.size() != 0) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        return "break";
    }
    
    if (command.lowercaseName == "add_subdirectory") {
        if (args.size() != 1) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        addedSubdirectories.insert(addedSubdirectories.end(), args.begin(), args.end());
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "add_subdirectory " << semicolonJoin(args) << std::endl;
        }
        return "";
    }
    
    if (command.lowercaseName == "set") {
        if (args.size() < 2) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        std::string varName = args.front();
        std::vector<std::string> restArgs = args;
        restArgs.erase(restArgs.begin());
        std::string value = semicolonJoin(restArgs);
        variables[varName] = value;
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "Set '" << varName << "' to '" << value << "'" << std::endl;
        }
        return "";
    }
    
    if (command.lowercaseName == "list") {
        if (args.size() < 2) {
            std::cerr << "Illegal arguments to command " << command << std::endl;
            __builtin_trap();
        }
        
        if (args[0] == "APPEND") {
            std::string varName = args[1];
            std::vector<std::string> restArguments = args;
            restArguments.erase(restArguments.begin());
            restArguments.erase(restArguments.begin());
            std::string value = semicolonJoin(restArguments);
            auto it = variables.find(varName);
            if (it == variables.end()) {
                variables[varName] = value;
            } else {
                variables[varName] = it->second + ";" + value;
            }
            if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
                std::cout << "list append '" << varName << "' to '" << variables.find(varName)->second << "'" << std::endl;
            }
            return "";
        }
        
        std::cout << "WARNING: todo list " << command.arguments[0].text << std::endl;
        return "";
    }
    
    if (command.lowercaseName == "message") {
        if (args.size() >= 1 && args.front() == "FATAL_ERROR") {
            std::cerr << "message fatal error: " << command << std::endl;
            __builtin_trap();
        } else {
            if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
                std::cout << "message: " << command << std::endl;
            }
        }
        return "";
    }
    
    if (command.lowercaseName == "pxr_library") {
        pxrLibraries.push_back(args);
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "pxr_library ";
            for (const auto& a : args) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }
        return "";
    }
    
    std::cout << "WARNING: unknown command: " << command << std::endl;
    return "";
}

std::vector<std::string> CMakeParser::Interpreter::variableExpandArguments(const std::vector<CommandInvocationBuilder::CommandInvocation::Argument> &arguments) const {
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "VariableExpandArguments: [";
        for (const auto& arg : arguments) {
            std::cout << arg << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    std::vector<std::string> result;
    
    std::string unquotedBuffer;
    for (const auto& argument : arguments) {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "  VariableExpandArguments element: " << argument << std::endl;
        }
        
        if (argument.isBracket) {
            if (unquotedBuffer != "") {
                std::vector<std::string> expanded = variableExpandIfNeeded(unquotedBuffer);
                unquotedBuffer = "";
                result.insert(result.end(), expanded.begin(), expanded.end());
            }
            result.push_back(argument.text);
            
        } else if (argument.isQuoted) {
            if (unquotedBuffer != "") {
                std::vector<std::string> expanded = variableExpandIfNeeded(unquotedBuffer);
                unquotedBuffer = "";
                result.insert(result.end(), expanded.begin(), expanded.end());
            }
            std::string toExpand = argument.text;
            toExpand = toExpand.substr(1, toExpand.size() - 2);
            std::vector<std::string> expanded = variableExpandIfNeeded(toExpand);
            result.insert(result.end(), expanded.begin(), expanded.end());

        } else if (argument.isUnquoted) {
            if (argument.text == " ") {
                if (unquotedBuffer != "") {
                    std::vector<std::string> expanded = variableExpandIfNeeded(unquotedBuffer);
                    unquotedBuffer = "";
                    result.insert(result.end(), expanded.begin(), expanded.end());
                }
            } else {
                unquotedBuffer += argument.text;
            }
        }
    }
    if (unquotedBuffer != "") {
        std::vector<std::string> expanded = variableExpandIfNeeded(unquotedBuffer);
        unquotedBuffer = "";
        result.insert(result.end(), expanded.begin(), expanded.end());
    }
    
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "VariableExpandArguments returning list [";
        for (const auto& x : result) {
            std::cout << "'" << x << "', ";
        }
        std::cout << "]" << std::endl;
    }
    
    return result;
}

std::vector<std::string> CMakeParser::Interpreter::variableExpandIfNeeded(const std::string& s) const {
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "VariableExpandIfNeeded '" << s << "'" << std::endl;
    }
    
    // Special case, doesn't require ${}
    if (s == "CMAKE_SIZEOF_VOID_P") {
        return {"8"};
    }
    
    std::vector<std::string> expansionStack = {""};
    uint64_t i = 0;
    while (i < s.size()) {
        unsigned char ch = s[i];
        // Advance by default
        i += 1;
        
//        std::cout << (i - 1) << ", '" << ch << "', '" << expansionStack.back() << "'" << std::endl;

        switch (ch) {
            case '$':
                // Don't have to worry about \$ because \ handles that
                if (i < s.size() && s[i] == '{') {
                    expansionStack.push_back("");
                    // Advance again because we handled { too
                    i += 1;
                }
                break;
                
            case '}':
                // Don't have to worry about \} because \ handles that
                if (expansionStack.size() > 1) {
                    std::string toLookup = expansionStack.back();
                    expansionStack.pop_back();
                    expansionStack.back() += varLookup(toLookup);
                }
                break;
                
            case '\\':
                // Don't have to worry about \\ because we handle that here,
                // by parity
                if (i < s.size()) {
                    expansionStack.back() += s[i];
                    // Advance again because we handled the next char
                    i += 1;
                } else {
                    expansionStack.back() += '\\';
                }
                break;
                
            case '{':
                // Don't have to worry about ${ because $ handles that.
                // So, fallthrough
                
            default:
                expansionStack.back() += ch;
                break;
        }
    }
    
    if (expansionStack.size() != 1) {
        std::cerr << "Couldn't variable expand " << s << std::endl;
        __builtin_trap();
    }
    
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "VariableExpandIfNeeded '" << s << "': '" << expansionStack.front() << "'" << std::endl;
    }

    return semicolonSplit(expansionStack.front());
}

std::string CMakeParser::Interpreter::varLookup(const std::string &s) const {
    const auto& it = variables.find(s);
    if (it == variables.end()) {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT_VARLOOKUP_NOTFOUND) {
            std::cout << "VARLOOKUP '" << s << "': ";
            std::cout << "'' (not found)" << std::endl;
        }
        return "";
    }
    
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "VARLOOKUP '" << s << "': ";
        std::cout << "'" << it->second << "'" << std::endl;
    }
    return it->second;
}

std::string CMakeParser::Interpreter::semicolonJoin(const std::vector<std::string>& l) {
    std::stringstream ss;
    for (uint64_t i = 0; i < l.size(); i++) {
        ss << l[i];
        if (i + 1 < l.size()) {
            ss << ";";
        }
    }
    return ss.str();
}

std::vector<std::string> CMakeParser::Interpreter::semicolonSplit(const std::string &s) {
    std::vector<std::string> result = {""};
    
    uint64_t i = 0;
    while (i < s.size()) {
        unsigned char ch = s[i];
        // Advance by default
        i += 1;
        
        switch (ch) {
            case '\\':
                // Don't have to worry about \\ because we handle that here,
                // by parity
                if (i < s.size()) {
                    result.back() += s[i];
                    // Advance again because we handled the next char
                    i += 1;
                } else {
                    result.back() += '\\';
                }
                break;
                
            case ';':
                // Don't have to worry about \; because \ handles that
                result.push_back("");
                break;
                
            default:
                result.back() += ch;
        }
    }
    
    return result;
}

bool CMakeParser::Interpreter::parseRealNumber(const std::string& s, double* f) const {
    std::istringstream iss(s);
    double localF;
    iss >> std::noskipws >> localF;
    *f = localF;
    
    return iss.eof() && !iss.fail();
}

std::vector<int> CMakeParser::Interpreter::parseVersion(const std::string& s) const {
    std::vector<std::string> parts = splitStringByRegex(s, std::regex("\\."));
    std::vector<int> result;
    for (int i = 0; i < 4; i++) {
        if (i >= parts.size()) {
            result.push_back(0);
            continue;
        }
        std::istringstream iss(parts[i]);
        int x;
        iss >> std::noskipws >> x;
        if (!iss.fail()) {
            result.push_back(x);
        }
        if (!iss.eof()) {
            break;
        }
    }
    for (int i = 0; i < result.size() - 4; i++) {
        result.push_back(0);
    }
    
    return result;
}


bool CMakeParser::Interpreter::isTrueConstant(const std::string &s) const {
    std::string lowercase = s;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char ch){
        return std::tolower(ch);
    });
        
    std::vector<std::string> exactMatches = {
        "1", "on", "yes", "true", "y"
    };
    for (const auto& m : exactMatches) {
        if (lowercase == m) {
            return true;
        }
    }
    
    double f;
    if (parseRealNumber(s, &f)) {
        return f != 0;
    }
    return false;
}

bool CMakeParser::Interpreter::isFalseConstant(const std::string &s) const {
    std::string lowercase = s;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char ch){
        return std::tolower(ch);
    });
    
    std::vector<std::string> exactMatches = {
        "0", "off", "no", "false", "n", "ignore", "notfound", ""
    };
    for (const auto& m : exactMatches) {
        if (lowercase == m) {
            return true;
        }
    }

    if (s.ends_with("-NOTFOUND")) {
        return true;
    }
    
    return false;
}

int CMakeParser::Interpreter::findControlFlowPointAfter(int pos, const BlockBuilder::Block& inBlock) const {
    std::vector<std::string> matches;
    
    switch (inBlock.kind) {
        case BlockBuilder::Block::none:
            std::cerr << "Can't find control flow point after none!" << std::endl;
            __builtin_trap();
        case BlockBuilder::Block::ifBlock:
            matches = {"elseif", "else", "endif"};
            break;
        case BlockBuilder::Block::foreachBlock:
            matches = {"endforeach"};
            break;
        case BlockBuilder::Block::whileBlock:
            matches = {"endwhile"};
            break;
        case BlockBuilder::Block::macroBlock:
            matches = {"endmacro"};
            break;
        case BlockBuilder::Block::functionBlock:
            matches = {"endfunction"};
            break;
    }
    
    for (int i = pos + 1; i < inBlock.blocks.size(); i++) {
        for (const auto& x : matches) {
            if (inBlock.blocks[i].command && inBlock.blocks[i].command->lowercaseName == x) {
                return i;
            }
        }
    }
    
    return -1;
}


bool CMakeParser::Interpreter::evaluateOperation(const std::string& leftOp, const std::string& op, const std::string& rightOp) const {
    
    if (op == "GREATER") {
        double l;
        double r;
        if (parseRealNumber(leftOp, &l) && parseRealNumber(rightOp, &r)) {
            return l < r;
        } else {
            return false;
        }

    } else if (op == "EQUAL") {
        double l;
        double r;
        if (parseRealNumber(leftOp, &l) && parseRealNumber(rightOp, &r)) {
            return l == r;
        } else {
            return false;
        }
        
    } else if (op == "STREQUAL") {
        return leftOp == rightOp;
        
    } else if (op == "VERSION_LESS") {
        std::vector<int> l = parseVersion(leftOp);
        std::vector<int> r = parseVersion(rightOp);
        for (int i = 0; i < 4; i++) {
            if (l[i] != r[i]) {
                return l[i] < r[i];
            }
        }
        return false;
        
    } else if (op == "NOT") {
        return !evaluateSingleString(rightOp);
        
    } else if (op == "AND") {
        // CMake explicitly doesn't do short-circuiting
        bool evaluatedL = evaluateSingleString(leftOp);
        bool evaluatedR = evaluateSingleString(rightOp);
        return evaluatedL && evaluatedR;
        
    } else if (op == "OR") {
        // CMake explicitly doesn't do short-circuiting
        bool evaluatedL = evaluateSingleString(leftOp);
        bool evaluatedR = evaluateSingleString(rightOp);
        return evaluatedL || evaluatedR;

    } else {
        std::cerr << "WARNING: Unsupported op '" << leftOp;
        std::cerr << "' '" << op << "' '" << rightOp << "'" << std::endl;
        return false;
    }
}

std::vector<std::string> CMakeParser::Interpreter::substituteBooleanValue(bool v, const std::vector<std::string>& exprs, int i, int j) const {
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "Substitute boolean value " << v << " [";
        for (const auto& x : exprs) {
            std::cout << x << ", ";
        }
        std::cout << "] " << i << " " << j << " => ";
    }
    
    std::string stringified = v ? "TRUE" : "FALSE";
    std::vector<std::string> result;
    result.insert(result.end(), exprs.begin(), exprs.begin() + i);
    result.push_back(stringified);
    result.insert(result.end(), exprs.begin() + j, exprs.end());
    
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "[";
        for (const auto& x : result) {
            std::cout << x << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    return result;
}

bool CMakeParser::Interpreter::evaluateBooleanExpression(const std::vector<std::string>& exprs) const {
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "Evaluate boolean expr [";
        for (const auto& x : exprs) {
            std::cout << x << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    // Parentheses
    // Unary tests like EXISTS, COMMAND, DEFINED
    // Binary tests like EQUAL, LESS, LESS_EQUAL
    // Unary NOT
    // Binary AND, OR, left-to-right, no short-circiting
    
    // Highest to lowest precedence. `true` indicates unary,
    // `false` indicates binary
    std::vector<std::pair<bool, std::vector<std::string>>> operatorClasses = {
        {true, {
            "COMMAND", "POLICY", "TARGET", "TEST", "DEFINED",
            "EXISTS", "IS_READABLE", "IS_WRITABLE", "IS_EXECUTABLE",
            "IS_DIRECTORY", "IS_SYMLINK", "IS_ABSOLUTE"
        }},
        {false, {
            "IN_LIST", "IS_NEWER_THAN", "MATCHES", "LESS", "GREATER", "EQUAL",
            "LESS_EQUAL", "GREATER_EQUAL", "STRLESS", "STRGREATER", "STREQUAL",
            "STRLESS_EQUAL", "STRGREATER_EQUAL",
            "VERSION_LESS", "VERSION_GREATER", "VERSION_EQUAL", "VERSION_LESS_THAN",
            "VERSION_GREATER_EQUAL", "PATH_EQUAL",
        }},
        {true, {"NOT"}},
        {false, {"AND", "OR"}},
    };
    
    
    // Look for parentheses
    int lparenCount = 0;
    int lparenFirstLocation = -1;
    for (int i = 0; i < exprs.size(); i++) {
        std::string curToken = exprs[i];
        if (curToken == "(") {
            if (lparenCount == 0) {
                lparenFirstLocation = i;
            }
            lparenCount += 1;
            
        } else if (curToken == ")") {
            lparenCount -= 1;
            // The first time the parentheses are balanced,
            // evaluate that subexpression, substitute into the original expression,
            // and recurse
            if (lparenCount == 0) {
                std::vector<std::string> toRecurse = std::vector<std::string>(exprs.begin() + lparenFirstLocation + 1, exprs.begin() + i);
                if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
                    std::cout << "Paren recurse: [";
                    for (const auto& x : toRecurse) {
                        std::cout << x << ", ";
                    }
                    std::cout << "]" << std::endl;
                }
                bool recurseResult = evaluateBooleanExpression(toRecurse);
                std::vector<std::string> newExprs = substituteBooleanValue(recurseResult, exprs, lparenFirstLocation, i + 1);
                return evaluateBooleanExpression(newExprs);
            }
        }
    }
    if (lparenFirstLocation != -1) {
        std::cerr << "Error! Unbalanced parentheses in boolean expression [";
        for (const auto& x : exprs) {
            std::cerr << x << ", ";
        }
        std::cerr << "]" << std::endl;
        __builtin_trap();
    }
    
    // Operators
    
    // For each class of operators...
    for (const auto& operatorClass : operatorClasses) {
        bool isUnary = operatorClass.first;
        const std::vector<std::string>& operators = operatorClass.second;
        
        // Move forwards through the expression...
        for (int i = 0; i < exprs.size(); i++) {
            // Looking for any operator that matches that part of the expression
            for (const auto& op : operators) {
                if (exprs[i] != op) {
                    continue;
                }
                // Found a potential match
                if (i + 1 >= exprs.size()) {
                    // Can't be, because this operator requires
                    // a right operand
                    continue;
                }
                if (!isUnary && i == 0) {
                    // Can't be, because this operator requires
                    // a left operand
                    continue;
                }
                
                // Definitely a match
                std::string leftOp = isUnary ? "" : exprs[i - 1];
                std::string rightOp = exprs[i + 1];
                if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
                    std::cout << "Op recurse: " << leftOp << " " << op << " " << rightOp << std::endl;
                }
                bool opResult = evaluateOperation(leftOp, op, rightOp);
                std::vector<std::string> newExprs = substituteBooleanValue(opResult, exprs, isUnary ? i : i - 1, i + 2);
                return evaluateBooleanExpression(newExprs);
            }
        }
    }
        
    if (exprs.size() != 1) {
        std::cerr << "Illegal boolean expression! [";
        for (const auto& x : exprs) {
            std::cerr << x << ", ";
        }
        std::cerr << "]" << std::endl;
        __builtin_trap();
    }
    
    return evaluateSingleString(exprs[0]);
}

bool CMakeParser::Interpreter::evaluateSingleString(const std::string& x) const {
    if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
        std::cout << "Evaluate single string '" << x << "': ";
    }
    // Constants
    if (isTrueConstant(x)) {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "TRUE" << std::endl;
        }
        return true;
    }
    if (isFalseConstant(x)) {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "FALSE" << std::endl;
        }
        return false;
    }
    
    // Variables, strings
    const auto& it = variables.find(x);
    if (it != variables.end() && !isFalseConstant(it->second)) {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "TRUE" << std::endl;
        }
        return true;
    } else {
        if (CMAKEPARSER_INTERPRETER_DEBUGPRINT) {
            std::cout << "FALSE" << std::endl;
        }
        return false;
    }
}



CMakeParser::PxrLibrary::PxrLibrary(const std::filesystem::path& dir, const std::vector<std::string>& _arguments)
: dir(dir)
{
    std::vector<std::string> arguments = _arguments;
    libraryName = arguments[0];
    
    if (CMAKEPARSER_PXRLIBRARY_DEBUGPRINT_STARTING_PARSE) {
        std::cout << "Parsing pxr_library " << libraryName << std::endl;
    }
    
    // Given that these were the arguments passed to pxr_library,
    // extract a subset of those arguments in a particular way.
    // pxr_library is defined at OpenUSD/cmake/macros/Public.cmake,
    // and that's where all this structure/parsing information is taken from

    // NAME positional argument
    arguments.erase(arguments.begin());
    std::vector<std::string> options = {"DISABLE_PRECOMPILED_HEADERS", "INCLUDE_SCHEMA_FILES"};
    std::vector<std::string> oneValueArgs = {"TYPE", "PRECOMPILED_HEADER_NAME", "METALLIB_NAME", "EMIT_DOCC_SYMBOL_GRAPH"};
    std::vector<std::string> multiValueArgs = {
        "PUBLIC_CLASSES", "PUBLIC_HEADERS", 
        "PRIVATE_CLASSES", "PRIVATE_HEADERS",
        "CPPFILES", "LIBRARIES", "INCLUDE_DIRS",
        "DOXYGEN_FILES", "RESOURCE_FILES",
        "PYTHON_PUBLIC_CLASSES", "PYTHON_PRIVATE_CLASSES",
        "PYTHON_PUBLIC_HEADERS", "PYTHON_PRIVATE_HEADERS",
        "PYTHON_CPPFILES", "PYMODULE_CPPFILES",
        "PYMODULE_FILES", "PYSIDE_UI_FILES",
        "SWIFT_FILES", "METAL_FILES", "SWIFTMODULE_DEPENDENCIES"
    };
    
    std::string lastKeywordArg = "";
    
    for (const auto& x : arguments) {
        if (x == "") {
            continue;
        }
        
        bool foundOption = false;
        for (const auto& option : options) {
            if (x == option) {
                foundOption = true;
                lastKeywordArg = "";
                break;
            }
        }
        if (foundOption) {
            if (x == "INCLUDE_SCHEMA_FILES") {
                std::filesystem::path to_parse = dir / "generatedSchema.classes.txt";
                if (!std::filesystem::exists(to_parse)) {
                    std::cerr << "Error! No file at " << to_parse << ", but there should be";
                    __builtin_trap();
                }
                
                std::ifstream infile(to_parse);
                std::string line;
                std::string fileType = "";
                while (std::getline(infile, line)) {
                    if (line.size() == 0) {
                        continue;
                    }
                    if (line == "# Public Classes" || line == "# Python Module Files" || line == "# Resource Files") {
                        fileType = line;
                        continue;
                    }
                    if (fileType == "# Public Classes") {
                        publicHeaders.push_back(dir / (line + ".h"));
                        sourceFiles.push_back(dir / (line + ".cpp"));
                    }
                }
            }
            continue;
        }
        
        bool foundOneValueArg = false;
        for (const auto& oneValueArg : oneValueArgs) {
            if (x == oneValueArg) {
                foundOneValueArg = true;
                lastKeywordArg = oneValueArg;
                break;
            }
        }
        if (foundOneValueArg) {
            continue;
        }
        
        bool foundMutliValueArg = false;
        for (const auto& multiValueArg : multiValueArgs) {
            if (x == multiValueArg) {
                foundMutliValueArg = true;
                lastKeywordArg = multiValueArg;
                break;
            }
        }
        if (foundMutliValueArg) {
            continue;
        }
        
        if (lastKeywordArg == "") {
            std::cerr << "Got bad argument " << x << std::endl;
            std::cerr << "[";
            for (const auto& arg : _arguments) {
                std::cerr << arg << ", ";
            }
            std::cerr << "]";
            __builtin_trap();
        }
        
        if (libraryName == "pegtl" && x != "pegtl.hpp") {
            // pegtl is a header-only library with a single entry point, pegtl.hpp.
            // including other headers out of order can cause obscure compiler errors. 
            continue;
        }
        
        if (lastKeywordArg == "PUBLIC_CLASSES") {
            publicHeaders.push_back(dir / (x + ".h"));
            sourceFiles.push_back(dir / (x + ".cpp"));
            
        } else if (lastKeywordArg == "PUBLIC_HEADERS") {
            publicHeaders.push_back(dir / x);
            
        } else if (lastKeywordArg == "PRIVATE_CLASSES") {
            sourceFiles.push_back(dir / (x + ".cpp"));
            
        } else if (lastKeywordArg == "CPPFILES") {
            sourceFiles.push_back(dir / x);
            
        } else {
            if (CMAKEPARSER_PXRLIBRARY_DEBUGPRINT) {
                std::cout << "(Ignoring '" << x << "' in section '" << lastKeywordArg << "')" << std::endl;
            }
        }
        
        for (const auto& oneValueArg : oneValueArgs) {
            if (lastKeywordArg == oneValueArg) {
                lastKeywordArg = "";
                break;
            }
        }
    } // for (const auto& x : arguments)

    for (const auto& x : publicHeaders) {
        isPublic[x] = true;
    }
    for (const auto& x : sourceFiles) {
        isPublic[x] = false;
    }
}
