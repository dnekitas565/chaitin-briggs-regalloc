// Owner: A
#include "allocator.h"

#include "cfg.h"
#include "interference.h"
#include "liveness.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Optimistic coloring
// ---------------------------------------------------------------------------

Allocation colorGraph(const InterferenceGraph& ig, int k, const std::vector<double>& cost,
                      const std::vector<bool>& active) {
    const int n = ig.size();
    Allocation alloc;
    alloc.k = k;
    alloc.color.assign(static_cast<size_t>(n), -1);

    std::vector<bool> remaining(static_cast<size_t>(n), false);
    int left = 0;
    for (int v = 0; v < n; ++v)
        if (active[static_cast<size_t>(v)]) {
            remaining[static_cast<size_t>(v)] = true;
            ++left;
        }
    auto liveDegree = [&](VReg v) {
        int d = 0;
        for (VReg u : ig.nodes[static_cast<size_t>(v)].adj)
            if (remaining[static_cast<size_t>(u)]) ++d;
        return d;
    };
    auto moveRelated = [&](VReg v) {
        for (VReg u : ig.nodes[static_cast<size_t>(v)].moves)
            if (remaining[static_cast<size_t>(u)]) return true;
        return false;
    };

    // Simplify: push nodes; spill candidates are pushed optimistically too.
    std::vector<VReg> stack;
    while (left > 0) {
        VReg pick = -1;
        // 1) low-degree node that is not move-related
        for (VReg v = 0; v < n && pick < 0; ++v)
            if (remaining[static_cast<size_t>(v)] && liveDegree(v) < k && !moveRelated(v)) pick = v;
        // 2) only move-related low-degree nodes are left: freeze one (its moves are given up)
        for (VReg v = 0; v < n && pick < 0; ++v)
            if (remaining[static_cast<size_t>(v)] && liveDegree(v) < k) pick = v;
        // 3) potential spill: minimize cost/degree, push anyway (no spilling decision here)
        if (pick < 0) {
            double best = std::numeric_limits<double>::infinity();
            for (VReg v = 0; v < n; ++v) {
                if (!remaining[static_cast<size_t>(v)]) continue;
                double ratio = cost[static_cast<size_t>(v)] / liveDegree(v);
                if (pick < 0 || ratio < best) {
                    pick = v;
                    best = ratio;
                }
            }
            alloc.candidates.insert(pick);
        }
        remaining[static_cast<size_t>(pick)] = false;
        --left;
        stack.push_back(pick);
    }

    // Select: pop and take the lowest color not used by already-colored neighbors.
    while (!stack.empty()) {
        VReg v = stack.back();
        stack.pop_back();
        std::vector<bool> used(static_cast<size_t>(k), false);
        for (VReg u : ig.nodes[static_cast<size_t>(v)].adj) {
            int c = alloc.color[static_cast<size_t>(u)];
            if (c >= 0) used[static_cast<size_t>(c)] = true;
        }
        int c = 0;
        while (c < k && used[static_cast<size_t>(c)]) ++c;
        if (c < k)
            alloc.color[static_cast<size_t>(v)] = c;
        else
            alloc.spilled.insert(v);  // real spill
    }
    return alloc;
}

