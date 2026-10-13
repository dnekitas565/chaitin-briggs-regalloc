#include "../include/costmodel.h"
#include <cmath>

double calculateStaticCost(int loopDepth) {
    return std::pow(10.0, static_cast<double>(loopDepth));
}

double calculateProfileCost(int blockExecutions, bool executed) {
    if (!executed) return 0.5;
    return static_cast<double>(blockExecutions);
}
