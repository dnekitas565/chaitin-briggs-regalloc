// Owner: B
#include "spillcode.h"

#include <algorithm>
#include <map>

void insertSpillCode(CFG& cfg, const std::set<VReg>& spilled, SpillState& state) {
    // Keep origin[] total: originals map to themselves.
    while (state.origin.size() < cfg.vregNames.size()) state.origin.push_back(static_cast<VReg>(state.origin.size()));

    auto slotFor = [&](VReg v) {
        VReg o = state.originOf(v);
        auto it = state.slotOf.find(o);
        if (it == state.slotOf.end()) it = state.slotOf.emplace(o, state.nextSlot++).first;
        return it->second;
    };
    auto fresh = [&](VReg v) {
        VReg t = cfg.addVReg(cfg.vregNames[static_cast<size_t>(state.originOf(v))] + "_t" + std::to_string(cfg.numVRegs()));
        state.origin.push_back(state.originOf(v));
        state.noSpill.insert(t);
        return t;
    };

    for (BasicBlock& b : cfg.blocks) {
        std::vector<Instr> out;
        out.reserve(b.code.size());
        for (Instr in : b.code) {
            std::map<VReg, VReg> useTemp;  // one reload per spilled vreg per instruction
            for (VReg& u : in.uses) {
                if (!spilled.count(u)) continue;
                auto it = useTemp.find(u);
                if (it == useTemp.end()) {
                    VReg t = fresh(u);
                    Instr r;
                    r.op = Op::Reload;
                    r.defs = {t};
                    r.slot = slotFor(u);
                    r.line = in.line;
                    out.push_back(r);
                    it = useTemp.emplace(u, t).first;
                }
                u = it->second;
            }
            std::vector<Instr> after;
            for (VReg& d : in.defs) {
                if (!spilled.count(d)) continue;
                VReg t = fresh(d);
                Instr s;
                s.op = Op::Spill;
                s.uses = {t};
                s.slot = slotFor(d);
                s.line = in.line;
                after.push_back(s);
                d = t;
            }
            out.push_back(std::move(in));
            for (Instr& s : after) out.push_back(std::move(s));
        }
        b.code = std::move(out);
    }
}
