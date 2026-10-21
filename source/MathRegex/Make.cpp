#include "MathRegex/Make.h"
namespace mreg {

State *make_sink(Automata &o) {
    if (o.terminal().size() != 1 || !o.terminal()[0]->edges().empty()) {
        auto end = o.add_state();
        while (o.terminal().size()) {
            auto cur_end = o.terminal().back();
            cur_end->add_edge(ZERO_CHAR, end);
            cur_end->set_terminal(false);
        }
        end->set_terminal(true);
        return end;
    } else {
        return o.terminal()[0];
    }
}

State *make_source(Automata &a) {
    if (!a.initial()->inv_edges().empty()) {
        auto start = a.add_state();
        start->add_edge(ZERO_CHAR, a.initial());
        a.set_initial(start);
        return start;
    } else {
        return a.initial();
    }
}

State* merge_states(State *a, State *b) {
    if (a == b) return a;
    if (a->edges().size() + a->inv_edges().size() 
        < b->edges().size() + b->inv_edges().size())
        std::swap(a, b);
    while (b->edges().size()) {
        Edge e = b->edges().back();
        b->erase_edge(e.ch(), e.to());
        a->add_edge(e.ch(), e.to());
    }
    while (b->inv_edges().size()) {
        Edge e = b->inv_edges().back();
        e.from()->erase_edge(e.ch(), b);
        e.from()->add_edge(e.ch(), a);
    }
    if (b->is_terminal())
        a->set_terminal(true);
    if (a->label().empty() && !b->label().empty())
        a->set_label(b->label());
    if (b->automata()->initial() == b)
        b->automata()->set_initial(a);
    b->remove();
    return a;
}

Automata make_optional(Automata &&a) {
    auto end = make_sink(a);
    auto start = make_source(a);
    start->add_edge(ZERO_CHAR, end);
    return std::move(a);
}

Automata make_loop(Automata &&a) {
    auto end = make_sink(a);
    end->add_edge(ZERO_CHAR, a.initial());
    return std::move(a);
}

Automata make_chain(Automata &&a, Automata &&b) {
    auto a_end = make_sink(a);
    auto b_start = make_source(b);
    a.dissolve(std::move(b));
    a_end->set_terminal(false);
    merge_states(a_end, b_start);
    return std::move(a);
}

Automata make_repeat(Automata &&a, size_t min, size_t max) {
    Automata res;
    auto initial = res.add_state(true);
    res.set_initial(initial);
    for (size_t i = 0; i < max; ++i) {
        auto item = i == max-1 ? std::move(a) : a.clone();
        if (i >= min)
            item = make_optional(std::move(item));
        res = make_chain(std::move(res), std::move(item));
    }
    return res;
}

Automata make_or(Automata &&a, Automata &&b) {
    auto a_start = make_source(a), b_start = b.initial();
    a.dissolve(std::move(b));
    a_start->add_edge(ZERO_CHAR, b_start);
    return std::move(a);
}

};
