///
/// @file
/// @brief Finite automata algorithms
///
#pragma once
#include "MathRegex/Automata.h"

namespace mreg {

/// Dump as graphviz to given stream
void dump_gviz(std::ostream &os, const Automata &aut);

/// Simplify automata a bit, for it to look cleaner when drawing
/// Removes duplicate epsilons, some unneeded epsilons (but not all).
void comb_eps(Automata &aut);

/// Remove dead ends (pieces of automata without terminal edges)
void remove_dead_ends(Automata &aut);

/// Generate determinized automata
Automata determinize(const Automata &aut);

/// Suffix-minimize determinate automata
Automata minimize(const Automata &aut);

/// Check if the determined automata matches a word
bool matches(const Automata &aut, View word);

/// Parse a simple finite automata format
Expected<Automata, SyntaxError> parse_fsa(View source);

/// Dump back into that format
void dump_fsa(std::ostream &os, const Automata &aut);

};
