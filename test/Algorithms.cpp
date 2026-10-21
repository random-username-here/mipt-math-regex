#include "MathRegex/Algorithms.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sstream>
using namespace mreg;
using namespace Catch::Matchers;

TEST_CASE("Algorithms: fsa parsing edge cases") {
    // parsing & writting is heavily used in other tests, so here we only do edge
    // cases (e.g. errors)
     
    struct ErrorTest {
        View source;
        size_t pos;
        View message;
    };
    std::vector<ErrorTest> tests = {
        // bad cases
        { "> a - qwerty > b", 6, "Multi-character edges are not supported" },
        { "> a - x", 6, "Edge without an end" },
        { "> ;", 2, "Expected a state name after `>`" },
        { "- x > a", 0, "Need a state name before `-`" },
        { "> a; > b", 5, "Repeating initial state" },
        { "a -> b", 0, "No initial state specified" },
        // good cases
        { "# comment\n> @a", 0, "" },
        { "> 0 - a - b - c - d > @1", 0, "" }
    };
    for (auto &test : tests) {
        INFO("Parsing `" << test.source << "`");
        if (test.message.empty())
            INFO("Expecting no error");
        else
            INFO("Expecting `" << test.message << "` @ " << test.pos);
        auto res = parse_fsa(test.source);
        if (!test.message.empty()) {
            REQUIRE(!res.has_value());
            REQUIRE(res.error().at == test.pos);
            REQUIRE(res.error().message == test.message);
        } else {
            REQUIRE(res.has_value());
        }
    }
}

TEST_CASE("Algorithms: gviz dump") {
    // this is essentially a smoke test, I won't be bringing dot
    // here to verify syntax

    auto aut = parse_fsa("> x - b > @y -> z");
    REQUIRE(aut.has_value());
    std::stringstream ss;
    dump_gviz(ss, aut.value());
    std::string sss = ss.str();

    REQUIRE_THAT(sss, StartsWith("digraph") 
        && ContainsSubstring("label=\"x\"")
        && ContainsSubstring("peripheries=2")
    );
}

struct CompareTest {
    View source;
    View expected;
};

static void l_run_op_and_cmp(ArrayRef<CompareTest> tests, std::function<void(Automata &a)> op) {
    for (auto &test : tests) {
        INFO("Resulting FSA is expected to be like:\n" << test.expected);
        auto reference = parse_fsa(test.expected);
        REQUIRE(reference.has_value());
        std::stringstream ref_strstr;
        dump_fsa(ref_strstr, reference.value());
        std::string ref_str = ref_strstr.str();
        INFO("In standard formatting:\n" << ref_str);

        INFO("Source FSA is:\n" << test.source);
        auto initial = parse_fsa(test.source);
        REQUIRE(initial.has_value());

        op(initial.value());
        std::stringstream out_strstr;
        dump_fsa(out_strstr, initial.value());
        std::string out_str = out_strstr.str();
        INFO("Got:\n" << out_str);

        REQUIRE(ref_str == out_str);
    }
}

TEST_CASE("Algorithms: epsilon combing") {
    std::vector<CompareTest> tests = {
        { "> .a -> @.b", "> @.a" },
        { "> .a -> .b -> @.c", "> @.a" },
        { 
            "> .i; .i - a > .a -> @.f; .i - b > .b -> @.f",
            "> .i; .i - a > @.f; .i  - b > @.f"
        }
    };
    l_run_op_and_cmp(tests, [](Automata &a){
        comb_eps(a);
    });
}

TEST_CASE("Algorithms: dead end removal") {
    std::vector<CompareTest> tests = {
        { 
            "> .0 - a > .1; .0 - b > @.2",
            "> .0 - b > @.2"
        },
        {
            // unreachable
            "> .0 - a > @.1; .2 - a > .2",
            "> .0 - a > @.1"
        },
        {
            // don't touch initial
            "> .0",
            "> .0" 
        },
        {
            // don't touch loops
            "> @.0 - a > .1 - a > @.0",
            "> @.0 - a > .1 - a > @.0",
        }
    };
    l_run_op_and_cmp(tests, [](Automata &a){
        remove_dead_ends(a);
    });
}

TEST_CASE("Algorithms: matching") {
    
    // kind of basic, but matching is also done simply

    struct Test {
        View source;
        View word;
        bool expected;
    };

    // match odd number of a's
    View evenodd = "> even - a > @odd - a > even; even - b > even; @odd - b > @odd";

    std::vector<Test> tests = {
        { evenodd, "ab", true },
        { evenodd, "aab", false },
        { evenodd, "aac", false },
    };

    for (auto &test : tests) {
        auto automata = parse_fsa(test.source).value();
        REQUIRE(automata.is_determined());
        REQUIRE(matches(automata, test.word) == test.expected);
    }
}

TEST_CASE("Algorithms: determinizing") {
    std::vector<CompareTest> tests = {
        { 
            // simple eps clearing
            "> .0 - a > .1 -> @.2",
            "> .0 - a > @.1"
        },
        {  
            // dedup
            "> .0 - a > @.1; .0 - a > @.1",
            "> .0 - a > @.1"
        },
        {
            // simple "last letter is `b`"
            "> .0 - b > @.1; .0 - a > .0; .0 - b > .0",
            "> .0 - a > .0; .0 - b > @.1; @.1 - b > @.1; @.1 - a > .0"
        }
    };
    l_run_op_and_cmp(tests, [](Automata &a){
        a = determinize(a);
    });
}

TEST_CASE("Algorithms: minimization") {
    std::vector<CompareTest> tests = {
        {
            // the basic test
            "> .0;"
            ".0 - a - b - c > @.1;"
            ".0 - b - c > @.1;"
            ".0 - c > @.1",

            "> .0 - a > .1 - b > .2 - c > @.3;"
            ".0 - b > .2; .0 - c > @.3"
        },
        {
            // only non-terminal
            "> .0 - a > .1",
            "> .0"
        },
        {
            // single terminal
            "> @.0",
            "> @.0"
        }
    };
    l_run_op_and_cmp(tests, [](Automata &a){
        a = minimize(a);
    });
}
