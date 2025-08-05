//
//  APINotesCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 7/12/24.
//

#include "AnalysisPass/PublicInheritanceAnalysisPass.h"
#include "AnalysisPass/ImportAnalysisPass.h"
#include "CodeGen/APINotesCodeGen.h"
#include "APINotesCodeGenNodes.h"
#include "clang/AST/Comment.h"
#include "llvm/Support/raw_os_ostream.h"

std::string formFullyQualifiedExprString(const clang::Expr*);

APINotesCodeGen::APINotesCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<APINotesCodeGen>(codeGenRunner) {}

APINotesCodeGen::~APINotesCodeGen() = default;

std::string APINotesNode::indent(int indentation) const {
    std::stringstream ss;
    for (int i = 0; i < indentation; i++) {
        ss << "  ";
    }
    return ss.str();
}

std::string APINotesCodeGen::fileNamePrefix() const {
    return "_OpenUSD_SwiftBindingHelpers";
}

bool APINotesCodeGen::ReplacedMethod::operator<(const ReplacedMethod& other) const {
    if (ASTHelpers::DeclComparator()(method, other.method)) {
        return true;
    } else if (ASTHelpers::DeclComparator()(other.method, method)) {
        return false;
    } else {
        return ASTHelpers::DeclComparator()(owningType, other.owningType);
    }
}

std::vector<APINotesCodeGen::ReplacedMethod> APINotesCodeGen::getReplacedMethods(const APINotesNode* node) {
    const PublicInheritanceAnalysisPass* publicInheritance = getPublicInheritanceAnalysisPass();
    const ImportAnalysisPass* importPass = getImportAnalysisPass();
    
    std::vector<ReplacedMethod> result;
    node->walk([&result, &publicInheritance, &importPass](const APINotesNode* node){
        const MethodItem* method = node->dyn_cast_opt<MethodItem>();
        if (!method) { return; }
        for (const auto& it : method->renamedMethods) {
            switch (it.second.getKind()) {
                case APINotesAnalysisResult::Kind::importTagAsShared: // fallthrough
                case APINotesAnalysisResult::Kind::importTagAsImmortal: // fallthrough
                case APINotesAnalysisResult::Kind::importTagAsOwned: // fallthrough
                case APINotesAnalysisResult::Kind::makeFunctionUnavailable: // fallthrough
                case APINotesAnalysisResult::Kind::renameTfNoticeRegisterFunctionSpecialCase: // fallthrough
                case APINotesAnalysisResult::Kind::markTfRemnantAsUnavailableImmortalFrtSpecialCase: // fallthrough
                case APINotesAnalysisResult::Kind::renameSdfZipFileIteratorSpecialCase:
                    std::cerr << "Error! Bad APINotesAnalysisResult::Kind in renamed methods" << std::endl;
                    std::cerr << ASTHelpers::getAsString(it.first) << " " << it.second << std::endl;
                    __builtin_trap();
                    break;

                case APINotesAnalysisResult::Kind::replaceMutatingFunctionWithNonmutatingWrapper: // fallthrough
                case APINotesAnalysisResult::Kind::replaceConstRefFunctionWithCopyingWrapper: // fallthrough
                    // valid for renaming methods; pass
                    break;
            }
            
            // ImportAsMember only attaches to the target type, not types that inherit from the target.
            // So, we need to replace the method on available publicly-inheriting types
            const clang::CXXMethodDecl* cxxMethod = clang::dyn_cast<clang::CXXMethodDecl>(it.first);
            for (const clang::CXXRecordDecl* cxxRecordDecl : publicInheritance->getPublicSubtypes(cxxMethod->getParent())) {
                if (importPass->find(cxxRecordDecl)->second.isImportedSomehow()) {
                    result.push_back({cxxMethod, cxxRecordDecl, it.second});
                }
            }
        }
    });
    
    std::sort(result.begin(), result.end());
    return result;
}

APINotesCodeGen::Data APINotesCodeGen::preprocess() {
    const APINotesAnalysisPass* analysisPass = getAPINotesAnalysisPass();
    _root = std::make_unique<NamespaceItem>("_OpenUSD_SwiftBindingHelpers", nullptr);
    
    for (const auto& it : analysisPass->getData()) {
        if (const clang::TagDecl* tagDecl = clang::dyn_cast<clang::TagDecl>(it.first)) {
            // Tf_Remnant is usually hidden from code gen, but we need to allow
            // annotating Tf_Remnant as an unavailable immortal FRT to work around
            // rdar://151640018 (crash when Tf_Remnant is not marked as FRT)
            if (!hasTypeName<SwiftNameInSwift>(tagDecl) && ASTHelpers::getAsString(tagDecl) != "class " PXR_NS"::Tf_Remnant") {
                continue;
            }
        }
        _root->add(it.first, it.second);
    }
    
    // Tell the auto-include line generation about the tags that own
    // methods we need to stub in due to using nonswift availability
    // to get around SwiftName not supporting overloads
    Data preprocessResult;
    std::set<const clang::NamedDecl*> alreadyAdded;
    
    for (const auto& it : getReplacedMethods(_root.get())) {
        const clang::CXXRecordDecl* typeOwningMethod = it.owningType;
        if (!alreadyAdded.contains(typeOwningMethod)) {
            preprocessResult.push_back(typeOwningMethod);
            alreadyAdded.insert(typeOwningMethod);
        }
    }

    return preprocessResult;
}

