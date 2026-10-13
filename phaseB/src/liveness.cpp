#include "../include/liveness.h"

void computeLiveness(CFG& cfg) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& bb : cfg.blocks) {
            std::unordered_set<std::string> live = bb.liveOut;
            for (auto it = bb.instrs.rbegin(); it != bb.instrs.rend(); ++it) {
                for (const auto& src : it->srcs) live.insert(src);
                if (!it->dest.empty()) live.erase(it->dest);
            }
            if (live != bb.liveIn) {
                bb.liveIn = live;
                changed = true;
            }
        }
    }
}
