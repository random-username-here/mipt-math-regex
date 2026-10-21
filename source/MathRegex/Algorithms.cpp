#include "MathRegex/Algorithms.h"
#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
#include <ostream>
#include <unordered_set>
namespace mreg {

void dump_gviz(std::ostream &os, const Automata &aut) {
    os << "digraph {\n";
    os << "    graph[rankdir=LR,ranksep=0.3,edgesep=0.3]\n";
    os << "    node[shape=circle,margin=0.03,width=0.2,height = 0.1];\n";
    for (const auto &s : aut.states()) {
        if (s->is_terminal())
            os << "    p" << s.get() << "[peripheries=2,label=\"" << s->label() << "\"];\n";
        else
            os << "    p" << s.get() << "[label=\"" << s->label() << "\"];\n";
        for (const auto &e : s->edges()) {
            os << "    p" << s.get() << " -> p" << e.to() << " ";
            if (e.ch() == ZERO_CHAR) 
                os << "[label=\"ε\", style=dashed, color=\"red\"];";
            else
                os << "[label=\"" << e.ch() << "\"];";
            os << "\n";
        }
    }
    os << "    init[label=\"\", style=invis];\n";
    os << "    init -> p" << aut.initial() << ";\n";
    os << "}";
}

void comb_eps(Automata &aut) {
    SmallVec<State*, 16> to_delete;
    for (auto &state : aut.states()) {
        if (state->is_terminal())
            continue;
        if (state->edges().size() != 1)
            continue;
        auto eps = state->edges()[0];
        if (eps.ch() != ZERO_CHAR) 
            continue;
        while (state->inv_edges().size()) {
            auto last = state->inv_edges().back();
            last.from()->erase_edge(last.ch(), last.to());
            last.from()->add_edge(last.ch(), eps.to());
        }
        if (state->automata()->initial() == state.get())
            state->automata()->set_initial(eps.to());
        to_delete.push_back(state.get());
    }
    for (auto i : to_delete)
        i->remove();
}

void remove_dead_ends(Automata &aut) {
    assert(aut.initial());

    // find strongly connected components, drop those without accessible terminals

    // most in-depth first, edges only from later to earlier
    SmallVec<State*> order;
    std::unordered_set<State*> visited;

    std::function<void(State*)> topsort;
    topsort = [&](State* s) {
        visited.insert(s);
        for (auto e : s->edges())
            if (!visited.count(e.to()))
                topsort(e.to());
        order.push_back(s);
    };

    topsort(aut.initial());

    SmallVec<State*> unused;
    for (const auto &s : aut.states())
        if (!visited.count(s.get()))
            unused.push_back(s.get());
    for (auto s : unused)
        s->remove();
    unused.clear();

    size_t cur_comp = 0;
    std::unordered_map<State*, size_t> component;
    SmallVec<bool> have_terminal;

    std::function<void(State*)> mark;
    mark = [&](State* st){
        component[st] = cur_comp;
        have_terminal.back() |= st->is_terminal();
        for (auto e : st->edges()) {
            if (!component.count(e.to()))
                mark(e.to());
            else
                have_terminal.back() |= have_terminal[component[e.to()]];
        }
    };

    for (auto st : order) {
        if (component.count(st)) continue;
        have_terminal.push_back(false);
        mark(st);
        cur_comp++;
    }

    for (auto &st : aut.states())
        if (st.get() != aut.initial() && !have_terminal[component[st.get()]])
            unused.push_back(st.get());
    for (auto s : unused)
        s->remove();
}

bool matches(const Automata &aut, View word) {
    assert(aut.is_determined());
    const State *s = aut.initial();
    for (Rune ch : U8Reader(word)) {
        auto next = s->for_char(ch);
        if (next.empty()) return false;
        assert(next.size() == 1);
        s = next[0].to();
    }
    return s->is_terminal();
}
};
