///==========================================================================///
/// MathRegex -- The finite automata data structure
///==========================================================================///
#pragma once
#include "MathRegex/Utils.h"
#include <cstddef>
#include <memory>

namespace mreg {

class Edge;
class State;
class Automata;

///
/// An edge from one state to another.
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
/// One of the automata's states. 
///
/// Has a sorted vector of edges to next states,
/// so it is fairly small. This makes adding a lot of
/// edges a bit more expensive, but we aren't making
/// hundreds of them usually.
///
/// States should be created by `Automata::add_state()`.
///
/// TODO: replace custom sorting with newly brought in Flat** structs
///
class State {
    friend class Automata;
    Automata *m_automata;
    SmallVec<Edge, 8> m_edges;
    SmallVec<Edge, 8> m_inv_edges;
    size_t m_duplicate_count = 0;
    bool m_is_terminal = false;
    size_t m_eps_count = 0;
public:

    Automata *automata() { return m_automata; }
    const Automata *automata() const { return m_automata; }

    bool is_determined() const { return m_duplicate_count == 0 && m_eps_count == 0; }
    bool is_terminal() const { return m_is_terminal; }
    size_t eps_count() const { return m_eps_count; }

    ArrayRef<const Edge> edges() const { return m_edges; }
    ArrayRef<Edge> edges() { return m_edges; }

    const Edge *first_for_ch(Rune ch) const; // can be incremented to get next
    Edge *first_for_ch(Rune ch);

    // those are also sorted by char, but just for ease of search
    ArrayRef<const Edge> inv_edges() const { return m_inv_edges; }
    ArrayRef<Edge> inv_edges() { return m_inv_edges; }

    void add_edge(Rune ch, State *to);
    void erase_edge(Rune ch, State *to);
    void set_terminal(bool terminal);
    void remove();
    
    State *add_next(Rune ch);
    State *add_chain(View string);
};

///
/// The finite automata itself.
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
    Automata(std::nullptr_t); // make without initial

    Automata(const Automata &o) = delete;
    Automata &operator=(const Automata &o) = delete;
    Automata(Automata &&o) { operator=(std::move(o)); }
    Automata &operator=(Automata &&o);
    ~Automata() = default; // we don't need to update ptrs on death

    bool is_determined() const { return m_duplicate_count == 0; }

    State *initial() { return m_initial; }
    void set_initial(State *state) { m_initial = state; } 

    const State *initial() const { return m_initial; }
    ArrayRef<const State* const> terminal() const { return m_terminal; }
    ArrayRef<State*> terminal() { return m_terminal; }

    ArrayRef<const std::unique_ptr<State>> states() const { return m_states; }

    State *add_state(bool terminal = false);

    /// Merge that automata into this one, not adding new edges
    /// Initial state is left from this one.
    /// Meanwhile, terminal nodes from dissolved one
    /// are terminal in this one.
    void dissolve(Automata &&o);

    Automata clone() const;
};

};
