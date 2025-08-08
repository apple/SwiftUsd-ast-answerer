//===----------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer project authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#ifndef CMakeParser_h
#define CMakeParser_h

#include "Driver/Driver.h"
#include <filesystem>
#include <map>

// This is not a good CMake parser, and it definitely has lots of holes.
// It's purpose built for the way Pixar uses CMake in OpenUSD, 
// has many missing features, and makes lots of assumptions
// that are wrong in the general case. 
class CMakeParser {
public:
    CMakeParser(const Driver* driver);
    
    std::vector<std::filesystem::path> getListOfSourceFiles() const;
    std::vector<std::filesystem::path> getListOfPublicHeaders() const;
    std::vector<std::string> getNamesOfPxrLibraries() const;
    std::vector<std::filesystem::path> getRelativePathsOfPxrLibraries() const;
    std::vector<std::filesystem::path> getPublicRelativeHeadersInLibrary(const std::filesystem::path& dir) const;
        
private:
    struct PxrLibrary;
    const Driver* _driver;
    std::vector<PxrLibrary> _pxrLibraries;
        
    void process(const std::filesystem::path& dir);
    void test();
    void serialize() const;
    
private:
    struct Tokenizer {
        struct Token;
        static std::vector<Token> tokenize(const std::string& s);
        
        struct Token {
            enum Kind {
                space, // '[ \t]+'
                newline, // '\n'
                identifier, // '[A-Za-z_][A-Za-z0-9_]*'
                lparen, // '('
                rparen, // ')'
                bracketOpen, // '[' '='* '['
                bracketClose, // ']' '='* ']'
                pound, // '#'
                dollar, // '$'
                lbrace, // '{'
                rbrace, // '}'
                bracketArgument, // '[''[' text ']'']'
                quotedArgument, // '"' text '"'
                unquotedArgument, // text
                lineComment // # text
            };
            
            Token(Kind kind, const std::string& text);
            Token(Kind kind, unsigned char ch);
            
            operator std::string() const;
            
            std::string text;
            Kind kind;
        };
        
        struct DFAState {
            DFAState();
            
            enum Kind {
                none, // nothing special
                parsingIdentifier,
                parsingBracketOpen,
                parsingBracketClose,
                withinBracketArgument,
                withinQuotedArgument,
                withinUnquotedArgument,
            };
            
            // Number as useful
            int n;
            bool justHadBackslash;
            bool withinLineComment;
            std::string untokenizedText;
            std::string partiallyParsedBracketClose;
            Kind kind;
        };
    };
    
    struct CommandInvocationBuilder {
        struct CommandInvocation;
        static std::vector<CommandInvocation> build(std::vector<Tokenizer::Token> tokens);
        
        struct CommandInvocation {
            struct Argument;
            CommandInvocation(const std::string& name, std::vector<Argument> arguments);
            
            struct Argument {
                std::string text;
                bool isUnquoted;
                bool isQuoted;
                bool isBracket;
                
                static Argument quoted(const std::string& text);
                static Argument unquoted(const std::string& text);
                static Argument bracket(const std::string& text);
                
            private:
                Argument(const std::string& text);
            };
            
            std::string lowercaseName;
            std::string _rawName;
            std::vector<Argument> arguments;
            
            operator std::string() const;
        };
        
        struct DFAState {
            DFAState();
            
            enum Kind {
                none,
                inLineComment,
                inBracketComment,
                startingCommandInvocation,
                withinCommandInvocation // after lparen, before rparen
            };
            
            bool justHadComment;
            std::string commandInvocationName;
            std::vector<CommandInvocation::Argument> commandInvocationArguments;
            int commandInvocationLparenCount;
                        
            std::vector<Kind> kindStack;
        };
    };
    
    struct BlockBuilder {
        struct Block;
        static Block buildBlock(const std::vector<CommandInvocationBuilder::CommandInvocation>& commands);
                
        struct Block {
            enum Kind {
                none, // not starting new control flow scope
                ifBlock,
                foreachBlock,
                whileBlock,
                macroBlock,
                functionBlock
            };
            
            Kind kind;
            std::vector<Block> blocks;
            std::optional<CommandInvocationBuilder::CommandInvocation> command;
            
            Block(Kind kind, const std::vector<Block>& blocks);
            Block(const CommandInvocationBuilder::CommandInvocation& command);
            std::string to_string(int indentation) const;
        };
        
    };
    
    struct Interpreter {
        Interpreter();
        
        std::map<std::string, std::string> variables;
        std::vector<std::string> addedSubdirectories;
        std::vector<std::vector<std::string>> pxrLibraries;
        
        std::string interpretBlock(const BlockBuilder::Block& block);
        std::string interpretCommand(const CommandInvocationBuilder::CommandInvocation& command);
        std::vector<std::string> variableExpandArguments(const std::vector<CommandInvocationBuilder::CommandInvocation::Argument>& arguments) const;
        std::vector<std::string> variableExpandIfNeeded(const std::string& s) const;
        std::string varLookup(const std::string& s) const;
        
        static std::string semicolonJoin(const std::vector<std::string>& l);
        static std::vector<std::string> semicolonSplit(const std::string& s);
        
        bool isTrueConstant(const std::string& s) const;
        bool isFalseConstant(const std::string& s) const;
        
        int findControlFlowPointAfter(int pos, const BlockBuilder::Block& inBlock) const;
        bool evaluateBooleanExpression(const std::vector<std::string>& exprs) const;
        std::vector<std::string> substituteBooleanValue(bool v, const std::vector<std::string>& exprs, int i, int j) const;
        bool evaluateSingleString(const std::string& x) const;
        bool evaluateOperation(const std::string& leftOp, const std::string& op, const std::string& rightOp) const;
        
        bool parseRealNumber(const std::string& s, double* f) const;
        std::vector<int> parseVersion(const std::string& s) const;
        
    };
    
    struct PxrLibrary {
        std::string libraryName;
        std::filesystem::path dir;
        
        // PUBLIC_HEADERS, PUBLIC_CLASSES
        std::vector<std::filesystem::path> publicHeaders;
        // PUBLIC_CLASSES, PRIVATE_CLASSES
        std::vector<std::filesystem::path> sourceFiles;
        std::map<std::filesystem::path, bool> isPublic;
        
        PxrLibrary(const std::filesystem::path& dir, const std::vector<std::string>& arguments);
    };
    
    friend std::ostream& operator<<(std::ostream& os, const Tokenizer::Token& obj);
    friend std::ostream& operator<<(std::ostream& os, const CommandInvocationBuilder::CommandInvocation& obj);
    friend std::ostream& operator<<(std::ostream& os, const CommandInvocationBuilder::CommandInvocation::Argument& obj);
    friend std::ostream& operator<<(std::ostream& os, const BlockBuilder::Block& obj);
};

std::ostream& operator<<(std::ostream& os, const CMakeParser::Tokenizer::Token& obj);
std::ostream& operator<<(std::ostream& os, const CMakeParser::CommandInvocationBuilder::CommandInvocation& obj);
std::ostream& operator<<(std::ostream& os, const CMakeParser::CommandInvocationBuilder::CommandInvocation::Argument& obj);
std::ostream& operator<<(std::ostream& os, const CMakeParser::BlockBuilder::Block& obj);

#endif /* CMakeParser_h */
