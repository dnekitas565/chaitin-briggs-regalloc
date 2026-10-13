#include "../include/costmodel.h"
#include <iostream>
#include <cassert>
#include <cmath>

void testCostModels() {
    double staticCost = calculateStaticCost(2);
    assert(std::abs(staticCost - 100.0) < 1e-5);

    double profileCostUnexecuted = calculateProfileCost(50, false);
    assert(std::abs(profileCostUnexecuted - 0.5) < 1e-5);

    std::cout << "Cost model unit tests passed successfully.\n";
}

int main() {
    testCostModels();
    return 0;
}
