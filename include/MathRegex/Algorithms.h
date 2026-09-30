///==========================================================================///
/// MathRegex -- Finite automata algorithms
///==========================================================================///
#pragma once
#include "MathRegex/Automata.h"

namespace mreg {

/// Dump as graphviz
void gviz_dump(std::ostream &os, const Automata &aut);

/// Simplify automata a bit, for it to look cleaner when drawing
void comb_eps(Automata &aut);

/// Generate determinized automata
Automata determinize(const Automata &aut);

/// Minimize determinate
Automata minimize(const Automata &aut);

};