void APINotesCodeGen::writeHeaderFile(const APINotesCodeGen::Data& data) {
    std::map<const clang::CXXRecordDecl*, std::string> importAsMemberTypedefs;
    for (const auto& it : getReplacedMethods(_root.get())) {
        writeReplaceMethod(it, true, importAsMemberTypedefs);
    }
}

void APINotesCodeGen::writeCppFile(const APINotesCodeGen::Data& data) {
    std::map<const clang::CXXRecordDecl*, std::string> importAsMemberTypedefs;
    
    for (const auto& it : getReplacedMethods(_root.get())) {
        writeReplaceMethod(it, false, importAsMemberTypedefs);
        
    }
}

void APINotesCodeGen::writeReplaceMethod(ReplacedMethod replacedMethod,
                                         bool isHeader,
                                         std::map<const clang::CXXRecordDecl *, std::string> &importAsMemberTypedefs) {
    const clang::CXXMethodDecl* method = replacedMethod.method;
    const clang::CXXRecordDecl* owningType = replacedMethod.owningType;
    
    // Important: We want scoped access to any number of typeNamePrinters,
    // for the return value and all the arguments. We need to make sure
    // we clear them in reverse order after writeLine(),
    // so that feature guarding gets properly nested
    std::vector<TypeNamePrinter> typeNamePrinters;
    
    // These are the elements we need to figure out before
    // we can actually write the replacement. (depending on isHeader,
    // not all of these will actually be used)
    
    bool isInstanceMethod; // if the original method is an instance method or a static method
    std::string documentation; // the documentation string on the original method
    std::string retType; // the return type of the method
    std::string replacementFName; // the name of the (free) function we're declaring/defining
    std::vector<std::string> argumentTypes; // the types of the arguments, properly formatted (e.g. adding `pxr::`)
    std::vector<std::string> argumentNames; // the external argument names in the original function declaration
    std::vector<std::string> defaultArguments; // the default arguments (if present) for each argument, properly formatted (e.g. adding `pxr::`)
    std::string importAsMemberTypedef; // the typedef to use in the SWIFT_NAME macro to ImportAsMember
    bool isFirstTypedefUse; // we only write the typedef the first time we use it
    bool copyArg0; // whether a copy should be made of the first (this/self) argument before calling the original function
    std::string originalCalledName; // the name of the original function to call (handles static vs instance)
    std::string originalFName; // the base name of the original function, i.e. unqualified
    

    { // isInstanceMethod
        isInstanceMethod = method->isInstance();
    }
    
    { // documentation
        clang::Preprocessor& preprocessor = getCodeGenRunner()->getDriver()->getClangToolHelper()->getASTUnits()[0]->getPreprocessor();
        clang::comments::FullComment* comment = method->getASTContext().getCommentForDecl(method, &preprocessor);
        const char* commentStart = method->getASTContext().getSourceManager().getCharacterData(comment->getBeginLoc());
        const char* commentEnd = method->getASTContext().getSourceManager().getCharacterData(comment->getEndLoc()) + 1;
        documentation = std::string(commentStart, static_cast<size_t>(commentEnd - commentStart));
        // trim leading whitespace on each line
        std::vector<std::string> docLines = {""};
        for (const auto& c : documentation) {
            if (c == '\n') {
                docLines.push_back("");
            } else {
                docLines.back().push_back(c);
            }
        }
        for (auto& l : docLines) {
            l.erase(l.begin(), std::find_if(l.begin(), l.end(), [](unsigned char ch){ return !std::isspace(ch); }));
        }
#warning todo: properly extracting the start of comments instead of using this workaround
        if (!docLines.empty() && !docLines[0].starts_with("//") && !docLines[0].starts_with("/*")) {
            if (docLines.size() == 1) {
                docLines[0] = "// " + docLines[0];
            } else {
                docLines[0] = "// " + docLines[0];
                if (docLines[1].starts_with("///")) {
                    docLines[0] = "/" + docLines[0];
                }
            }
        }
        
        documentation = "";
        for (const auto& l : docLines) {
            documentation += l + "\n";
        }
        
        // Attribution
        std::string attributionLinkPrefix = getCodeGenRunner()->getFileSystemInfo().usdDocumentationAttributionLinkPrefix;
        auto analysisPass = getCodeGenRunner()->getASTAnalysisRunner().getImportAnalysisPass();
        std::string attributionLinkSuffix = analysisPass->headerPathForUsdType(method);
        documentation += "// Original documentation from " + attributionLinkPrefix + attributionLinkSuffix + "\n";
    }
    
    { // retType
        clang::QualType retQualType = method->getReturnType();
        if (replacedMethod.analysisResult.getKind() == APINotesAnalysisResult::Kind::replaceConstRefFunctionWithCopyingWrapper) {
            retQualType = ASTHelpers::removingRefConst(retQualType);
        }
        typeNamePrinters.push_back(typeNamePrinter(retQualType));
        retType = getTypeName<CppNameInCpp>(typeNamePrinters.back());
    }
        
    { // replacementFName
        replacementFName = "__SwiftUsdImportAsMemberReplacement_" + mangleName(owningType) + "__" + method->getNameAsString();
    }
    
    { // argumentTypes, argumentNames, defaultArguments
        if (isInstanceMethod) {
            clang::QualType instanceType = owningType->getTypeForDecl()->getCanonicalTypeUnqualified();
            // Make the instance type be const&
            instanceType.addConst();
            instanceType = method->getASTContext().getLValueReferenceType(instanceType);
            
            typeNamePrinters.push_back(typeNamePrinter(instanceType));
            argumentTypes.push_back(getTypeName<CppNameInCpp>(typeNamePrinters.back()));
            argumentNames.push_back("_this");
            defaultArguments.push_back("");
        }
        
        for (const clang::ParmVarDecl* parm : method->parameters()) {
            typeNamePrinters.push_back(typeNamePrinter(parm->getType()));
            argumentTypes.push_back(getTypeName<CppNameInCpp>(typeNamePrinters.back()));
            argumentNames.push_back(parm->getNameAsString());
            if (parm->hasDefaultArg()) {
                const clang::Expr* defaultArg = parm->getDefaultArg();
                defaultArguments.push_back(formFullyQualifiedExprString(defaultArg));
                std::cout << defaultArguments.back() << std::endl;
            } else {
                defaultArguments.push_back("");
            }
        }
        
        if (argumentTypes.size() != argumentNames.size() || argumentTypes.size() != defaultArguments.size()) {
            std::cerr << "Error! Impossible. (Silly logic error in the last <40 lines?)" << std::endl;
            __builtin_trap();
        }
    }
        
    { // importAsMemberTypedef, isFirstTypedefUse
        if (isHeader) {
            if (importAsMemberTypedefs.contains(owningType)) {
                importAsMemberTypedef = importAsMemberTypedefs.find(owningType)->second;
                isFirstTypedefUse = false;
            } else {
                importAsMemberTypedef = "__SwiftUsdImportAsMemberTypedef_" + mangleName(owningType);
                importAsMemberTypedefs.insert({owningType, importAsMemberTypedef});
                isFirstTypedefUse = true;
            }
        }
    }
    
    { // copyArg0
        copyArg0 = replacedMethod.analysisResult.getKind() == APINotesAnalysisResult::Kind::replaceMutatingFunctionWithNonmutatingWrapper;
    }
    
    { // originalCalledName
        if (isInstanceMethod) {
            originalCalledName = method->getNameAsString();
        } else {
            originalCalledName = method->getQualifiedNameAsString();
            originalCalledName = std::regex_replace(originalCalledName, std::regex(PXR_NS), "pxr");
        }
    }
    
    { // originalFName
        originalFName = method->getNameAsString();
    }
    
    // Okay, we've set everything up. Now, let's actually write.
    // (Use a stringstream instead of calling writeLine() immediately
    // because we want to write parts of a line at a time.)
    std::stringstream ss;
    
    if (isHeader && isFirstTypedefUse) {
        // Typedef declaration: `typedef pixar-type single-token;`
        ss << "typedef ";
        typeNamePrinters.push_back(typeNamePrinter(owningType));
        ss << getTypeName<CppNameInCpp>(typeNamePrinters.back());
        ss << " " << importAsMemberTypedef << ";\n";
    }
    
    if (isHeader) {
        ss << documentation;
    }
    // Function declaration: `retType replacementFName(T0 arg0, T1 arg1 = default1)`
    ss << retType << " " << replacementFName << "(";
    for (int i = 0; i < argumentTypes.size(); i++) {
        // Headers get the original argument names, and may use default arguments
        // Non-headers get arg{i} as argument names, and never use default arguments
        if (isHeader) {
            ss << argumentTypes[i] << " " << argumentNames[i];
            if (!defaultArguments[i].empty()) {
                ss << " = " << defaultArguments[i];
            }
        } else {
            ss << argumentTypes[i] << " arg" << i;
        }
        if (i + 1 < argumentTypes.size()) {
            ss << ", ";
        }
    }
    ss << ")";
    
    if (isHeader) {
        // Headers get SWIFT_NAME(Typedef.method(...))
        ss << "\nSWIFT_NAME(" + importAsMemberTypedef + "." + originalFName + "(";
        for (int i = 0; i < argumentNames.size(); i++) {
            if (i == 0 && !method->isStatic()) {
                ss << "self:";
            } else {
                ss << "_:";
            }
        }
        // First `)` closes Typedef.method(...)`, second `)` closes `SWIFT_NAME(....)`
        ss << "));";
    } else {
        // Body: `{ return originalCalledName`
        ss << " {\n";
        int originalFCallArgumentStartIndex;
        
        if (copyArg0) {
            ss << "    auto arg0Copy = arg0;\n";
            ss << "    return arg0Copy." << originalCalledName;
            originalFCallArgumentStartIndex = 1;
        } else if (isInstanceMethod) {
            ss << "    return arg0." << originalCalledName;
            originalFCallArgumentStartIndex = 1;
        } else {
            ss << "    return " << originalCalledName;
            originalFCallArgumentStartIndex = 0;
        }
        
        // Call expression: `(args) }`
        ss << "(";
        for (int i = originalFCallArgumentStartIndex; i < argumentTypes.size(); i++) {
            ss << "arg" << i;
            if (i + 1 < argumentTypes.size()) {
                ss <<", ";
            }
        }
        ss << ");";
        ss << "\n}";
    }
    
    writeLine(ss.str());
    writeLine("");
    
    // Important: pop the typeNamePrinters in reverse order,
    // to make sure that feature guarding #if/#endifs are paired up
    // correctly
    while (!typeNamePrinters.empty()) {
        typeNamePrinters.pop_back();
    }
}


