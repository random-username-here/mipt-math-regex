///
/// @file
/// @brief The finite automata data structure
///
#pragma once
#include "MathRegex/Utils.h"
#include <cstddef>
#include <memory>

namespace mreg {

class Edge;
class State;
class Automata;

///
/// @brief An edge from one state to another.
/// May be an ε-edge, in that case `ch() == 0`.
///
class Edge {
    Rune m_ch;
    State *m_from, *m_to;
public:

    Edge(Rune ch, State *from, State *to) 
        :m_ch(ch), m_from(from), m_to(to) {}

    Rune ch() const { return m_ch; }
    const State *to() const { return m_to; }
    State *to() { return m_to; }
    const State *from() const { return m_from; }
    State *from() { return m_from; }

    bool is_eps() const { return m_ch == 0; }
};

///
/// @brief A comparator to order edges in flat sets
/// Sorts first by character, then by `.to` pointer, then by `.from` pointer.
///
struct EdgeComparator {
    bool operator()(const Edge &a, const Edge &b) const;
};

///
/// @brief One of the automata's states.
/// States should be created by `Automata::add_state()`.
///
class State {
    friend class Automata;

    Automata *m_automata;
    FlatMultiSet<Edge, 8, EdgeComparator> m_edges;
    FlatMultiSet<Edge, 8, EdgeComparator> m_inverse;
    bool m_is_terminal = false;
    size_t m_duplicate_count = 0;
    size_t m_eps_count = 0;
    std::string m_label;

public:

    void set_label(View label) { m_label = label; }
    View label() const { return m_label; }

    Automata *automata() { return m_automata; }
    const Automata *automata() const { return m_automata; }

    /// If next state is clearly defined
    bool is_determined() const { return m_duplicate_count == 0 && m_eps_count == 0; }
    // If this state is terminal
    bool is_terminal() const { return m_is_terminal; }
    /// Number of epsilon edges out of here
    size_t eps_count() const { return m_eps_count; }
    size_t duplicate_count() const { return m_duplicate_count; }

    /// Array of all the edges for that
    ArrayRef<const Edge> edges() const { return m_edges.container; }
    ArrayRef<Edge> edges() { return m_edges.container; }

    /// Obtain all edges for given character
    /// (this doesn't include eps-edges and edges from nodes acessible via them)
    ArrayRef<const Edge> for_char(Rune ch) const;
    ArrayRef<Edge> for_char(Rune ch);

    /// Inverse edges. Those are also sorted the same way as usual.
    ArrayRef<const Edge> inv_edges() const { return m_inverse.container; }
    ArrayRef<Edge> inv_edges() { return m_inverse.container; }

    /// @brief Add an edge.
    /// Will invalidate iterators and views to edge list.
    void add_edge(Rune ch, State *to);

    /// @brief Remove an edge.
    /// Will invalidate iterators and views to edge list.
    void erase_edge(Rune ch, State *to);

    /// Make this state a terminal state.
    void set_terminal(bool terminal);

    /// Drop this state completely, removing incoming/outgoing edges.
    void remove();
    
    /// Create new state acessible via that character.
    State *add_next(Rune ch);

    /// Create a chain of edges, acessibly by going by a given chain.
    /// Returns the last state.
    State *add_chain(View string);
};

///
/// @brief The finite automata itself.
///
/// Various utility functions (like printing)
/// are separated into `Algorithms.h`.
///
class Automata {
    friend class State; // for dupcount
    SmallVec<std::unique_ptr<State>, 32> m_states;
    SmallVec<State*, 16> m_terminal;
    size_t m_duplicate_count = 0;
    size_t m_eps_count = 0;
    State *m_initial = nullptr;
public:
    Automata();

    Automata(const Automata &o) = delete;
    Automata &operator=(const Automata &o) = delete;
    Automata(Automata &&o) { operator=(std::move(o)); }
    Automata &operator=(Automata &&o);
    ~Automata() = default; // we don't need to update ptrs on death

    size_t eps_count() const { return m_eps_count; }
    size_t duplicate_count() const { return m_duplicate_count; }
    bool is_determined() const { return m_duplicate_count == 0 && m_eps_count == 0; }

    /// Initial state
    State *initial() { return m_initial; }
    const State *initial() const { return m_initial; }
    void set_initial(State *state) { m_initial = state; }

    /// Terminal states
    ArrayRef<const State* const> terminal() const { return m_terminal; }
    ArrayRef<State*> terminal() { return m_terminal; }

    /// All states
    ArrayRef<const std::unique_ptr<State>> states() const { return m_states; }

    /// @brief Create a new state.
    /// This won't create any edges.
    State *add_state(bool terminal = false);

    /// @brief Merge that automata into this one, not adding new edges.
    /// New initial state is from this one. Dissolved automata's initial
    /// state becomes a regular state. Terminality of it's terminal states
    /// is preserved.
    void dissolve(Automata &&o);

    /// @brief Make a full copy
    Automata clone() const;
};

};
