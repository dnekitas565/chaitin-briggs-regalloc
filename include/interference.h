// Owner: B
#pragma once

#include "ir.h"

#include <string>

/// Build the interference graph from a CFG whose liveness has been computed.
/// For a move d = s, s gets no edge to d; the pair is recorded as move-related.
InterferenceGraph buildInterference(const CFG& cfg);
std::string dumpIG(const InterferenceGraph& ig, const CFG& cfg);
/// Plain DOT text (no Graphviz needed to produce it).
std::string igToDot(const InterferenceGraph& ig, const CFG& cfg);
