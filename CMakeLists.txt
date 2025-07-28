cmake_minimum_required(VERSION 3.27)
project(ast-answerer)

if (NOT DEFINED USD_SOURCE_REPO_PATH)
    message(FATAL_ERROR "USD_SOURCE_REPO_PATH is not set. Pass `-DUSD_SOURCE_REPO_PATH=/some/path` when invoking cmake.")
endif()

if (NOT DEFINED USD_INSTALL_NO_PYTHON_PATH)
    message(FATAL_ERROR "USD_INSTALL_NO_PYTHON_PATH is not set. Pass `-DUSD_INSTALL_NO_PYTHON_PATH=/some/path` when invoking cmake. (Path to non-python Usd)")
endif()

# First, let's set up some stuff for llvm and clang. 

set(LLVM_INSTALL_DIR "~/llvm-project/build")
set(CMAKE_CXX_STANDARD 20)

# Help CMake find clang, in these locations
set(CLANG_SEARCH_PATHS
  ${LLVM_INSTALL_DIR}
  ${LLVM_INSTALL_DIR}/lib/cmake
  ${LLVM_INSTALL_DIR}/lib/cmake/llvm
  ${LLVM_INSTALL_DIR}/lib/cmake/clang
  ${LLVM_INSTALL_DIR}/share/clang/cmake
  ${LLVM_INSTALL_DIR}/share/llvm/cmake
)
  
find_package(Clang
  REQUIRED
  CONFIG
  PATHS ${CLANG_SEARCH_PATHS}
  NO_DEFAULT_PATH
)

# CLANG_INCLUDE_DIRS and LLVM_INCLUDE_DIRS are set by
# `find_package(Clang ...)`. They let us `#include "clang/...h"`.
# Use `SYSTEM` so that `#include <clang/.h>` style includes don't
# give us clang's warnings
include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

# Now, llvm/clang is set up.
# Next, lets set up our Xcode project.
# First, the main executable

