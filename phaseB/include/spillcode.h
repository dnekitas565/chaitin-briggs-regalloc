#ifndef SPILLCODE_H
#define SPILLCODE_H

#include "ir.h"

void insertSpillCode(CFG& cfg, const std::string& spilledReg);

#endif // SPILLCODE_H
