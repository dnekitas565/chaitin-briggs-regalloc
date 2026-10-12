// Owner: A
#include "test_framework.h"

#include "allocator.h"
#include "cfg.h"
#include "emit.h"
#include "interp.h"
#include "parser.h"

#include <string>

namespace {
// Builds a (a=0, b=1) move pair plus `neighbors` extra nodes; each neighbor in `heavy` has k filler
// neighbors so that its degree is >= k.
struct Fixture {
    InterferenceGraph g;
    int k;
    Fixture(int k_, int plain, int heavy) : g(0), k(k_) {
        int total = 2 + plain + heavy + k_;
        g = InterferenceGraph(total);
        g.addMove(0, 1);
        int fillerBase = 2 + plain + heavy;
        for (int i = 0; i < plain; ++i) g.addEdge(i % 2, 2 + i);
        for (int i = 0; i < heavy; ++i) {
            int x = 2 + plain + i;
            g.addEdge(i % 2, x);
            for (int f = 0; f < k_; ++f) g.addEdge(x, fillerBase + f);
        }
    }
};

int countMoves(const CFG& cfg) {
    int n = 0;
    for (const auto& b : cfg.blocks)
        for (const auto& in : b.code)
            if (in.op == Op::Move) ++n;
    return n;
}
}  // namespace

TEST(briggsAllowsKMinusOneSignificantNeighbors) {
    Fixture f(3, /*plain=*/2, /*heavy=*/2);  // 2 significant neighbors, k = 3
    ASSERT_TRUE(briggsTest(f.g, 0, 1, 3));
    CoalesceResult cr = coalesceGraph(f.g, 3);
    ASSERT_EQ(cr.coalescedPairs, 1);
    ASSERT_EQ(cr.alias[1], 0);
}

TEST(briggsRejectsKSignificantNeighbors) {
    Fixture f(3, /*plain=*/0, /*heavy=*/3);  // 3 significant neighbors, k = 3
    ASSERT_FALSE(briggsTest(f.g, 0, 1, 3));
    CoalesceResult cr = coalesceGraph(f.g, 3);
    ASSERT_EQ(cr.coalescedPairs, 0);
    ASSERT_EQ(cr.alias[1], 1);
}

TEST(commonNeighborLosesOneDegreeAfterMerge) {
    // k = 3: c is adjacent to both a and b and to one filler, degree 3. After merging it has degree 2.
    InterferenceGraph g(4);
    g.addMove(0, 1);
    g.addEdge(0, 2);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    ASSERT_EQ(g.degree(2), 3);
    ASSERT_TRUE(briggsTest(g, 0, 1, 3));
}

TEST(interferingMovePairIsNeverMerged) {
    InterferenceGraph g(2);
    g.addMove(0, 1);
    g.addEdge(0, 1);
    CoalesceResult cr = coalesceGraph(g, 8);
    ASSERT_EQ(cr.coalescedPairs, 0);
    ASSERT_EQ(cr.alias[1], 1);
}

TEST(mergingUnionsAdjacencyAndMoves) {
    InterferenceGraph g(5);  // a~b, b~c moves; a-x, c-y edges
    g.addMove(0, 1);
    g.addMove(1, 2);
    g.addEdge(0, 3);
    g.addEdge(2, 4);
    CoalesceResult cr = coalesceGraph(g, 4);
    ASSERT_EQ(cr.coalescedPairs, 2);
    ASSERT_EQ(cr.alias[1], 0);
    ASSERT_EQ(cr.alias[2], 0);
    ASSERT_TRUE(cr.merged.hasEdge(0, 3));
    ASSERT_TRUE(cr.merged.hasEdge(0, 4));
    ASSERT_TRUE(cr.merged.hasEdge(4, 0));
    ASSERT_EQ(cr.merged.degree(1), 0);
    ASSERT_EQ(cr.merged.degree(2), 0);
}

TEST(moveChainSampleLosesMovesAndKeepsOutput) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/move_chain.tac");
    StaticCost sc;
    int before = countMoves(cfg);
    AllocResult on = allocate(cfg, {4, true, &sc, 10});
    AllocResult off = allocate(cfg, {4, false, &sc, 10});
    ASSERT_TRUE(on.movesRemoved > 0);
    ASSERT_EQ(off.movesRemoved, 0);
    ASSERT_EQ(countMoves(on.cfg), before - (on.movesRemoved - on.movesSameRegister));
    ASSERT_EQ(runProgram(on.cfg).printed, runProgram(cfg).printed);
    ASSERT_TRUE(on.registersUsed <= off.registersUsed);
}

TEST(coalescingNeverIncreasesSpillsOnMoveChain) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/move_chain.tac");
    StaticCost sc;
    for (int k = 3; k <= 8; ++k) {
        AllocResult on = allocate(cfg, {k, true, &sc, 10});
        AllocResult off = allocate(cfg, {k, false, &sc, 10});
        ASSERT_TRUE(on.spillVictims.size() <= off.spillVictims.size());
        ASSERT_TRUE(verifyColoring(on.ig, on.alloc));
    }
}

TEST(coalescingPreservesOutputsOnAllSamples) {
    for (const char* n : {"straight_line.tac", "branchy.tac", "loop_high_pressure.tac", "loop_splittable.tac",
                          "move_chain.tac", "profile_demo.tac"}) {
        CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/" + n);
        StaticCost sc;
        for (int k = 3; k <= 8; ++k) {
            AllocResult r = allocate(cfg, {k, true, &sc, 10});
            ASSERT_EQ(runProgram(r.cfg).printed, runProgram(cfg).printed);
        }
    }
}

TEST(noSameRegisterMoveSurvivesInEmittedCode) {
    for (const char* n : {"move_chain.tac", "branchy.tac", "profile_demo.tac"}) {
        CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/" + n);
        StaticCost sc;
        AllocResult r = allocate(cfg, {4, true, &sc, 10});
        CFG physical = parseTAC(emitAllocated(r, 4));
        for (const auto& b : physical.blocks)
            for (const auto& in : b.code)
                if (in.op == Op::Move) ASSERT_NE(in.defs[0], in.uses[0]);
    }
}

TEST(moveChainEmitsNoMovesAtAll) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/move_chain.tac");
    StaticCost sc;
    AllocResult r = allocate(cfg, {4, true, &sc, 10});
    ASSERT_EQ(countMoves(parseTAC(emitAllocated(r, 4))), 0);
}

TEST_MAIN()
