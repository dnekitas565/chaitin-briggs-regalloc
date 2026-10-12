// Owner: B
#pragma once

#include "ir.h"

#include <map>
#include <set>
#include <vector>

/// State that persists across allocation rounds.
struct SpillState {
    std::map<VReg, int> slotOf;  // original vreg -> stable slot number
    int nextSlot = 0;
    std::vector<VReg> origin;    // fresh vreg -> original vreg (originals map to themselves)
    std::set<VReg> noSpill;      // fresh short-lived vregs: infinite spill cost
    VReg originOf(VReg v) const {
        return static_cast<size_t>(v) < origin.size() ? origin[static_cast<size_t>(v)] : v;
    }
};

/// Rewrite the program: reload into a fresh vreg before each use, spill after each def.
void insertSpillCode(CFG& cfg, const std::set<VReg>& spilled, SpillState& state);
