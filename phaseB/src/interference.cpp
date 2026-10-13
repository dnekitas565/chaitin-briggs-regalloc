#include "../include/interference.h"

void InterferenceGraph::addEdge(const std::string& u, const std::string& v) {
    if (u != v) {
        adjList[u].insert(v);
        adjList[v].insert(u);
        nodes.insert(u);
        nodes.insert(v);
    }
}

InterferenceGraph buildInterferenceGraph(const CFG& cfg) {
    InterferenceGraph ig;
    for (const auto& bb : cfg.blocks) {
        std::unordered_set<std::string> live = bb.liveOut;
        for (auto it = bb.instrs.rbegin(); it != bb.instrs.rend(); ++it) {
            if (it->isMove && !it->srcs.empty()) {
                live.erase(it->srcs[0]);
            }
            if (!it->dest.empty()) {
                for (const auto& l : live) {
                    ig.addEdge(it->dest, l);
                }
            }
            if (!it->dest.empty()) live.erase(it->dest);
            for (const auto& src : it->srcs) live.insert(src);
        }
    }
    return ig;
}
