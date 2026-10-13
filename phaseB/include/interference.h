#ifndef INTERFERENCE_H
#define INTERFERENCE_H

#include "ir.h"

struct InterferenceGraph {
    std::unordered_map<std::string, std::unordered_set<std::string>> adjList;
    std::unordered_set<std::string> nodes;
    void addEdge(const std::string& u, const std::string& v);
};

InterferenceGraph buildInterferenceGraph(const CFG& cfg);

#endif // INTERFERENCE_H
