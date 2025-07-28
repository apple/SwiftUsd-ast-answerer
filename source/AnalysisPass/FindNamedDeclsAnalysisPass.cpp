//
//  FindNamedDeclsAnalysisPass.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 4/24/24.
//

#include "AnalysisPass/FindNamedDeclsAnalysisPass.h"
#include "AnalysisPass/ASTAnalysisPass.h"
#include "AnalysisResult/FindNamedDeclsAnalysisResult.h"
#include "AnalysisPass/ASTAnalysisRunner.h"
#include <fstream>



FindNamedDeclsAnalysisPass::FindNamedDeclsAnalysisPass(ASTAnalysisRunner* astAnalysisRunner) :
ASTAnalysisPass<FindNamedDeclsAnalysisPass, FindNamedDeclsAnalysisResult>(astAnalysisRunner) {
    insert_or_assign(nullptr, FindNamedDeclsAnalysisResult());
}

std::string FindNamedDeclsAnalysisPass::serializationFileName() const {
    return "FindNamedDecls.txt";
}

std::string FindNamedDeclsAnalysisPass::testFileName() const {
    return "testFindNamedDecls.txt";
}

void FindNamedDeclsAnalysisPass::serialize() const {
    std::cout << "Serializing " << serializationFileName() << std::endl;
    
    const FileSystemInfo& f = getFileSystemInfo();
    std::filesystem::path filePath = f.getSerializedAnalysisPath(serializationFileName());
    
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream stream(filePath);
    
    stream << find(nullptr)->second << std::endl;
    
    stream.close();
}

bool FindNamedDeclsAnalysisPass::deserialize() {
    return false;
}

// We need to traverse the entire AST to find types like `std::vector<pxr::TfToken>`,
// for things like typedef processing
bool FindNamedDeclsAnalysisPass::shouldOnlyVisitDeclsFromUsd() const {
    return false;
}

bool FindNamedDeclsAnalysisPass::VisitNamedDecl(clang::NamedDecl* namedDecl) {
    // Important: Don't require that this tagDecl is from Usd.
    // The import pass has to handle types that are fully non-Usd,
    // like std::vector<std::string>, so for import to be able to deserialize,
    // we need to be very permissive in what types we find.
    
    if (namedDecl) {
        if (const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(namedDecl)) {
            if (tagDecl->getDefinition()) {
                namedDecl = tagDecl->getDefinition();
            }
        }
    }
    
    std::string typeName = ASTHelpers::getAsString(namedDecl);
    FindNamedDeclsAnalysisResult::NamedDeclMap& namedDeclMap = getNamedDeclMap();
    
    // Important: If we already had an entry for this typeName,
    // and it is a definition, then don't replace it,
    // because we might be replacing a definition with a declaration
    bool shouldReplace = true;
    
    const auto& existingIt = namedDeclMap.find(typeName);
    if (existingIt != namedDeclMap.end()) {
        if (const clang::TagDecl* existingTag = clang::dyn_cast<clang::TagDecl>(existingIt->second)) {
            shouldReplace = !existingTag->isThisDeclarationADefinition();
        }
        
        if (clang::dyn_cast<clang::TypedefNameDecl>(existingIt->second) && clang::dyn_cast<clang::TypedefNameDecl>(namedDecl)) {
            // Don't replace typedefs/using
            shouldReplace = false;
        }
    }
    if (shouldReplace) {
        namedDeclMap.insert_or_assign(typeName, namedDecl);
    }
    
    return true;
}

bool FindNamedDeclsAnalysisPass::VisitType(clang::Type* type) {
    if (type) {
        std::string typeName = ASTHelpers::getAsString(type);
        getTypeMap().insert_or_assign(typeName, type);
    }
    return true;
}

void FindNamedDeclsAnalysisPass::test() const {
    std::cout << "Testing " << serializationFileName() << std::endl;
    std::vector<std::string> expected = TestDataLoader::loadOneField(getFileSystemInfo(), testFileName(), TestDataLoader::PxrNsReplacement::replace);
    
    const auto& namedDeclMap = getNamedDeclMap();
    const auto& typeMap = getTypeMap();
    for (const std::string& line : expected) {
        const auto& it1 = namedDeclMap.find(line);
        const auto& it2 = typeMap.find(line);
        if (it1 == namedDeclMap.end() && it2 == typeMap.end()) {
            std::cerr << "No named decl or type matches " << line << std::endl;
            __builtin_trap();
        }
    }
    
    std::cout << serializationFileName() << " passed" << std::endl;
}

const clang::TagDecl* FindNamedDeclsAnalysisPass::findTagDecl(const std::string &typeName) const {
    const clang::NamedDecl* result = findNamedDecl(typeName);
    if (!result) {
        return nullptr;
    }
    return clang::dyn_cast<clang::TagDecl>(result);
}

const clang::NamedDecl* FindNamedDeclsAnalysisPass::findNamedDecl(const std::string& name) const {
    const auto& it = getNamedDeclMap().find(name);
    return it != getNamedDeclMap().end() ? it->second : nullptr;
}

const clang::Type* FindNamedDeclsAnalysisPass::findType(const std::string& name) const {
    const auto& it = getTypeMap().find(name);
    return it != getTypeMap().end() ? it->second : nullptr;
}

const clang::FunctionDecl* FindNamedDeclsAnalysisPass::findFunctionDecl(const std::string &signature) const {
    const clang::NamedDecl* result = findNamedDecl(signature);
    if (!result) {
        return nullptr;
    }
    return clang::dyn_cast<clang::FunctionDecl>(result);
}

const FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisPass::getNamedDeclMap() const {
    return find(nullptr)->second.getNamedDeclMap();
}
FindNamedDeclsAnalysisResult::NamedDeclMap& FindNamedDeclsAnalysisPass::getNamedDeclMap() {
    return find(nullptr)->second.getNamedDeclMap();
}

const FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisPass::getTypeMap() const {
    return find(nullptr)->second.getTypeMap();
}
FindNamedDeclsAnalysisResult::TypeMap& FindNamedDeclsAnalysisPass::getTypeMap() {
    return find(nullptr)->second.getTypeMap();
}