# Let us generate a scheme so we can set CLI arguments from CMake
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Make an executable from our sources.
set(SOURCES
    source/Driver/main.cpp
    source/Driver/Driver.h
    source/Driver/Driver.cpp

    source/Util/FileSystemInfo.h
    source/Util/FileSystemInfo.cpp
    source/Util/ClangToolHelper.cpp
    source/Util/ClangToolHelper.h
    source/Util/TestDataLoader.h
    source/Util/TestDataLoader.cpp
    source/Util/FileWriterHelper.h
    source/Util/FileWriterHelper.cpp
    source/Util/CMakeParser.h
    source/Util/CMakeParser.cpp
    source/Util/Graph.h
    source/Util/Graph.cpp

    source/AnalysisPass/ASTAnalysisRunner.cpp
    source/AnalysisPass/ASTAnalysisRunner.h
    source/AnalysisPass/ASTAnalysisPass.h
    source/AnalysisPass/ASTAnalysisPass.cpp

    source/AnalysisPass/FindNamedDeclsAnalysisPass.h
    source/AnalysisPass/FindNamedDeclsAnalysisPass.cpp
    source/AnalysisResult/FindNamedDeclsAnalysisResult.h
    source/AnalysisResult/FindNamedDeclsAnalysisResult.cpp

    source/AnalysisPass/ImportAnalysisPass.h
    source/AnalysisPass/ImportAnalysisPass.cpp
    source/AnalysisResult/ImportAnalysisResult.h
    source/AnalysisResult/ImportAnalysisResult.cpp

    source/AnalysisPass/PublicInheritanceAnalysisPass.h
    source/AnalysisPass/PublicInheritanceAnalysisPass.cpp
    source/AnalysisResult/PublicInheritanceAnalysisResult.h
    source/AnalysisResult/PublicInheritanceAnalysisResult.cpp

    source/AnalysisPass/BinaryOpProtocolAnalysisPassBase.h
    source/AnalysisPass/BinaryOpProtocolAnalysisPassBase.cpp
    source/AnalysisResult/BinaryOpProtocolAnalysisResult.h
    source/AnalysisResult/BinaryOpProtocolAnalysisResult.cpp

    source/AnalysisPass/EquatableAnalysisPass.h
    source/AnalysisPass/EquatableAnalysisPass.cpp
    
    source/AnalysisPass/HashableAnalysisPass.h
    source/AnalysisPass/HashableAnalysisPass.cpp
    source/AnalysisResult/HashableAnalysisResult.h
    source/AnalysisResult/HashableAnalysisResult.cpp

    source/AnalysisPass/ComparableAnalysisPass.h
    source/AnalysisPass/ComparableAnalysisPass.cpp

    source/AnalysisPass/CustomStringConvertibleAnalysisPass.h
    source/AnalysisPass/CustomStringConvertibleAnalysisPass.cpp
    source/AnalysisResult/CustomStringConvertibleAnalysisResult.h    
    source/AnalysisResult/CustomStringConvertibleAnalysisResult.cpp

    source/AnalysisPass/TypedefAnalysisPass.h
    source/AnalysisPass/TypedefAnalysisPass.cpp
    source/AnalysisResult/TypedefAnalysisResult.h    
    source/AnalysisResult/TypedefAnalysisResult.cpp

    source/AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.h
    source/AnalysisPass/SdfValueTypeNamesMembersAnalysisPass.cpp
    source/AnalysisResult/SdfValueTypeNamesMembersAnalysisResult.h
    source/AnalysisResult/SdfValueTypeNamesMembersAnalysisResult.cpp

    source/AnalysisPass/FindSchemasAnalysisPass.h
    source/AnalysisPass/FindSchemasAnalysisPass.cpp
    source/AnalysisResult/FindSchemasAnalysisResult.h
    source/AnalysisResult/FindSchemasAnalysisResult.cpp

    source/AnalysisPass/FindEnumsAnalysisPass.h
    source/AnalysisPass/FindEnumsAnalysisPass.cpp
    source/AnalysisResult/FindEnumsAnalysisResult.h
    source/AnalysisResult/FindEnumsAnalysisResult.cpp

    source/AnalysisPass/FindStaticTokensAnalysisPass.h
    source/AnalysisPass/FindStaticTokensAnalysisPass.cpp
    source/AnalysisResult/FindStaticTokensAnalysisResult.h
    source/AnalysisResult/FindStaticTokensAnalysisResult.cpp

    source/AnalysisPass/FindTfNoticeSubclassesAnalysisPass.h
    source/AnalysisPass/FindTfNoticeSubclassesAnalysisPass.cpp
    source/AnalysisResult/FindTfNoticeSubclassesAnalysisResult.h
    source/AnalysisResult/FindTfNoticeSubclassesAnalysisResult.cpp

    source/AnalysisPass/FindSendableDependenciesAnalysisPass.h
    source/AnalysisPass/FindSendableDependenciesAnalysisPass.cpp
    source/AnalysisResult/FindSendableDependenciesAnalysisResult.h
    source/AnalysisResult/FindSendableDependenciesAnalysisResult.cpp

    source/AnalysisPass/SendableAnalysisPass.h
    source/AnalysisPass/SendableAnalysisPass.cpp
    source/AnalysisResult/SendableAnalysisResult.h
    source/AnalysisResult/SendableAnalysisResult.cpp

    source/AnalysisPass/APINotesAnalysisPass.h
    source/AnalysisPass/APINotesAnalysisPass.cpp
    source/AnalysisResult/APINotesAnalysisResult.h
    source/AnalysisResult/APINotesAnalysisResult.cpp

    source/CodeGen/CodeGenRunner.h
    source/CodeGen/CodeGenRunner.cpp
    source/CodeGen/CodeGenBase.h
    source/CodeGen/CodeGenBase.cpp

    source/CodeGen/ReferenceTypeConformanceCodeGen.h
    source/CodeGen/ReferenceTypeConformanceCodeGen.cpp

    source/CodeGen/EquatableCodeGen.h
    source/CodeGen/EquatableCodeGen.cpp

    source/CodeGen/OpenUSDSwiftModuleCodeGen.h
    source/CodeGen/OpenUSDSwiftModuleCodeGen.cpp

    source/CodeGen/ModulemapCodeGen.h
    source/CodeGen/ModulemapCodeGen.cpp

    source/CodeGen/CustomStringConvertibleCodeGen.h
    source/CodeGen/CustomStringConvertibleCodeGen.cpp

    source/CodeGen/SdfValueTypeNamesMembersCodeGen.h
    source/CodeGen/SdfValueTypeNamesMembersCodeGen.cpp

    source/CodeGen/SchemaGetPrimCodeGen.h
    source/CodeGen/SchemaGetPrimCodeGen.cpp

    source/CodeGen/HashableCodeGen.h
    source/CodeGen/HashableCodeGen.cpp

    source/CodeGen/ComparableCodeGen.h
    source/CodeGen/ComparableCodeGen.cpp

    source/CodeGen/EnumsCodeGen.h
    source/CodeGen/EnumsCodeGen.cpp

    source/CodeGen/StaticTokensCodeGen.h
    source/CodeGen/StaticTokensCodeGen.cpp

    source/CodeGen/TfNoticeProtocolCodeGen.h
    source/CodeGen/TfNoticeProtocolCodeGen.cpp

    source/CodeGen/SendableCodeGen.h
    source/CodeGen/SendableCodeGen.cpp    

    source/CodeGen/APINotesCodeGenNodes.h
    source/CodeGen/APINotesCodeGenNodes.cpp
    source/CodeGen/APINotesCodeGen.h
    source/CodeGen/APINotesCodeGen.cpp
)
set(RESOURCES
    resources/test.cpp
    resources/testFindNamedDecls.txt
    resources/testImport.txt
    resources/testPublicInheritance.txt
    resources/testEquatable.txt
    resources/testHashable.txt
    resources/testComparable.txt
    resources/testCustomStringConvertible.txt
    resources/testTypedef.txt
    resources/testCMakeParser.txt
    resources/testSdfValueTypeNamesMembers.txt
    resources/testFindSchemas.txt
    resources/testFindEnums.txt
    resources/testFindStaticTokens.txt
    resources/testFindTfNoticeSubclasses.txt
    resources/testFindSendableDependencies.txt
    resources/testSendable.txt
    resources/testAPINotes.txt
)

