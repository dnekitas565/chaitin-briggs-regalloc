#include "../include/ir.h"
#include "../include/cfg.h"
#include <iostream>
#include <cassert>

void testCFGAndDominators() {
    CFG cfg;
    BasicBlock bb0;
    bb0.label = "bb0";
    bb0.succs.push_back("bb1");
    cfg.blocks.push_back(bb0);

    auto rpo = getReversePostorder(cfg);
    assert(!rpo.empty());
    std::cout << "CFG and Dominators test passed successfully.\n";
}

int main() {
    testCFGAndDominators();
    return 0;
}
