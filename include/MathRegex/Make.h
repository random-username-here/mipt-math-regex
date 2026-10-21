///
/// @file
/// @brief Creating new finite automata
///

#include "MathRegex/Automata.h"
namespace mreg {

///
/// @brief Make automata terminate in one non-exiting state.
///
/// Addes ε-edges from final states to new one.
/// If there is exactly one final state in given
/// automata, AND it has no outgoing edges, then it
/// will be returned instead. If there are no final
/// states, an unreachable one will be created.
/// 
State *make_sink(Automata &a);

///
/// @brief Make automata start in one non-reentrant state.
///
/// Uses existing start if it has no incoming edges.
///
State *make_source(Automata &a);

///
/// @brief Merge one state and another, moving incoming and outgoing edges
/// Which one gets reused is unknown.
///
State* merge_states(State *a, State *b);

/// Generate `A?`
Automata make_optional(Automata &&a);

/// Generate `A*`
Automata make_loop(Automata &&a);

/// Generate `AB`
Automata make_chain(Automata &&a, Automata &&b);

/// Generate `A{min,max}`
Automata make_repeat(Automata &&a, size_t min, size_t max);

/// Generate `A|B`
Automata make_or(Automata &&a, Automata &&b);

};
