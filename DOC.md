# ast-answerer

## Overview
`ast-answerer` is a tool for analyzing the source code of [OpenUSD](https://github.com/PixarAnimationStudios/OpenUSD) and automatically generating useful Swift bindings, such as protocol conformances, modulemaps, and wrappers for some limitations in Swift-Cxx interop.  
To use it, you need to have already built llvm's `clang` and `clang-tools-extra` products, and a no-python version of OpenUSD. (See `README.md` for instructions)

## Overall structure

### Driver
`Driver/Driver.h` is the entry point for the project. The `Driver` is a singleton global object that gets passed around to classes as needed. It is responsible for initializing and running the different phases of the project:

### File system preparation  
`Util/FileSystemInfo.h` contains the lowest level of file system manipulation. All hard-coded file system behavior should be contained in the `FileSystemInfo` struct. It is responsible for interpreting CMake `-D`/`#define`'d variables, creating the AstAnswererOutput directory, and working around include-file limitations in ClangTool. 
    
### CMake parsing  
`Util/CMakeParser.h` contains a minimal, feature-incomplete CMake parser, purpose-built for the way Pixar uses CMake in OpenUSD. It interprets CMake code within the `USD_FRAMEWORK_REPO_PATH` to find calls to `pxr_library` and get the source files and public headers used. This information is used to determine which files in what order should be given to clang to build into an AST, as well as generating the modulemap. 

### Clang AST building, serialization, and loading  
`Util/ClangToolHelper` contains class `ClangToolHelper`, which manages clang AST building, serialization, and loading. 
    
### AST analysis passes
AST analysis passes are coordinated by `ASTAnalysisRunner` (see "Runner" pattern below). Each analysis pass inherits from `ASTAnalysisPass`, which defines the common interface to all analysis passes. Analysis passes visit the clang AST using clang's `RecursiveASTVisitor`, and analyze `TagDecl`s to produce a `AnalysisResult` for some visited `TagDecl`s. (The `AnalysisResult` is defined by each AST analysis pass subclass.) 

AST analysis passes automatically serialize the result of their analysis to disk. This makes it easy to examine and query the analysis pass using e.g. `grep` and `sed` from the command line. It also speeds up program execution. If an analysis pass can deserialize a prior result from disk, it will do so. After visiting the AST or deserializing a prior result, each analysis pass will test its result using the test data in `resources`. 

### Code generation passes
"Code generation" includes Swift and C++ source code, as well as modulemap and API notes files. Code generation passes are coordinated by `CodeGenRunner` (see "Runner" pattern below"). Each code gen pass inherits from `CodeGenBase`, which defines the common interface to all code gen passes. Code gen passes use an analysis pass and generate source code for a particular language/file type. 

Code generation passes automatically write include lines in header files given C++ types used in code generation, deduplicates const qualifiers in templated type arguments, and sorts the C++ types used in code generation according to their order of occurence within OpenUSD. 

## "Runner" pattern
AST analysis and code generation are two complex phases that can both be broken down into a number of independent passes, thereby simplifying the architecture of the phases. Some passes may be dependent on other passes or require access to other singleton types. For both of these, this project uses the "Runner" pattern: A Runner type creates, owns, and runs multiple passes sequentially, and each pass inherits from a base type that defines the generic interface for the pass.  

For example, in `AnalysisPass/ASTAnalysisRunner.h`, class `ASTAnalysisRunner` owns many different `ASTAnalysisPass` subclasses, and initializes and runs them sequentially. By owning the different passes, `ASTAnalysisRunner` can give passes access to the result of previous passes (e.g. in Swift, a type cannot be `Hashable` unless it is also `Equatable`). 

Similarly, in `CodeGen/CodeGenRunner.h`, class `CodeGenRunner` owns many different `CodeGenBase` subclasses, and initializes and runs them sequentially. By owning the different passes, `CodeGenRunner` can give passes access to the result of previous passes access (e.g. in Swift 5.10 and earlier, a linker error occurs in Debug mode if a C++ type imported as a reference is extended to conform to two different protocols in two different files, which is worked around by setting up `_referenceTypeCodeGen` before other code gen passes). 

## Other types
- `Util/FileWriterHelper.h` contains struct `FileWriterHelper` that abstracts away common boilerplate for code generation by automatically generating a "prologue" (such as header guards in header files, header includes in .cpp/.mm files, and file information in most files), as well as writing multiple files with the same base name but different extensions. 

- `Util/Graph.h` contains templated class `DirectedGraph`, a minimal implementation of a directed graph. It is used for Sendable analysis of types, and is suited to that purpose, but shouldn't be used more widely. 

- `Util/TestDataLoader.h` contains class `TestDataLoader`, a helper class for loading testing data from test files in the `resources` directory. It is used by phases and sub-phases that automatically test their own output, such as `CMakeParser` and AST analysis passes. 

## Resources
`resources` contains various test files for different phases and passes of the project. While running, the program automatically checks its state against the test files and exits if tests fail. These typically contain the expected result of different kinds of AST analysis passes. 