void APINotesCodeGen::writeAPINotesFile() {
    std::vector<std::string> lines;
    _root->write(lines, 0);
    
    // Deduplicate consecutive empty lines
    std::vector<std::string> linesCopy;
    for (const auto& l : lines) {
        if (!linesCopy.empty() && linesCopy.back().empty() && l.empty()) {
            continue;
        }
        linesCopy.push_back(l);
    }
    
    writeLines(linesCopy);
}













std::string formFullyQualifiedExprString(const clang::Expr* e) {
    e->dump();
    std::cout << std::endl;
    
    std::stringstream ss;
    
    
    if (const clang::CXXConstructExpr* cxxConstructExpr = clang::dyn_cast<clang::CXXConstructExpr>(e)) {
        const clang::CXXConstructorDecl* ctor = cxxConstructExpr->getConstructor();
        // Want the ctor's _parent's_ qualified name, not the ctors name,
        // because we want pxr::SdfLayerOffset, not pxr::SdfLayerOffset::SdfLayerOffset
        ss << ctor->getParent()->getQualifiedNameAsString() << "(";
        std::vector<const clang::Expr*> explicitArgs;
        for (const clang::Expr* arg : cxxConstructExpr->arguments()) {
            if (clang::dyn_cast<clang::CXXDefaultArgExpr>(arg)) {
                // Don't want to write it out, because the user didn't
            } else {
                explicitArgs.push_back(arg);
            }
        }
        
        for (int i = 0; i < explicitArgs.size(); i++) {
            ss << formFullyQualifiedExprString(explicitArgs[i]);
            if (i + 1 < explicitArgs.size()) {
                ss << ", ";
            }
        }
        ss << ")";
    } else if (const clang::DeclRefExpr* declRefExpr = clang::dyn_cast<clang::DeclRefExpr>(e)) {
        ss << declRefExpr->getDecl()->getQualifiedNameAsString();
    } else if (const clang::MaterializeTemporaryExpr* materializeTemporaryExpr = clang::dyn_cast<clang::MaterializeTemporaryExpr>(e)) {
        ss << formFullyQualifiedExprString(materializeTemporaryExpr->getSubExpr());
    } else if (const clang::CastExpr* castExpr = clang::dyn_cast<clang::CastExpr>(e)) {
        ss << formFullyQualifiedExprString(castExpr->getSubExpr());
    } else {
#warning todo: change this to __builtin_trap() once we have most of them down
        std::cout << "WARNING! Unhandled Expr type" << std::endl;
    }
    
    std::string result = ss.str();
    result = std::regex_replace(result, std::regex(PXR_NS), "pxr");
    std::cout << result << std::endl << std::endl;
    return result;
}