# Add the resources as part of the executable, so we can do
# source_group to group them in the Xcode file navigator. 
add_executable(ast-answerer ${SOURCES} ${RESOURCES})

# We don't want to compile any of the resources, even test.cpp,
# so tell CMake they're all secretly header files
set_source_files_properties(${RESOURCES} PROPERTIES
    HEADER_FILE_ONLY true
)

source_group("Resources" FILES ${RESOURCES})
source_group("Util" REGULAR_EXPRESSION "source/Util/.*")
source_group("Driver" REGULAR_EXPRESSION "source/Driver/.*")
source_group("AnalysisPass" REGULAR_EXPRESSION "source/AnalysisPass/.*")
source_group("AnalysisResult" REGULAR_EXPRESSION "source/AnalysisResult/.*")
source_group("CodeGen" REGULAR_EXPRESSION "source/CodeGen/.*")
target_include_directories(ast-answerer PUBLIC "source")

# Pass -fno-rtti, because custom clang disables rtti by default
target_compile_options(ast-answerer
  PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -Wall>
)

# Figure out the PXR_NS string
file(READ "${USD_INSTALL_NO_PYTHON_PATH}/include/pxr/pxr.h" PXR_H)
string(REGEX MATCH "#define PXR_INTERNAL_NS ([^\n]+)" PXR_NS ${PXR_H})
set(PXR_NS ${CMAKE_MATCH_1})

# Figure out the USD_DOC_ATTRIBUTION string

execute_process(COMMAND git describe --exact-match --tags
                WORKING_DIRECTORY "${USD_SOURCE_REPO_PATH}"
                OUTPUT_VARIABLE USD_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(USD_DOC_ATTRIBUTION "https://github.com/PixarAnimationStudios/OpenUSD/blob/${USD_TAG}")

file(REAL_PATH "${LLVM_INSTALL_DIR}" LLVM_INSTALL_DIR EXPAND_TILDE)

# Wire up various #defines

target_compile_options(ast-answerer
  PUBLIC
  -DUSD_SOURCE_REPO_PATH="${USD_SOURCE_REPO_PATH}"
  -DUSD_INSTALL_NO_PYTHON_PATH="${USD_INSTALL_NO_PYTHON_PATH}"
  -DAST_ANSWERER_REPO_PATH="${CMAKE_SOURCE_DIR}"
  -DAST_ANSWERER_BUILD_PATH="${CMAKE_CURRENT_BINARY_DIR}"
  -DPXR_NS="${PXR_NS}"
  -DUSD_DOC_ATTRIBUTION="${USD_DOC_ATTRIBUTION}"
  -DLLVM_INSTALL_DIR="${LLVM_INSTALL_DIR}"
)

message(STATUS "ast-answerer will use these #defines:")
message(STATUS "  #define USD_SOURCE_REPO_PATH \"${USD_SOURCE_REPO_PATH}\"")
message(STATUS "  #define USD_INSTALL_NO_PYTHON_PATH \"${USD_INSTALL_NO_PYTHON_PATH}\"")
message(STATUS "  #define AST_ANSWERER_REPO_PATH \"${CMAKE_SOURCE_DIR}\"")
message(STATUS "  #define AST_ANSWERER_BUILD_PATH \"${CMAKE_CURRENT_BINARY_DIR}\"")
message(STATUS "  #define PXR_NS \"${PXR_NS}\"")
message(STATUS "  #define USD_DOC_ATTRIBUTION \"${USD_DOC_ATTRIBUTION}\"")
message(STATUS "  #define LLVM_INSTALL_DIR \"${LLVM_INSTALL_DIR}\"")


# Link against libclang-cpp.dylib
target_link_libraries(ast-answerer
  PRIVATE
  clang-cpp
)


# Set up a default command line argument
set_target_properties(ast-answerer PROPERTIES
  XCODE_SCHEME_ARGUMENTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/test.cpp"
  XCODE_SCHEME_ENVIRONMENT PATH=$ENV{PATH}
)
