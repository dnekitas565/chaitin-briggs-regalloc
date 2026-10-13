#include "../include/ir.h"
#include "../include/interference.h"
#include <iostream>
#include <cassert>

void testInterferenceAndMoveException() {
    CFG cfg;
    BasicBlock bb;
    bb.label = "entry";
    
    Instr moveInstr;
    moveInstr.op = "MOV";
    moveInstr.dest = "t1";
    moveInstr.srcs = {"t2"};
    moveInstr.isMove = true;
    bb.instrs.push_back(moveInstr);
    cfg.blocks.push_back(bb);

    InterferenceGraph ig = buildInterferenceGraph(cfg);
    assert(ig.adjList["t1"].find("t2") == ig.adjList["t1"].end());
    std::cout << "Interference graph and move exception test passed successfully.\n";
}

int main() {
    testInterferenceAndMoveException();
    return 0;
}
