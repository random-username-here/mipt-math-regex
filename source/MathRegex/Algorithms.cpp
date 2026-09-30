#include "MathRegex/Algorithms.h"
#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
#include <iostream>
#include <ostream>
namespace mreg {

void gviz_dump(std::ostream &os, const Automata &aut) {
    os << "digraph {\n";
    os << "    graph[rankdir=LR,ranksep=0.3,edgesep=0.3]\n";
    os << "    node[shape=circle,margin=0.03,width=0.2,height = 0.1];\n";
    for (const auto &s : aut.states()) {
        if (s->is_terminal())
            os << "    p" << s.get() << "[peripheries=2,label=\"\"];\n";
        else
            os << "    p" << s.get() << "[label=\"\"];\n";
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

};
