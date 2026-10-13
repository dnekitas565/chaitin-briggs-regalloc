#ifndef IR_H
#define IR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct Instr {
    std::string op;
    std::string dest;
    std::vector<std::string> srcs;
    bool isMove = false;
};

struct BasicBlock {
    std::string label;
    std::vector<Instr> instrs;
    std::vector<std::string> succs;
    std::vector<std::string> preds;
    std::unordered_set<std::string> liveIn;
    std::unordered_set<std::string> liveOut;
};

struct CFG {
    std::vector<BasicBlock> blocks;
};

#endif // IR_H
