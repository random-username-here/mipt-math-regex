#include "MathRegex/Automata.h"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include "MathRegex/Utils.h"

namespace mreg {

//============================================================================//

bool EdgeComparator::operator()(const Edge &a, const Edge &b) const {
    if (a.ch() < b.ch()) return true;
    if (a.ch() > b.ch()) return false;
    if (a.to() < b.to()) return true;
    if (a.to() > b.to()) return false;
    return a.from() < b.from();
}

//============================================================================//

ArrayRef<const Edge> State::for_char(Rune ch) const {
    auto start = m_edges.lower_bound(Edge(ch, 0, 0)).underlying;
    auto end = m_edges.lower_bound(Edge(ch, (State*) UINTPTR_MAX, (State*) UINTPTR_MAX)).underlying;
    return ArrayRef<const Edge>(start, end - start);
}

ArrayRef<Edge> State::for_char(Rune ch) {
    auto start = m_edges.lower_bound(Edge(ch, 0, 0)).underlying;
    auto end = m_edges.lower_bound(Edge(ch, (State*) UINTPTR_MAX, (State*) UINTPTR_MAX)).underlying;
    return ArrayRef<Edge>(start, end - start);
}

void State::add_edge(Rune ch, State *to) {
    auto edge = Edge(ch, this, to);
    auto it = m_edges.insert(edge).first;
    to->m_inverse.insert(edge);

    if (ch == ZERO_CHAR) {
        ++m_eps_count;
        ++automata()->m_eps_count;
    }
    if ((it != m_edges.begin() && std::prev(it)->ch() == ch)
        || (std::next(it) != m_edges.end() && std::next(it)->ch() == ch)) {
        ++m_duplicate_count;
        ++automata()->m_duplicate_count;
    }
}

void State::erase_edge(Rune ch, State *to) {
    auto edge = Edge(ch, this, to);
    auto it = m_edges.find(edge);
    assert(it != m_edges.end());
    assert(it->ch() == ch || it->to() == to); // no deleting wrong edge

    if (ch == ZERO_CHAR) {
        --m_eps_count;
        --automata()->m_eps_count;
    }
    if ((it+1 != m_edges.end() && (it+1)->ch() == ch)
        || (it != m_edges.begin() && (it-1)->ch() == ch)) {
        --m_duplicate_count;
        --automata()->m_duplicate_count;
    }

    m_edges.erase(it);

    auto rit = to->m_inverse.find(edge);
    assert(rit->from() == this);
    assert(rit->to() == to);
    to->m_inverse.erase(rit);
}

State *State::add_next(Rune ch) {
    auto st = automata()->add_state();
    add_edge(ch, st);
    return st;
}

State *State::add_chain(View str) {
    State *st = this;
    for (auto ch : U8Reader(str)) {
        st = st->add_next(ch);
    }
    return st;
}

void State::set_terminal(bool terminal) {
    if (terminal == m_is_terminal) return;
    if (!terminal) {
        automata()->m_terminal.erase(std::find(
            automata()->m_terminal.begin(), automata()->m_terminal.end(),
            this
        ));
    } else {
        automata()->m_terminal.push_back(this);
    }
    m_is_terminal = terminal;
}

void State::remove() {
    if (is_terminal())
        set_terminal(false);
    if (automata()->initial() == this)
        automata()->set_initial(nullptr);
    while (!m_inverse.empty()) {
        auto e = m_inverse.container.back();
        e.from()->erase_edge(e.ch(), this);
    }
    automata()->m_duplicate_count -= m_duplicate_count;
    automata()->m_eps_count -= m_eps_count;
    auto it = std::find_if(
        automata()->m_states.begin(), automata()->m_states.end(),
        [this](const auto &up){
            return up.get() == this;
        }
    );
    assert(it->get() == this);
    automata()->m_states.erase(it);
}

Automata::Automata() {}

Automata &Automata::operator=(Automata &&o) {
    m_states = std::exchange(o.m_states, {});
    m_terminal = std::exchange(o.m_terminal, {});
    m_duplicate_count = std::exchange(o.m_duplicate_count, 0);
    m_eps_count = std::exchange(o.m_eps_count, 0);
    m_initial = std::exchange(o.m_initial, nullptr);
    for (auto &i : m_states)
        i->m_automata = this;
    return *this;
}

State *Automata::add_state(bool terminal) {
    m_states.push_back(std::make_unique<State>());
    auto s = m_states.back().get();
    s->m_automata = this;
    s->set_terminal(terminal);
    return s;
}


void Automata::dissolve(Automata &&o) {
    m_states.reserve(m_states.size() + o.m_states.size());
    for (auto &i : o.m_states) {
        i->m_automata = this;
        m_states.push_back(std::move(i));
    }
    o.m_states.clear();
    m_terminal.insert(m_terminal.end(), o.m_terminal.begin(), o.m_terminal.end());
    o.m_terminal.clear();
    m_duplicate_count += o.m_duplicate_count;
    m_eps_count += o.m_eps_count;
    o.m_duplicate_count = 0;
    o.m_eps_count = 0;
    o.m_initial = nullptr; // no reusing it
}

Automata Automata::clone() const {

    Automata copy;
    copy.m_states.reserve(m_states.size());
    copy.m_terminal.reserve(m_terminal.size());
    
    std::unordered_map<const State*, State*> state_remap;
    for (const auto &s : m_states)
        state_remap[s.get()] = copy.add_state();
    
    for (const auto &s : m_states) {
        State *rs = state_remap[s.get()];
        for (const auto &e : s->edges())
            rs->add_edge(e.ch(), state_remap[e.to()]);
        if (s->is_terminal())
            rs->set_terminal(true);
    }
    copy.set_initial(state_remap[initial()]);

    return copy;
}



};
