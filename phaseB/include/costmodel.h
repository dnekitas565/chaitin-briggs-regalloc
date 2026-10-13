#ifndef COSTMODEL_H
#define COSTMODEL_H

#include <string>

double calculateStaticCost(int loopDepth);
double calculateProfileCost(int blockExecutions, bool executed);

#endif // COSTMODEL_H
