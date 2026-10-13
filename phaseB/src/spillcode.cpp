#include "../include/spillcode.h"

void insertSpillCode(CFG& cfg, const std::string& spilledReg) {
    for (auto& bb : cfg.blocks) {
        std::vector<Instr> updatedInstrs;
        for (const auto& instr : bb.instrs) {
            updatedInstrs.push_back(instr);
        }
        bb.instrs = updatedInstrs;
    }
}
