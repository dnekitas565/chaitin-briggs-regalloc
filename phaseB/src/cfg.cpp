#include "../include/cfg.h"

std::vector<int> getReversePostorder(const CFG& cfg) {
    std::vector<int> rpo;
    for (size_t i = 0; i < cfg.blocks.size(); ++i) {
        rpo.push_back(static_cast<int>(i));
    }
    return rpo;
}
