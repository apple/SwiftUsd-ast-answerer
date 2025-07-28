//
//  Graph.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 6/18/24.
//

#include "Util/Graph.h"
#include <random>
#include <algorithm>
#include <iostream>

#define N_TRIALS_PER_TEST 100

std::string randomStr(int len = 6) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<> dist(0, 25);
    std::string charSet = "abcdefghijklmnopqrstuvwxyz";
    
    std::string result;
    for (int i = 0; i < len; i++) {
        result += charSet[dist(rng)];
    }
    return result;
}

void assertTrue(bool value, const std::string& message) {
    if (!value) {
        std::cerr << "Assertion failed! " << message << std::endl;
        __builtin_trap();
    }
}

void assertFalse(bool value, const std::string& message) {
    if (value) {
        std::cerr << "Assertion failed! " << message << std::endl;
        __builtin_trap();
    }
}


#define BUILD_GRAPH \
    std::string n1 = randomStr() + "_1"; \
    std::string n2 = randomStr() + "_2"; \
    std::string n3 = randomStr() + "_3"; \
    std::string n4 = randomStr() + "_4"; \
    std::string n5 = randomStr() + "_5"; \
    std::string n6 = randomStr() + "_6"; \
    std::string n7 = randomStr() + "_7"; \
    std::string n8 = randomStr() + "_8"; \
    \
    std::vector<std::pair<std::string, std::string>> edges = { \
        {n1, n2}, \
        {n2, n3}, \
        {n3, n1}, \
        {n4, n2}, {n4, n3}, {n4, n5}, \
        {n5, n4}, {n5, n6}, \
        {n6, n3}, {n6, n7}, \
        {n7, n6}, \
        {n8, n5}, {n8, n7}, {n8, n8} \
    }; \
    \
    std::random_device rd; \
    std::mt19937 gen(rd()); \
    std::shuffle(edges.begin(), edges.end(), gen); \
    \
    DirectedGraph<std::string> graph; \
    for (const auto& p : edges) { \
        graph.addEdge(p.first, p.second); \
    } \

void testIsNode();
void testHasEdge();
void testNeighbors();
void testGetAllNodes();
void testFindStronglyConnectedComponents();
void testHandcraftedNodeInMultipleSCCs();

void testStart(const std::string& s) {
    std::cout << "Testing " + s + "..." << std::endl;
}

void testEnd(const std::string& s) {
    std::cout << s << " passed " << N_TRIALS_PER_TEST << " trials" << std::endl;
}

void testDirectedGraph() {
    testStart("directed graph");
    
    testIsNode();
    testHasEdge();
    testNeighbors();
    testGetAllNodes();
    testFindStronglyConnectedComponents();
    testHandcraftedNodeInMultipleSCCs();
    
    testEnd("directed graph");
}

