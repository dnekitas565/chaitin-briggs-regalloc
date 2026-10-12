// Owner: B
#include "interference.h"

#include <sstream>

InterferenceGraph buildInterference(const CFG& cfg) {
    InterferenceGraph ig(cfg.numVRegs());
    for (const BasicBlock& b : cfg.blocks) {
        std::set<VReg> live = b.liveOut;
        for (auto it = b.code.rbegin(); it != b.code.rend(); ++it) {
            const Instr& in = *it;
            // `live` is the set live after this instruction.
            for (VReg d : in.defs) {
                for (VReg l : live) {
                    if (in.isMove && l == in.uses[0]) continue;  // d = s: s may share d's register
                    ig.addEdge(d, l);
                }
                if (in.isMove) ig.addMove(d, in.uses[0]);
            }
            for (VReg d : in.defs) live.erase(d);
            for (VReg u : in.uses) live.insert(u);
        }
    }
    return ig;
}

std::string dumpIG(const InterferenceGraph& ig, const CFG& cfg) {
    auto name = [&](VReg v) { return cfg.vregNames[static_cast<size_t>(v)]; };
    std::ostringstream os;
    for (VReg v = 0; v < ig.size(); ++v) {
        const IGNode& n = ig.nodes[static_cast<size_t>(v)];
        os << name(v) << " (deg " << n.adj.size() << "):";
        for (VReg a : n.adj) os << " " << name(a);
        if (!n.moves.empty()) {
            os << "  | moves:";
            for (VReg m : n.moves) os << " " << name(m);
        }
        os << "\n";
    }
    return os.str();
}

std::string igToDot(const InterferenceGraph& ig, const CFG& cfg) {
    auto name = [&](VReg v) { return "\"" + cfg.vregNames[static_cast<size_t>(v)] + "\""; };
    std::ostringstream os;
    os << "graph IG {\n";
    for (VReg v = 0; v < ig.size(); ++v) os << "  " << name(v) << ";\n";
    for (VReg v = 0; v < ig.size(); ++v) {
        for (VReg a : ig.nodes[static_cast<size_t>(v)].adj)
            if (v < a) os << "  " << name(v) << " -- " << name(a) << ";\n";
        for (VReg m : ig.nodes[static_cast<size_t>(v)].moves)
            if (v < m) os << "  " << name(v) << " -- " << name(m) << " [style=dashed, color=blue];\n";
    }
    os << "}\n";
    return os.str();
}
