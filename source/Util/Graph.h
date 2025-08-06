// ===-------------------------------------------------------------------===//
// This source file is part of github.com/apple/SwiftUsd-ast-answerer
//
// Copyright © 2025 Apple Inc. and the SwiftUsd-ast-answerer authors. All Rights Reserved. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at: 
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.     
// 
// SPDX-License-Identifier: Apache-2.0
// ===-------------------------------------------------------------------===//

#ifndef ast_answerer_Graph_h
#define ast_answerer_Graph_h

// Sendable analysis needs to manipulate the graph of dependencies between types,
// and unfortunately that graph has cycles, so we can't treat it as a DAG and just
// use stack recursion. So, this file has a simple implementation of some graph data structures
// and algorithms, just for the purposes of making Sendable analysis work. This
// is not a feature-complete Graph library



// In phase 1, we build the graph by pulling types while deserializing,
// and either adding edges to their bases and fields or special casing.
// So, we're building up the graph one node at a time.

// In phase 2, we walk the graph to produce the strongly connected components.
// That's also building up the new graph, one strongly connected component at a time.

// So, we use an adjacency list implementation, which is fast at adding nodes
// but slow at removing nodes

#include <map>
#include <set>
#include <vector>
#include <algorithm>


template <typename T>
class DirectedGraph {
public:
    DirectedGraph() {}
    
    uint64_t nodeCount() { return _implData.size(); }
    
    bool isNode(const T& t) { return _toImplData.find(t) != _toImplData.end(); }
    
    void addNode(T t) {
        if (isNode(t)) {
            return;
        }
        uint64_t key = _implData.size();
        _implData[key] = {};
        _toImplData[t] = key;
        _fromImplData[key] = t;
    }
    
    void addEdge(const T& t, const T& u) {
        addNode(t);
        addNode(u);
        
        uint64_t tKey = _toImplData[t];
        uint64_t uKey = _toImplData[u];
        _implData[tKey].insert(uKey);
    }
    
    bool hasEdge(const T& t, const T& u) {
        if (!isNode(t) || !isNode(u)) {
            return false;
        }
        uint64_t tKey = _toImplData[t];
        uint64_t uKey = _toImplData[u];
        return _implData[tKey].contains(uKey);
    }
    
    std::set<T> neighbors(const T& t) {
        if (!isNode(t)) {
            return {};
        }
        uint64_t tKey = _toImplData[t];
        std::set<T> result;
        for (uint64_t uKey : _implData[tKey]) {
            result.insert(_fromImplData[uKey]);
        }
        return result;
    }
    
    std::vector<T> getAllNodes() {
        std::vector<T> result;
        for (const auto& it : _toImplData) {
            result.push_back(it.first);
        }
        return result;
    }
    
    void clear() {
        _implData = {};
        _toImplData = {};
        _fromImplData = {};
    }
    
    void findStronglyConnectedComponents(std::vector<std::unique_ptr<std::set<T>>>& outSCCs,
                                         std::map<T, std::set<T>*>& outToSCCMapping,
                                         DirectedGraph<std::set<T>*>& outDirectedGraph
                                         ) const {
        // Use Tarjan's algorithm for finding strongly connected components
        
        outSCCs.clear();
        outToSCCMapping.clear();
        outDirectedGraph.clear();
        
        std::map<uint64_t, uint64_t> startIndex; // The "time" when we started processing a given node
        std::map<uint64_t, uint64_t> lowIndex; // The lowest time reachable from a given node to a node on the stack
        std::vector<uint64_t> stack;
        std::set<uint64_t> isOnStack;
                
        std::function<void(uint64_t)> process = [&](uint64_t n){
            // Start bookkeeping for n
            uint64_t thisStartIndex = startIndex.size();
            startIndex[n] = thisStartIndex;
            lowIndex[n] = thisStartIndex;
            stack.push_back(n);
            isOnStack.insert(n);
            
            for (uint64_t m : _implData.find(n)->second) {
                // Depth-first search. Since there's an edge from n to m,
                // m will be on the stack when it finishes,
                // so min our low index
                if (!startIndex.contains(m)) {
                    process(m);
                    lowIndex[n] = std::min(lowIndex[n], lowIndex[m]);
                    
                } else if (isOnStack.contains(m)) {
                    lowIndex[n] = std::min(lowIndex[n], lowIndex[m]);
                }
            }
            
            if (lowIndex[n] == startIndex[n]) {
                // We're the start of a new SCC
                outSCCs.push_back(std::make_unique<std::set<T>>());
                uint64_t poppedNode;
                do {
                    // Pop down the stack until getting to ourselves,
                    // adding to the SCC
                    poppedNode = stack.back();
                    stack.pop_back();
                    isOnStack.erase(poppedNode);
                    
                    const T& userNode = _fromImplData.find(poppedNode)->second;
                    outSCCs.back()->insert(userNode);
                    outToSCCMapping[userNode] = outSCCs.back().get();
                    
                } while (poppedNode != n);
                
                outDirectedGraph.addNode(outSCCs.back().get());
            }
        };
        
        // Process all the nodes
        for (const auto& it : _implData) {
            if (!startIndex.contains(it.first)) {
                process(it.first);
            }
        }
        
        // Add external edges between the SCCs
        for (const auto& it : _implData) {
            uint64_t n = it.first;
            for (uint64_t m : it.second) {
                std::set<T>* nSCC = outToSCCMapping[_fromImplData.find(n)->second];
                std::set<T>* mSCC = outToSCCMapping[_fromImplData.find(m)->second];
                if (nSCC != mSCC) {
                    outDirectedGraph.addEdge(nSCC, mSCC);
                }
            }
        }
    }
    
private:
    // Nodes are uint64_t, and they have directed edges stored in the edge set
    std::map<uint64_t, std::set<uint64_t>> _implData;
    
    // The interface hides the uint64_t backing, so
    // we want to efficiently map between the backing
    // and the user's node type
    std::map<T, uint64_t> _toImplData;
    std::map<uint64_t, T> _fromImplData;
};

void testDirectedGraph();

#endif /* ast_answerer_Graph_h */
