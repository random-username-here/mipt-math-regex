#include "MathRegex/Regex.h"
#include "MathRegex/Algorithms.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
using namespace mreg;

TEST_CASE("Automata: normal regex building") {

    // fsa repr is predictable unless we have duplicate edges
    std::vector<std::pair<std::string_view, std::string_view>> cases = {

        // ==== simple

        { "a", "> .0 - a > @.1" },
        { "\\[🌎\\]", "> .0 - [ > .1 - 🌎 > .2 - ] > @.3" }, // unicode and escapes
        { "ab", "> .0 - a > .1 - b > @.2" }, // no eps needed here
        { 
            "[ab]", 
            // we don't make one terminal here
            "> .i;"
            ".i - a > @.a;"
            ".i - b > @.b;"
        },  
        {
            "[ab]c",
            // but we must merge them here
            "> .i;"
            ".i - a > .a -> .f;"
            ".i - b > .b -> .f;"
            ".f - c > @.c;"
        }, 

        // ==== qualifiers
        // no a* test for now, but that is just ((x)+)?
        // TODO: make some more determined comparsion way 

        {
            "a?",
            "> .i - a > @.f; .i -> @.f"
        },
        {
            "(ab)+",
            "> .0 - a > .1 - b > @.2 -> .0"
        },
        {
            "a{2,4}",
            "> .0 - a > .1 - a > .2 - a > .3 - a > @.4; .2 -> .3 -> @.4"
        },
        {
            "a|xb?",
            "> .i - a > @.a;"
            ".i -> .t - x > .x;" // that temp exists, 
            ".x - b > @.e;"
            ".x -> @.e"
        }
    };

    for (auto &[expr, repr] : cases) {
        INFO("Converting regex /" << expr << "/");

        // parse and dump so formatting is the standard
        auto correct = parse_fsa(repr);
        REQUIRE(correct.has_value());
        std::stringstream s1;
        dump_fsa(s1, correct.value());
        std::string s1s = s1.str();
        INFO("Expected fsa is:\n" << s1s);

        auto reg = regex(expr);
        if (!reg.has_value()) {
            std::stringstream ss;
            reg.error().print(ss);
            INFO("Error:\n" << ss.str());
        }
        REQUIRE(reg.has_value());
        std::stringstream s2;
        dump_fsa(s2, reg.value());
        std::string s2s = s2.str();
        INFO("Got:\n" << s2s);

        REQUIRE(s1s == s2s);
    }
}

TEST_CASE("Automata: errors") {

    struct Test {
        View regex;
        size_t pos;
        View message;
    };
    std::vector<Test> tests = {
        { "(a", 0, "Unterminated group" },
        { "[a", 0, "Unterminated set" },
        { "a{x}", 2, "A number expected" },
        { "a{3x}", 1, "Count not closed after the numbers" },
        { "a*+", 2, "Duplicate repetition suffix" }
    };
    for (auto &test : tests) {
        INFO("Parsing /" << test.regex << "/");
        INFO("Expecting `" << test.message << "` @ " << test.pos);
        auto res = regex(test.regex);
        REQUIRE(!res.has_value());
        REQUIRE(res.error().at == test.pos);
        REQUIRE(res.error().message == test.message);
    }
}
