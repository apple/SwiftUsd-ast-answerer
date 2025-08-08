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

#include "Driver/Driver.h"
#include <iostream>
#include <chrono>

int main(int argc, const char **argv) {
    const auto start{std::chrono::steady_clock::now()};
    
    {
        Driver driver(argc, argv);
    };
    
    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};
    std::cout << "Elapsed seconds: " << elapsed_seconds << std::endl;
    
    return 0;
}

/*
 Questions we want to answer:
   Find all types in public headers
   For a given type, will be be imported to Swift?
   For a given type, is it a class/struct or an enum?
   For a given class/struct, what is the inheritance tree above/below it?
   For a given type, is it POD?
 
   
 Things we could potentially automate:
       Creating the modulemap: Parse CMake files in Usd
   SWIFT_NAME copy with API notes and cleverness
       OpenUSD.swift
       AnyObject/Pointers
   -CaseIterable
   -OptionSet
       Comparable
       CustomStringConvertible
       Enums
       Equatable
   ExpressibleByLiteral
       Hashable
   Math
   *Sendable
   -StdVectorProtocol
   -Typedefs
   -VtArrayProtocol
   *operatorBool
   -Wrapped types
   *Bool_Init_Schema
       Overlay_GetPrim_Schema
       SdfValueTypeNames_Extensions
   *TokensExtensions
       Vector_Descriptions
   swiftUsdCppHeadersDocC.swift
 */



/*
 Sendable analysis:
 Paraphrasing from the docs (including source files that are newer than out of date documentation),
 if a type is Sendable, that means it its values can be shared across arbitrary concurrency contexts
 without introducing a risk of data races. Data races occur when shared mutable state is accessed simultaneously
 from different concurrency contexts, and potential data races arise when shared mutable state allows for unsynchronized
 access.
 
 
 A value can be shared in three ways:
 - Pass by copy, which invokes the copy constructor
 - Pass by move, which invokes the move constructor
 - Pass by borrow, which maps to passing a const&
 
 
 Assumption: Usd does not accidentally create shared mutable state. 
 For example, the copy constructor on a type like GfVec2f (a vector of two
 floats) does not accidentally leave the original instance and the new
 instance pointing at the same block of memory.
 This means that we don't have to do analysis of copy constructors and move constructors.
 Instead, it suffices to look at the fields in a type, and use that information to determine
 whether a type could access shared mutable state.
 
 Assumption: By default, all shared mutable state is not thread-safe.
 There may be some backing types (e.g. for TfToken and SdfPath) that are manually examined
 and may be considered thread-safe, but by default, any shared mutable state is treated as unsafe.
 
 Assumption: All raw pointers, smart pointers (Pixar and Stl), and references are not thread-safe,
 regardless of their const-ness. There may be some cases that are manually examined
 and may be considered thread-safe, but by default, these are treated as shared mutable state.
 
 
 If we represent pointers and references as different types from their underlying type,
 then we can form a graph where nodes are types and there are directed edges from types to
 the types of their fields. Due to special cases, the graph may have cycles.
 (std::vector<T> is Sendable iff T is Sendable, so type T with a field of type std::vector<T> forms a cycle)
 Thus, we can perform Sendable analysis as follows:
 1. Form a graph where nodes are types and edges are from types to their dependencies, where
    a) If T derives from U, T depends on U
    b) If T has a field of type U, T depends on U
    c) If T matches one of a number of special cases, the above conditions might be substituted
       with something else.
 2. Form the strongly connected components of the graph, to produce a DAG.
 3. Topologically sort the DAG, and iterate through it. For each strongly connected component,
    all the types within it are Sendable if all of its outgoing edges are Sendable, and non-Sendable
    if at least one of its dependencies is non-Sendable. (Non-Sendability is viral, but cannot
    be computed on edges within an SCC, so internal edges cannot classify a type as non-Sendable)
 
 Step 1 should be one analysis pass, that dumps types and their dependencies (and kind of dependency).
 Step 2 and 3 should be one analysis pass, that assigns Sendability, and lists why types are non-Sendable.
   */
