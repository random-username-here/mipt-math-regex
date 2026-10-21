#include "MathRegex/Automata.h"
#include <catch2/catch_test_macros.hpp>
using namespace mreg;

TEST_CASE("Automata: basic operations") {

    Automata aut;

    // must be uninitialized
    REQUIRE(aut.states().empty());
    REQUIRE(aut.terminal().empty());
    REQUIRE(aut.initial() == nullptr);
    REQUIRE(aut.is_determined());

    // now add start
    State *start = aut.add_state();
    REQUIRE(!start->is_terminal());
    REQUIRE(start->edges().empty());

    // make it initial
    aut.set_initial(start);
    REQUIRE(aut.initial() == start);
    REQUIRE(aut.is_determined());

    // add a chain
    State *end = start->add_chain("abc");
    end->set_terminal(true);
    REQUIRE(aut.is_determined());
    REQUIRE(end->is_terminal());
    REQUIRE(aut.terminal().size() == 1);
    REQUIRE(aut.terminal()[0] == end);
    REQUIRE(start->for_char('a').size() == 1);

    // add an epsilon edge
    State *followup = start->add_next(ZERO_CHAR);
    followup->add_edge('a', followup); // loop
    followup->add_edge(ZERO_CHAR, end);
    REQUIRE(aut.eps_count() == 2);
    REQUIRE(start->eps_count() == 1);
    REQUIRE_FALSE(aut.is_determined());

    // and a duplicate edge
    State *another = start->add_next('a');
    REQUIRE(start->duplicate_count() == 1);
    REQUIRE(start->for_char('a').size() == 2);

    // and remove it
    another->remove();
    REQUIRE(start->duplicate_count() == 0);

    // be sure what props stay on clone
    Automata clone = aut.clone();
    REQUIRE(clone.states().size() == aut.states().size());
    REQUIRE(clone.terminal().size() == aut.terminal().size());
    REQUIRE(clone.initial() != nullptr);
    REQUIRE(clone.is_determined() == aut.is_determined());
    REQUIRE(clone.eps_count() == aut.eps_count());
    REQUIRE(clone.duplicate_count() == aut.duplicate_count());

    // move automata - could be a source of problems, because we keep ptr to it
    Automata moved = std::move(aut);
    REQUIRE(moved.initial()->automata() == &moved);
    // and others for good measures
    REQUIRE(moved.states().size() == clone.states().size());
    REQUIRE(moved.terminal().size() == clone.terminal().size());
    REQUIRE(moved.is_determined() == clone.is_determined());
    REQUIRE(moved.eps_count() == clone.eps_count());
    REQUIRE(moved.duplicate_count() == clone.duplicate_count());

    // drop a terminal
    clone.terminal()[0]->remove();
    REQUIRE(clone.terminal().empty());

    // and the initial one too
    clone.initial()->remove();
    REQUIRE(!clone.initial());

    // add cv-qualified for_char(), it is the exact same as non-cv,
    // but codecov doesnt know that
    static_cast<const State*>(moved.initial())->for_char('a');

    // dissolving
    Automata small;
    auto st = small.add_state();
    small.set_initial(st);
    auto small_end = st->add_next('f');
    small_end->set_terminal(true);

    moved.dissolve(std::move(small));
    REQUIRE(std::find(moved.terminal().begin(), moved.terminal().end(), small_end)
            != moved.terminal().end());
    REQUIRE(moved.initial() != st);
}