// Owner: A
#include "test_framework.h"

#include "allocator.h"
#include "cfg.h"
#include "interp.h"
#include "parser.h"

#include <limits>
#include <string>

static std::vector<bool> allActive(int n) { return std::vector<bool>(static_cast<size_t>(n), true); }
static std::vector<double> uniform(int n, double c = 1.0) { return std::vector<double>(static_cast<size_t>(n), c); }

static InterferenceGraph clique(int n) {
    InterferenceGraph g(n);
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) g.addEdge(i, j);
    return g;
}

static const char* SAMPLE_NAMES[] = {"straight_line.tac", "branchy.tac", "loop_high_pressure.tac",
                                     "loop_splittable.tac", "move_chain.tac", "profile_demo.tac"};

TEST(kColorableGraphHasNoSpills) {
    InterferenceGraph g(4);  // path a-b-c-d is 2-colorable
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    Allocation a = colorGraph(g, 2, uniform(4), allActive(4));
    ASSERT_TRUE(a.spilled.empty());
    ASSERT_TRUE(verifyColoring(g, a));
    for (int c : a.color) ASSERT_TRUE(c >= 0 && c < 2);
}

TEST(cliqueOfKPlusOneSpillsExactlyOne) {
    InterferenceGraph g = clique(5);
    Allocation a = colorGraph(g, 4, uniform(5), allActive(5));
    ASSERT_EQ(a.spilled.size(), size_t(1));
    ASSERT_TRUE(verifyColoring(g, a));
}

TEST(optimisticCandidateStillGetsColored) {
    InterferenceGraph g(4);  // 4-cycle, k = 2: every degree is 2 >= k, yet the graph is 2-colorable
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 0);
    Allocation a = colorGraph(g, 2, uniform(4), allActive(4));
    ASSERT_EQ(a.candidates.size(), size_t(1));
    ASSERT_TRUE(a.spilled.empty());
    ASSERT_TRUE(verifyColoring(g, a));
    for (int c : a.color) ASSERT_TRUE(c >= 0);
}

TEST(cheapestNodeIsTheSpillVictim) {
    InterferenceGraph g = clique(5);
    std::vector<double> cost = {100, 100, 100, 1, 100};
    Allocation a = colorGraph(g, 4, cost, allActive(5));
    ASSERT_EQ(a.spilled, (std::set<VReg>{3}));
}

TEST(infiniteCostNodeIsNotChosenWhenAlternativeExists) {
    InterferenceGraph g = clique(5);
    std::vector<double> cost = {std::numeric_limits<double>::infinity(), 5, 5, 5, 5};
    Allocation a = colorGraph(g, 4, cost, allActive(5));
    ASSERT_EQ(a.spilled.size(), size_t(1));
    ASSERT_EQ(a.spilled.count(0), size_t(0));
}

TEST(inactiveNodesStayUncoloredAndUnspilled) {
    InterferenceGraph g(3);
    g.addEdge(0, 1);
    std::vector<bool> active = {true, true, false};
    Allocation a = colorGraph(g, 2, uniform(3), active);
    ASSERT_EQ(a.color[2], -1);
    ASSERT_TRUE(a.spilled.empty());
}

TEST(verifyColoringCatchesConflicts) {
    InterferenceGraph g(2);
    g.addEdge(0, 1);
    Allocation bad;
    bad.k = 2;
    bad.color = {0, 0};
    ASSERT_FALSE(verifyColoring(g, bad));
    bad.color = {0, 1};
    ASSERT_TRUE(verifyColoring(g, bad));
    bad.color = {0, 2};  // out of range
    ASSERT_FALSE(verifyColoring(g, bad));
}

TEST(allocateStraightLineNeedsNoSpills) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/straight_line.tac");
    StaticCost sc;
    AllocResult r = allocate(cfg, {4, true, &sc, 10});
    ASSERT_TRUE(r.spillVictims.empty());
    ASSERT_EQ(r.rounds, 1);
    ASSERT_TRUE(verifyColoring(r.ig, r.alloc));
}

TEST(allocateHighPressureLoopSpillsAndKeepsSemantics) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/loop_high_pressure.tac");
    StaticCost sc;
    AllocResult r = allocate(cfg, {4, true, &sc, 10});
    ASSERT_TRUE(!r.spillVictims.empty());
    ASSERT_TRUE(r.rounds >= 2);
    ASSERT_TRUE(verifyColoring(r.ig, r.alloc));
    ASSERT_TRUE(r.registersUsed <= 4);
    ASSERT_EQ(runProgram(r.cfg).printed, runProgram(cfg).printed);
}

TEST(everySampleAllocatesForEveryK) {
    for (const char* n : SAMPLE_NAMES) {
        CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/" + n);
        RunResult ref = runProgram(cfg);
        StaticCost sc;
        for (int k = 3; k <= 8; ++k) {
            AllocResult r = allocate(cfg, {k, true, &sc, 10});
            ASSERT_TRUE(verifyColoring(r.ig, r.alloc));
            ASSERT_TRUE(r.registersUsed <= k);
            RunResult got = runProgram(r.cfg);
            ASSERT_EQ(got.printed, ref.printed);
            ASSERT_EQ(got.retValue, ref.retValue);
        }
    }
}

TEST(spillVictimsAreOriginalVirtualRegisters) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/loop_high_pressure.tac");
    StaticCost sc;
    AllocResult r = allocate(cfg, {3, true, &sc, 10});
    for (VReg v : r.spillVictims) ASSERT_TRUE(v < cfg.numVRegs());
    for (VReg t : r.spill.noSpill) ASSERT_TRUE(t >= cfg.numVRegs());
}

TEST(roundCapIsEnforced) {
    CFG cfg = parseTACFile(std::string(SAMPLES_DIR) + "/loop_high_pressure.tac");
    StaticCost sc;
    ASSERT_THROW(allocate(cfg, {4, true, &sc, 1}), std::runtime_error);
}

TEST(missingCostModelIsRejected) {
    CFG cfg = parseTAC("x = 1\nret x\n");
    ASSERT_THROW(allocate(cfg, {4, true, nullptr, 10}), std::invalid_argument);
}

TEST_MAIN()