#define ASSERT_IS_NODE(n) \
    assertTrue(graph.isNode(n), "isNode " #n)

#define ASSERT_IS_NOT_NODE(n) \
    assertFalse(graph.isNode(n), "isNode " #n)

void testIsNode() {
    testStart("isNode");
    for (int i = 0; i < N_TRIALS_PER_TEST; i++) {
        BUILD_GRAPH;
        
        ASSERT_IS_NODE(n1);
        ASSERT_IS_NODE(n2);
        ASSERT_IS_NODE(n3);
        ASSERT_IS_NODE(n4);
        ASSERT_IS_NODE(n5);
        ASSERT_IS_NODE(n6);
        ASSERT_IS_NODE(n7);
        ASSERT_IS_NODE(n8);
        
        std::string foo = randomStr() + "foo";
        ASSERT_IS_NOT_NODE(foo);
        std::string bar = randomStr() + "bar";
        ASSERT_IS_NOT_NODE(bar);
    }
    testEnd("isNode");
}

#define ASSERT_HAS_EDGE(n1, n2) \
    assertTrue(graph.hasEdge(n1, n2), "hasEdge " #n1 " " #n2)

#define ASSERT_NOT_HAS_EDGE(n1, n2) \
    assertFalse(graph.hasEdge(n1, n2), "hasEdge " #n1 " " #n2)


void testHasEdge() {
    testStart("hasEdge");
    
    for (int i = 0; i < N_TRIALS_PER_TEST; i++) {
        BUILD_GRAPH;
        
        ASSERT_NOT_HAS_EDGE(n1, n1);
        ASSERT_HAS_EDGE(n1, n2);
        ASSERT_NOT_HAS_EDGE(n1, n3);
        ASSERT_NOT_HAS_EDGE(n1, n4);
        ASSERT_NOT_HAS_EDGE(n1, n5);
        ASSERT_NOT_HAS_EDGE(n1, n6);
        ASSERT_NOT_HAS_EDGE(n1, n7);
        ASSERT_NOT_HAS_EDGE(n1, n8);
        
        ASSERT_NOT_HAS_EDGE(n2, n1);
        ASSERT_NOT_HAS_EDGE(n2, n2);
        ASSERT_HAS_EDGE(n2, n3);
        ASSERT_NOT_HAS_EDGE(n2, n4);
        ASSERT_NOT_HAS_EDGE(n2, n5);
        ASSERT_NOT_HAS_EDGE(n2, n6);
        ASSERT_NOT_HAS_EDGE(n2, n7);
        ASSERT_NOT_HAS_EDGE(n2, n8);
        
        ASSERT_HAS_EDGE(n3, n1);
        ASSERT_NOT_HAS_EDGE(n3, n2);
        ASSERT_NOT_HAS_EDGE(n3, n3);
        ASSERT_NOT_HAS_EDGE(n3, n4);
        ASSERT_NOT_HAS_EDGE(n3, n5);
        ASSERT_NOT_HAS_EDGE(n3, n6);
        ASSERT_NOT_HAS_EDGE(n3, n7);
        ASSERT_NOT_HAS_EDGE(n3, n8);
        
        ASSERT_NOT_HAS_EDGE(n4, n1);
        ASSERT_HAS_EDGE(n4, n2);
        ASSERT_HAS_EDGE(n4, n3);
        ASSERT_NOT_HAS_EDGE(n4, n4);
        ASSERT_HAS_EDGE(n4, n5);
        ASSERT_NOT_HAS_EDGE(n4, n6);
        ASSERT_NOT_HAS_EDGE(n4, n7);
        ASSERT_NOT_HAS_EDGE(n4, n8);
        
        ASSERT_NOT_HAS_EDGE(n5, n1);
        ASSERT_NOT_HAS_EDGE(n5, n2);
        ASSERT_NOT_HAS_EDGE(n5, n3);
        ASSERT_HAS_EDGE(n5, n4);
        ASSERT_NOT_HAS_EDGE(n5, n5);
        ASSERT_HAS_EDGE(n5, n6);
        ASSERT_NOT_HAS_EDGE(n5, n7);
        ASSERT_NOT_HAS_EDGE(n5, n8);
        
        ASSERT_NOT_HAS_EDGE(n6, n1);
        ASSERT_NOT_HAS_EDGE(n6, n2);
        ASSERT_HAS_EDGE(n6, n3);
        ASSERT_NOT_HAS_EDGE(n6, n4);
        ASSERT_NOT_HAS_EDGE(n6, n5);
        ASSERT_NOT_HAS_EDGE(n6, n6);
        ASSERT_HAS_EDGE(n6, n7);
        ASSERT_NOT_HAS_EDGE(n6, n8);
        
        ASSERT_NOT_HAS_EDGE(n7, n1);
        ASSERT_NOT_HAS_EDGE(n7, n2);
        ASSERT_NOT_HAS_EDGE(n7, n3);
        ASSERT_NOT_HAS_EDGE(n7, n4);
        ASSERT_NOT_HAS_EDGE(n7, n5);
        ASSERT_HAS_EDGE(n7, n6);
        ASSERT_NOT_HAS_EDGE(n7, n7);
        ASSERT_NOT_HAS_EDGE(n7, n8);
        
        ASSERT_NOT_HAS_EDGE(n8, n1);
        ASSERT_NOT_HAS_EDGE(n8, n2);
        ASSERT_NOT_HAS_EDGE(n8, n3);
        ASSERT_NOT_HAS_EDGE(n8, n4);
        ASSERT_HAS_EDGE(n8, n5);
        ASSERT_NOT_HAS_EDGE(n8, n6);
        ASSERT_HAS_EDGE(n8, n7);
        ASSERT_HAS_EDGE(n8, n8);
        
        std::string foo = randomStr() + "foo";
        assertFalse(graph.hasEdge(n1, foo), "hasEdge n1 foo");
        assertFalse(graph.hasEdge(foo, n1), "hasEdge foo n1");
        assertFalse(graph.hasEdge(foo, foo), "hasEdge foo foo");
    }
    testEnd("hasEdge");
}

#define ASSERT_NEIGHBORS(n, ...) \
    std::vector<std::string> expectedYes##n = {__VA_ARGS__}; \
    std::set<std::string> actualYes##n = graph.neighbors(n); \
    for (const auto& x : expectedYes##n) { \
        assertTrue(actualYes##n.contains(x), "neighbors " #n " + x"); \
    }

#define ASSERT_NOT_NEIGHBORS(n, ...) \
    std::vector<std::string> expectedNo##n = {__VA_ARGS__}; \
    std::set<std::string> actualNo##n = graph.neighbors(n); \
    for (const auto& x : expectedNo##n) { \
        assertFalse(actualNo##n.contains(x), "neighbors " #n " + x"); \
    }


void testNeighbors() {
    testStart("hasNeighbors");
    for (int i = 0; i < N_TRIALS_PER_TEST; i++) {
        BUILD_GRAPH;
        
        ASSERT_NEIGHBORS(n1, n2); ASSERT_NOT_NEIGHBORS(n1, n1, n3, n4, n5, n6, n7, n8);
        ASSERT_NEIGHBORS(n2, n3); ASSERT_NOT_NEIGHBORS(n2, n1, n2, n4, n5, n6, n7, n8);
        ASSERT_NEIGHBORS(n3, n1); ASSERT_NOT_NEIGHBORS(n3, n2, n3, n4, n5, n6, n7, n8);
        ASSERT_NEIGHBORS(n4, n2, n3, n5); ASSERT_NOT_NEIGHBORS(n4, n1, n4, n6, n7, n8);
        ASSERT_NEIGHBORS(n5, n4, n6); ASSERT_NOT_NEIGHBORS(n5, n1, n2, n3, n5, n7, n8);
        ASSERT_NEIGHBORS(n6, n3, n7); ASSERT_NOT_NEIGHBORS(n6, n1, n2, n4, n5, n6, n8);
        ASSERT_NEIGHBORS(n7, n6); ASSERT_NOT_NEIGHBORS(n7, n1, n2, n3, n4, n5, n7, n8);
        ASSERT_NEIGHBORS(n8, n5, n7, n8); ASSERT_NOT_NEIGHBORS(n8, n1, n2, n3, n4, n6);
        
        std::string foo = randomStr() + "foo";
        std::string bar = randomStr() + "bar";
        
        ASSERT_NOT_NEIGHBORS(foo, n1, n2, n3, n4, n5, n6, n7, n8, foo, bar);
        ASSERT_NOT_NEIGHBORS(bar, n1, n2, n3, n4, n5, n6, n7, n8, foo, bar);
    }
    testEnd("hasNeighbors");
}

#define ASSERT_IN_GET_ALL_NODES(n) \
    assertTrue(std::find(allNodes.begin(), allNodes.end(), n) != allNodes.end(), "getAllNodes " #n);

#define ASSERT_NOT_IN_GET_ALL_NODES(n) \
    assertFalse(std::find(allNodes.begin(), allNodes.end(), n) != allNodes.end(), "getAllNodes " #n);


void testGetAllNodes() {
    testStart("getAllNodes");
    
    for (int i = 0; i < N_TRIALS_PER_TEST; i++) {
        BUILD_GRAPH;
        std::vector<std::string> allNodes = graph.getAllNodes();
        ASSERT_IN_GET_ALL_NODES(n1);
        ASSERT_IN_GET_ALL_NODES(n2);
        ASSERT_IN_GET_ALL_NODES(n3);
        ASSERT_IN_GET_ALL_NODES(n4);
        ASSERT_IN_GET_ALL_NODES(n5);
        ASSERT_IN_GET_ALL_NODES(n6);
        ASSERT_IN_GET_ALL_NODES(n7);
        ASSERT_IN_GET_ALL_NODES(n8);
        
        std::string foo = randomStr() + "foo";
        std::string bar = randomStr() + "bar";
        ASSERT_NOT_IN_GET_ALL_NODES(foo);
        ASSERT_NOT_IN_GET_ALL_NODES(bar);
    }
    testEnd("getAllNodes");
}

#define ASSERT_SAME_SCC(n1, n2) \
    assertTrue(outToSCCMapping[n1] == outToSCCMapping[n2], #n1 " == " #n2);

#define ASSERT_DIFFERENT_SCC(n1, n2) \
    assertTrue(outToSCCMapping[n1] != outToSCCMapping[n2], #n1 " != " #n2);

#define ASSERT_HAS_SCC_EDGE(n1, n2) \
    assertTrue(outDirectedGraph.hasEdge(outToSCCMapping[n1], outToSCCMapping[n2]), "SCC hasEdge " #n1 " " #n2);

#define ASSERT_NOT_HAS_SCC_EDGE(n1, n2) \
    assertFalse(outDirectedGraph.hasEdge(outToSCCMapping[n1], outToSCCMapping[n2]), "SCC hasEdge " #n1 " " #n2);


void testFindStronglyConnectedComponents() {
    testStart("findStronglyConnectedComponents");
    
    for (int i = 0; i < N_TRIALS_PER_TEST; i++) {
        BUILD_GRAPH;
        
        std::vector<std::unique_ptr<std::set<std::string>>> outSCCs;
        std::map<std::string, std::set<std::string>*> outToSCCMapping;
        DirectedGraph<std::set<std::string>*> outDirectedGraph;
        
        graph.findStronglyConnectedComponents(outSCCs, outToSCCMapping, outDirectedGraph);
        
        ASSERT_SAME_SCC(n1, n1);
        ASSERT_SAME_SCC(n1, n2);
        ASSERT_SAME_SCC(n1, n3);
        ASSERT_DIFFERENT_SCC(n1, n4);
        ASSERT_DIFFERENT_SCC(n1, n5);
        ASSERT_DIFFERENT_SCC(n1, n6);
        ASSERT_DIFFERENT_SCC(n1, n7);
        ASSERT_DIFFERENT_SCC(n1, n8);
        
        ASSERT_SAME_SCC(n2, n1);
        ASSERT_SAME_SCC(n2, n2);
        ASSERT_SAME_SCC(n2, n3);
        ASSERT_DIFFERENT_SCC(n2, n4);
        ASSERT_DIFFERENT_SCC(n2, n5);
        ASSERT_DIFFERENT_SCC(n2, n6);
        ASSERT_DIFFERENT_SCC(n2, n7);
        ASSERT_DIFFERENT_SCC(n2, n8);
        
        ASSERT_SAME_SCC(n3, n1);
        ASSERT_SAME_SCC(n3, n2);
        ASSERT_SAME_SCC(n3, n3);
        ASSERT_DIFFERENT_SCC(n3, n4);
        ASSERT_DIFFERENT_SCC(n3, n5);
        ASSERT_DIFFERENT_SCC(n3, n6);
        ASSERT_DIFFERENT_SCC(n3, n7);
        ASSERT_DIFFERENT_SCC(n3, n8);
        
        ASSERT_DIFFERENT_SCC(n4, n1);
        ASSERT_DIFFERENT_SCC(n4, n2);
        ASSERT_DIFFERENT_SCC(n4, n3);
        ASSERT_SAME_SCC(n4, n4);
        ASSERT_SAME_SCC(n4, n5);
        ASSERT_DIFFERENT_SCC(n4, n6);
        ASSERT_DIFFERENT_SCC(n4, n7);
        ASSERT_DIFFERENT_SCC(n4, n8);
        
        ASSERT_DIFFERENT_SCC(n5, n1);
        ASSERT_DIFFERENT_SCC(n5, n2);
        ASSERT_DIFFERENT_SCC(n5, n3);
        ASSERT_SAME_SCC(n5, n4);
        ASSERT_SAME_SCC(n5, n5);
        ASSERT_DIFFERENT_SCC(n5, n6);
        ASSERT_DIFFERENT_SCC(n5, n7);
        ASSERT_DIFFERENT_SCC(n5, n8);
        
        ASSERT_DIFFERENT_SCC(n6, n1);
        ASSERT_DIFFERENT_SCC(n6, n2);
        ASSERT_DIFFERENT_SCC(n6, n3);
        ASSERT_DIFFERENT_SCC(n6, n4);
        ASSERT_DIFFERENT_SCC(n6, n5);
        ASSERT_SAME_SCC(n6, n6);
        ASSERT_SAME_SCC(n6, n7);
        ASSERT_DIFFERENT_SCC(n6, n8);
        
        ASSERT_DIFFERENT_SCC(n7, n1);
        ASSERT_DIFFERENT_SCC(n7, n2);
        ASSERT_DIFFERENT_SCC(n7, n3);
        ASSERT_DIFFERENT_SCC(n7, n4);
        ASSERT_DIFFERENT_SCC(n7, n5);
        ASSERT_SAME_SCC(n7, n6);
        ASSERT_SAME_SCC(n7, n7);
        ASSERT_DIFFERENT_SCC(n7, n8);
        
        ASSERT_DIFFERENT_SCC(n8, n1);
        ASSERT_DIFFERENT_SCC(n8, n2);
        ASSERT_DIFFERENT_SCC(n8, n3);
        ASSERT_DIFFERENT_SCC(n8, n4);
        ASSERT_DIFFERENT_SCC(n8, n5);
        ASSERT_DIFFERENT_SCC(n8, n6);
        ASSERT_DIFFERENT_SCC(n8, n7);
        ASSERT_SAME_SCC(n8, n8);
        
        
        
        ASSERT_NOT_HAS_SCC_EDGE(n1, n1);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n2);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n5);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n6);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n1, n8);
        
        ASSERT_NOT_HAS_SCC_EDGE(n2, n1);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n2);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n5);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n6);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n2, n8);
        
        ASSERT_NOT_HAS_SCC_EDGE(n3, n1);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n2);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n5);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n6);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n3, n8);
        
        ASSERT_HAS_SCC_EDGE(n4, n1);
        ASSERT_HAS_SCC_EDGE(n4, n2);
        ASSERT_HAS_SCC_EDGE(n4, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n4, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n4, n5);
        ASSERT_HAS_SCC_EDGE(n4, n6);
        ASSERT_HAS_SCC_EDGE(n4, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n4, n8);
        
        ASSERT_HAS_SCC_EDGE(n5, n1);
        ASSERT_HAS_SCC_EDGE(n5, n2);
        ASSERT_HAS_SCC_EDGE(n5, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n5, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n5, n5);
        ASSERT_HAS_SCC_EDGE(n5, n6);
        ASSERT_HAS_SCC_EDGE(n5, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n5, n8);
        
        ASSERT_HAS_SCC_EDGE(n6, n1);
        ASSERT_HAS_SCC_EDGE(n6, n2);
        ASSERT_HAS_SCC_EDGE(n6, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n6, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n6, n5);
        ASSERT_NOT_HAS_SCC_EDGE(n6, n6);
        ASSERT_NOT_HAS_SCC_EDGE(n6, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n6, n8);
        
        ASSERT_HAS_SCC_EDGE(n7, n1);
        ASSERT_HAS_SCC_EDGE(n7, n2);
        ASSERT_HAS_SCC_EDGE(n7, n3);
        ASSERT_NOT_HAS_SCC_EDGE(n7, n4);
        ASSERT_NOT_HAS_SCC_EDGE(n7, n5);
        ASSERT_NOT_HAS_SCC_EDGE(n7, n6);
        ASSERT_NOT_HAS_SCC_EDGE(n7, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n7, n8);
        
        ASSERT_NOT_HAS_SCC_EDGE(n8, n1);
        ASSERT_NOT_HAS_SCC_EDGE(n8, n2);
        ASSERT_NOT_HAS_SCC_EDGE(n8, n3);
        ASSERT_HAS_SCC_EDGE(n8, n4);
        ASSERT_HAS_SCC_EDGE(n8, n5);
        ASSERT_HAS_SCC_EDGE(n8, n6);
        ASSERT_HAS_SCC_EDGE(n8, n7);
        ASSERT_NOT_HAS_SCC_EDGE(n8, n8);
    }
    testEnd("findStronglyConnectedComponents");
}

void testHandcraftedNodeInMultipleSCCs() {
    DirectedGraph<std::string> directedGraph;
    directedGraph.addNode("a");
    directedGraph.addEdge("a", "b");
    directedGraph.addNode("a");
    
    std::vector<std::unique_ptr<std::set<std::string>>> outSccs;
    std::map<std::string, std::set<std::string>*> outToSCCMapping;
    DirectedGraph<std::set<std::string>*> outDirectedGraph;
    directedGraph.findStronglyConnectedComponents(outSccs, outToSCCMapping, outDirectedGraph);
    
    for (const auto& scc : outSccs) {
        for (std::string node : *scc) {
            if (outToSCCMapping[node] != scc.get()) {
                __builtin_trap();
            }
        }
    }    
}
