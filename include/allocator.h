// Owner: A
#pragma once

#include "costmodel.h"
#include "ir.h"
#include "spillcode.h"

#include <string>
#include <vector>

/// Optimistic Chaitin-Briggs coloring of a single graph.
/// `active[v]` is false for vregs that do not occur in the program (they stay uncolored, never spilled).
/// `cost[v]` is the spill cost (infinity = must not be spilled).
Allocation colorGraph(const InterferenceGraph& ig, int k, const std::vector<double>& cost,
                      const std::vector<bool>& active);
/// True iff no two adjacent nodes share a color and every color is in [0,k).
bool verifyColoring(const InterferenceGraph& ig, const Allocation& alloc);

/// Briggs test: the merged node would have fewer than k neighbors of degree >= k.
bool briggsTest(const InterferenceGraph& ig, VReg a, VReg b, int k);

struct CoalesceResult {
    InterferenceGraph merged;
    std::vector<VReg> alias;  // alias[v] = representative of v
    int coalescedPairs = 0;
};
CoalesceResult coalesceGraph(const InterferenceGraph& ig, int k);

struct AllocOptions {
    int k = 4;
    bool coalesce = true;
    const CostModel* cost = nullptr;  // required
    int maxRounds = 10;
};

struct AllocResult {
    CFG cfg;                         // final rewritten program (virtual form, every vreg colored)
    InterferenceGraph ig;            // graph of the final round
    Allocation alloc;                // final coloring
    SpillState spill;
    std::vector<VReg> spillVictims;  // original vregs really spilled (all rounds, in order)
    int rounds = 0;
    int movesRemoved = 0;        // coalesced away + same-register moves (total)
    int movesSameRegister = 0;   // part of movesRemoved still present in cfg; dropped at emit time
    int registersUsed = 0;
    std::string costName;
};

/// Driver: [coalesce] -> liveness -> IG -> color -> spill code -> repeat (<= maxRounds, else throws).
AllocResult allocate(const CFG& input, const AllocOptions& opts);