bool verifyColoring(const InterferenceGraph& ig, const Allocation& alloc) {
    for (VReg v = 0; v < ig.size(); ++v) {
        int c = alloc.color[static_cast<size_t>(v)];
        if (c >= alloc.k) return false;
        if (c < 0) continue;
        for (VReg u : ig.nodes[static_cast<size_t>(v)].adj)
            if (alloc.color[static_cast<size_t>(u)] == c) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Conservative (Briggs) coalescing
// ---------------------------------------------------------------------------

bool briggsTest(const InterferenceGraph& ig, VReg a, VReg b, int k) {
    const auto& na = ig.nodes[static_cast<size_t>(a)].adj;
    const auto& nb = ig.nodes[static_cast<size_t>(b)].adj;
    std::set<VReg> neighbors(na.begin(), na.end());
    neighbors.insert(nb.begin(), nb.end());
    neighbors.erase(a);
    neighbors.erase(b);
    int significant = 0;
    for (VReg x : neighbors) {
        int deg = ig.degree(x);
        if (na.count(x) && nb.count(x)) --deg;  // x loses one edge when a and b merge
        if (deg >= k) ++significant;
    }
    return significant < k;
}

CoalesceResult coalesceGraph(const InterferenceGraph& ig, int k) {
    CoalesceResult res{ig, {}, 0};
    InterferenceGraph& g = res.merged;
    const int n = g.size();
    res.alias.resize(static_cast<size_t>(n));
    for (int v = 0; v < n; ++v) res.alias[static_cast<size_t>(v)] = v;

    bool changed = true;
    while (changed) {
        changed = false;
        for (VReg a = 0; a < n && !changed; ++a) {
            for (VReg b : g.nodes[static_cast<size_t>(a)].moves) {
                if (b <= a) continue;                // consider each unordered pair once; a is the survivor
                if (g.hasEdge(a, b)) continue;       // interfering pairs are never merged
                if (!briggsTest(g, a, b, k)) continue;
                // Merge b into a: adjacency and move lists.
                for (VReg x : std::set<VReg>(g.nodes[static_cast<size_t>(b)].adj)) {
                    g.nodes[static_cast<size_t>(x)].adj.erase(b);
                    g.addEdge(a, x);
                }
                g.nodes[static_cast<size_t>(b)].adj.clear();
                for (VReg x : std::set<VReg>(g.nodes[static_cast<size_t>(b)].moves)) {
                    g.nodes[static_cast<size_t>(x)].moves.erase(b);
                    g.addMove(a, x);
                }
                g.nodes[static_cast<size_t>(b)].moves.clear();
                g.nodes[static_cast<size_t>(a)].moves.erase(b);
                res.alias[static_cast<size_t>(b)] = a;
                ++res.coalescedPairs;
                changed = true;  // iterators into `moves` are invalid now; restart the scan
                break;
            }
        }
    }
    for (int v = 0; v < n; ++v) {
        VReg r = v;
        while (res.alias[static_cast<size_t>(r)] != r) r = res.alias[static_cast<size_t>(r)];
        res.alias[static_cast<size_t>(v)] = r;
    }
    return res;
}

// ---------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------

namespace {

int countMoves(const CFG& cfg) {
    int n = 0;
    for (const auto& b : cfg.blocks)
        for (const auto& in : b.code)
            if (in.op == Op::Move) ++n;
    return n;
}

/// Rename every vreg to its representative and delete moves whose operands became identical.
void applyAlias(CFG& cfg, const std::vector<VReg>& alias) {
    for (auto& b : cfg.blocks) {
        std::vector<Instr> out;
        for (Instr& in : b.code) {
            for (VReg& d : in.defs) d = alias[static_cast<size_t>(d)];
            for (VReg& u : in.uses) u = alias[static_cast<size_t>(u)];
            if (in.op == Op::Move && in.defs[0] == in.uses[0]) continue;
            out.push_back(std::move(in));
        }
        b.code = std::move(out);
    }
}

std::vector<bool> occurringVRegs(const CFG& cfg) {
    std::vector<bool> occurs(static_cast<size_t>(cfg.numVRegs()), false);
    for (const auto& b : cfg.blocks)
        for (const auto& in : b.code) {
            for (VReg d : in.defs) occurs[static_cast<size_t>(d)] = true;
            for (VReg u : in.uses) occurs[static_cast<size_t>(u)] = true;
        }
    return occurs;
}

}  // namespace

AllocResult allocate(const CFG& input, const AllocOptions& opts) {
    if (!opts.cost) throw std::invalid_argument("allocate: a cost model is required");
    AllocResult R;
    R.cfg = input;
    R.costName = opts.cost->name();
    analyzeCFG(R.cfg);  // edges + loop depths; block ids stay stable across rounds

    for (int round = 1; round <= opts.maxRounds; ++round) {
        computeLiveness(R.cfg);
        InterferenceGraph ig = buildInterference(R.cfg);

        // Coalesce only in the first round: later rounds contain spill temporaries that must stay short-lived.
        if (round == 1 && opts.coalesce) {
            CoalesceResult cr = coalesceGraph(ig, opts.k);
            if (cr.coalescedPairs > 0) {
                int before = countMoves(R.cfg);
                applyAlias(R.cfg, cr.alias);
                R.movesRemoved = before - countMoves(R.cfg);
                computeLiveness(R.cfg);
                ig = buildInterference(R.cfg);
            }
        }

        std::vector<double> cost = computeSpillCosts(R.cfg, *opts.cost);
        for (VReg t : R.spill.noSpill) cost[static_cast<size_t>(t)] = std::numeric_limits<double>::infinity();
        std::vector<bool> active = occurringVRegs(R.cfg);

        Allocation alloc = colorGraph(ig, opts.k, cost, active);
        if (!verifyColoring(ig, alloc)) throw std::logic_error("internal error: invalid coloring");
        R.rounds = round;
        R.ig = ig;
        R.alloc = alloc;

        if (alloc.spilled.empty()) {
            if (opts.coalesce) {
                // Moves whose operands received the same register become no-ops in physical form.
                // They stay in R.cfg (still distinct vregs there); emitAllocated() drops them.
                for (const auto& b : R.cfg.blocks)
                    for (const auto& in : b.code)
                        if (in.op == Op::Move && alloc.color[static_cast<size_t>(in.defs[0])] ==
                                                     alloc.color[static_cast<size_t>(in.uses[0])])
                            ++R.movesSameRegister;
                R.movesRemoved += R.movesSameRegister;
            }
            std::set<int> colors;
            for (VReg v = 0; v < ig.size(); ++v)
                if (active[static_cast<size_t>(v)]) colors.insert(alloc.color[static_cast<size_t>(v)]);
            R.registersUsed = static_cast<int>(colors.size());
            return R;
        }
        for (VReg v : alloc.spilled) {
            if (R.spill.noSpill.count(v))
                throw std::runtime_error("cannot allocate with k=" + std::to_string(opts.k) +
                                         ": a short-lived spill temporary needs a register");
            R.spillVictims.push_back(R.spill.originOf(v));
        }
        insertSpillCode(R.cfg, alloc.spilled, R.spill);
    }
    throw std::runtime_error("register allocation did not converge within " + std::to_string(opts.maxRounds) +
                             " rounds (k=" + std::to_string(opts.k) + ")");
}
