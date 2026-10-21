#include "MathRegex/Automata.h"
#include "MathRegex/Make.h"
#include <catch2/catch_test_macros.hpp>
using namespace mreg;

static bool l_connects(State *from, State *to, Rune ch) {
    for (auto i : from->edges())
        if (i.to() == to && i.ch() == ch)
            return true;
    return false;
}

TEST_CASE("Automata: construction") {
    //
    //           .<. b
    //      b    \ /
    //       ,-> {b}
    //      /
    // -> (a) -> {c}
    //    / \  c
    //    '<'
    //     a
    Automata usual;
    auto a = usual.add_state();
    usual.set_initial(a);
    a->add_edge('a', a);
    auto b = a->add_next('b');
    b->add_edge('b', b);
    auto c = a->add_next('c');
    b->set_terminal(true);
    c->set_terminal(true);

    //        a       b       c
    // -> (b) -> (pb) -> (pe) -> {e}
    //
    Automata chain;
    auto chain_begin = chain.add_state();
    chain.set_initial(chain_begin);
    auto chain_end = chain_begin->add_chain("abc");
    auto chain_postbegin = chain_begin->edges()[0].to();
    auto chain_preend = chain_end->inv_edges()[0].from();
    chain_end->set_terminal(true);

    //        x
    // -> (s) -> {e}
    //
    Automata one_char;
    auto ch_start = one_char.add_state();
    one_char.set_initial(ch_start);
    auto ch_end = ch_start->add_next('x');
    ch_end->set_terminal(true);

    Automata empty;

    SECTION("Adding a sink") {
        auto usual_sink = make_sink(usual);
        REQUIRE(l_connects(b, usual_sink, ZERO_CHAR));
        REQUIRE(l_connects(c, usual_sink, ZERO_CHAR));
        REQUIRE(usual.terminal().size() == 1);
        REQUIRE(usual.terminal()[0] == usual_sink);

        // reuse
        auto chain_sink = make_sink(chain);
        REQUIRE(chain_sink == chain_end);
    }

    SECTION("Adding a source") {
        auto usual_source = make_source(usual);
        REQUIRE(usual_source != a);
        REQUIRE(l_connects(usual_source, a, ZERO_CHAR));
        REQUIRE(usual.initial() == usual_source);

        auto chain_source = make_source(chain);
        REQUIRE(chain_source == chain_begin);
    }

    SECTION("State merging") {
        // nothing should happen
        auto same_a = merge_states(a, a);
        REQUIRE(a == same_a);

        // very basic test
        chain.dissolve(std::move(usual));
        auto mid = merge_states(chain_end, a); // attach to end
        REQUIRE((mid == chain_end || mid == a)); // some will be reused
        REQUIRE(l_connects(mid, b, 'b'));
        REQUIRE(l_connects(chain_preend, mid, 'c'));
        REQUIRE(mid->is_terminal()); // should preserve chain end terminality
    }

    SECTION("Optional") {
        Automata opt = make_optional(std::move(chain));
        REQUIRE(opt.terminal().size() == 1);
        auto opt_begin = opt.initial(), opt_end = opt.terminal()[0];
        REQUIRE(l_connects(opt_begin, opt_end, ZERO_CHAR));
        REQUIRE(opt_begin == chain_begin);
        REQUIRE(chain_end == opt_end);
    }

    SECTION("Loop") {
        Automata loop = make_loop(std::move(chain));
        REQUIRE(loop.terminal().size() == 1);
        REQUIRE(loop.initial() == chain_begin);
        REQUIRE(loop.terminal()[0] == chain_end);
        REQUIRE(l_connects(chain_end, chain_begin, ZERO_CHAR));
    }

    SECTION("Chain") {
        // should cap off the first
        Automata bigchain = make_chain(std::move(usual), std::move(chain));
        REQUIRE(bigchain.terminal().size() == 1);
        REQUIRE(bigchain.terminal()[0] == chain_end);
        REQUIRE(bigchain.initial() == a);
        auto cap = c->edges()[0].to(); // capping and chain_begin merged
        REQUIRE(l_connects(b, cap, ZERO_CHAR));
        REQUIRE(l_connects(cap, chain_postbegin, 'a'));
    }

    SECTION("Repeat") {
        // checking nodes manually is too tedious
        // repeat is made like x x x x ... x x? x? x? ... x?
        Automata repeated = make_repeat(std::move(one_char), 2, 4);
        State *s = repeated.initial();
        for (int i = 0; i < 4; ++i) {
            REQUIRE(s->for_char('x').size() == 1);
            auto next = s->for_char('x')[0].to();
            if (i >= 2) {
                REQUIRE(s->for_char(ZERO_CHAR).size() == 1);
                REQUIRE(s->for_char(ZERO_CHAR)[0].to() == next);
            }
            s = next;
        }
    }
    SECTION("Or") {
        Automata options = make_or(std::move(usual), std::move(chain));
        REQUIRE(c->is_terminal()); // it shoudn't touch ends
        REQUIRE(b->is_terminal());
        REQUIRE(chain_end->is_terminal());
        auto start = options.initial();
        REQUIRE(l_connects(start, a, ZERO_CHAR));
        REQUIRE(l_connects(start, chain_begin, ZERO_CHAR));
    }
}