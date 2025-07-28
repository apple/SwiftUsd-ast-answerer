/*
//
//  ModulemapCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 5/22/24.
//

#include "CodeGen/ModulemapCodeGen.h"

struct ModulemapModule {
    std::string name;
    std::vector<std::filesystem::path> headers;
    std::vector<std::unique_ptr<ModulemapModule>> children;
    std::vector<std::string> dylibs;
    
    ModulemapModule(std::string name, std::vector<std::filesystem::path> headers, std::vector<std::unique_ptr<ModulemapModule>> children, std::vector<std::string> dylibs) :
    name(name),
    headers(headers),
    children(std::move(children)),
    dylibs(dylibs) {}
};

ModulemapCodeGen::ModulemapCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<ModulemapCodeGen>(codeGenRunner) {}

std::string ModulemapCodeGen::fileNamePrefix() const {
    return "module";
}

ModulemapCodeGen::Data ModulemapCodeGen::preprocess() const {
    return {};
}

std::unique_ptr<ModulemapModule> ModulemapCodeGen::getModulemapModule() const {
    const CMakeParser* cmakeParser = getAstAnalysisRunner().getDriver()->getCMakeParser();
    
#warning Hard coding non-usd dylibs
    std::vector<std::string> dylibs = {
        "boost_atomic", "boost_regex",
        "MaterialXCore", "MaterialXFormat", "MaterialXGenGlsl", "MaterialXGenMsl",
        "MaterialXGenShader", "MaterialXRender",
        "MaterialXRenderHw", "MaterialXRenderMsl",
        "osdCPU", "osdGPU",
        "tbb_debug", "tbb", "tbbmalloc_debug", "tbbmalloc_proxy_debug",
        "tbbmalloc_proxy", "tbbmalloc",
    };
    
    std::vector<std::string> usdDylibs;
    for (const auto& lib : cmakeParser->getNamesOfPxrLibraries()) {
        if (lib == "OpenUSD") { continue; }
        if (std::find(usdDylibs.begin(), usdDylibs.end(), lib) == usdDylibs.end()) {
            usdDylibs.push_back("usd_" + lib);
        }
    }
    std::sort(usdDylibs.begin(), usdDylibs.end());
    // Alphabetize to match the old modulemap structure for now
    dylibs.insert(dylibs.end(), usdDylibs.begin(), usdDylibs.end());
    
    dylibs.push_back("z");
    
    std::unique_ptr<ModulemapModule> result = std::make_unique<ModulemapModule>(std::string("pxr"),
                                                                                std::vector<std::filesystem::path>({"pxr/pxr.h"}),
                                                                                std::vector<std::unique_ptr<ModulemapModule>>(),
                                                                                dylibs);
    std::vector<ModulemapModule*> stack = {result.get()};
    
    for (const auto& lib : cmakeParser->getRelativePathsOfPxrLibraries()) {
        std::vector<std::string> libComponents;
        for (const auto& x : lib) {
            libComponents.push_back(x.string());
        }
        
        for (int i = 0; i < libComponents.size(); i++) {
            if (i < stack.size()) {
                if (libComponents[i] != stack[i]->name) {
                    // we aren't in stack[i]'s subtree, so we must be the start of a new subtree
                    stack.erase(stack.begin() + i, stack.end());
                    
                } else {
                    // pass, same name so we're in stack[i]'s the subtree
                }
                
            }
            
            if (i >= stack.size() && i + 1 < libComponents.size()) {
                // Add an intermediate node
                std::unique_ptr<ModulemapModule> intermediateNode = std::make_unique<ModulemapModule>(libComponents[i],
                                                                                                      std::vector<std::filesystem::path>(),
                                                                                                      std::vector<std::unique_ptr<ModulemapModule>>(),
                                                                                                      std::vector<std::string>());
                stack.back()->children.push_back(std::move(intermediateNode));
                stack.push_back(stack.back()->children.back().get());
            }
        }

        std::unique_ptr<ModulemapModule> leaf = std::make_unique<ModulemapModule>(lib.filename().string(),
                                                                                  cmakeParser->getPublicRelativeHeadersInLibrary(lib),
                                                                                  std::vector<std::unique_ptr<ModulemapModule>>(),
                                                                                  std::vector<std::string>());
        stack.back()->children.push_back(std::move(leaf));
        stack.push_back(stack.back()->children.back().get());
    }
    
    return result;
}

void ModulemapCodeGen::writeModulemapModule(const ModulemapModule& x, int indentation) {
    writeIndentedLine(indentation, "module " + x.name + " {");
    
    if (!x.headers.empty()) {
        for (const auto& header : x.headers) {
            if (header.string() == "pxr/imaging/hdSt/extCompGpuComputation.h" ||
                header.string() == "pxr/imaging/hdSt/extCompGpuComputationResource.h") {
                writeIndentedLine(indentation + 1, "// header \"" + header.string() + "\" // 24.05 HdSt_ResourceBinder #include fix");
                continue;
            }
            if (header.string() == "pxr/imaging/hdSt/glslfxShader.h") {
                writeIndentedLine(indentation + 1, "// header \"" + header.string() + "\" // 24.05 HdSt_MaterialNetworkShader #include fix");
                continue;
            }
            
            writeIndentedLine(indentation + 1, "header \"" + header.string() + "\"");
        }
        writeLine("");
    }
    
    if (!x.children.empty()) {
        for (const auto& child : x.children) {
            writeModulemapModule(*child.get(), indentation + 1);
        }
        writeLine("");
    }
    
    writeIndentedLine(indentation + 1, "export *");
    
    if (!x.dylibs.empty()) {
        writeLine("");
        writeIndentedLine(indentation + 1, "// note: this list of dylibs is taken from lib/.");
        writeIndentedLine(indentation + 1, "// linking against tbbmalloc_proxy_debug, usd_usdUtils, and tbbmalloc_proxy");
        writeIndentedLine(indentation + 1, "// can cause issues. but, modulemap link commands are treated as suggestions");
        writeIndentedLine(indentation + 1, "// by the linker, and dropped unless they're required to find a symbol.");
        writeIndentedLine(indentation + 1, "// so, if this doesn't cause problems, it's probably okay.");

        for (const auto& dylib : x.dylibs) {
            writeIndentedLine(indentation + 1, "link \"" + dylib + "\"");
        }
    }
    
    writeIndentedLine(indentation, "}");
    writeLine("");
}

void ModulemapCodeGen::writeModulemapFile() {
    std::unique_ptr<ModulemapModule> root = getModulemapModule();
    writeModulemapModule(*root.get(), 0);
}

void ModulemapCodeGen::writeIndentedLine(int x, const std::string& line) {
    std::string spacer = "";
    for (int i = 0; i < x; i++) {
        spacer += "  ";
    }
    writeLine(spacer + line);
}
*/
