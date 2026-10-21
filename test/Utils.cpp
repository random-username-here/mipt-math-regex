#include "MathRegex/Utils.h"
#include <catch2/catch_test_macros.hpp>
#include <sstream>
using namespace mreg;

TEST_CASE("Utils: U8Reader & Rune printing") {
    // !ш       : normal decoding of 1 and 2 byte
    // \x9f     : cont not in place
    // ┼        : 3-byte
    // \xd1\xd1 : non-cont where cont expected
    // 🌎       : 4-byte
    // \xf0     : char cut in half
    View str = "!ш\x9f┼\xd1\xd1🌎\xf0";
    SmallVec<Rune, 16> v;
    for (Rune r : U8Reader(str))
        v.push_back(r);

    REQUIRE(v.size() == 7);
    REQUIRE(v[0] == U'!');
    REQUIRE(v[1] == U'ш');
    REQUIRE(v[2] == REPLACEMENT_CHAR);
    REQUIRE(v[3] == U'┼');
    REQUIRE(v[4] == REPLACEMENT_CHAR);
    REQUIRE(v[5] == U'🌎');
    REQUIRE(v[6] == REPLACEMENT_CHAR);

    // now print good unicode
    std::stringstream ss;
    for (Rune r : v)
        if (r != REPLACEMENT_CHAR)
            ss << r;
    REQUIRE(ss.str() == "!ш┼🌎");
}

TEST_CASE("Utils: SmallBitset") {
    SmallBitSet<128> bitset;
    REQUIRE(bitset.size() == 0);
    REQUIRE(bitset.word_size() == 0);
    
    bitset.resize(128);
    REQUIRE(bitset.size() == 128);
    REQUIRE(bitset.word_size() == 2);
    REQUIRE(bitset.word(0) == 0);
    REQUIRE(bitset.word(1) == 0);

    bitset.set(65);
    REQUIRE(bitset.word(0) == 0);
    REQUIRE(bitset.word(1) == 1 << 1);

    bitset.set(66);
    bitset.unset(65);
    REQUIRE(bitset.word(1) == 1 << 2);

    REQUIRE(bitset.test(66));
    REQUIRE_FALSE(bitset.test(67));
}

TEST_CASE("Utils: Syntax error") {
    SyntaxError err { "a\nтест\nc", 5, "Test" };
    std::stringstream str;
    err.print(str);
    REQUIRE(str.str() == 
        "Error @ 2:4 : Test\n"
        "   2 | тест\n"
        "     |   ^\n"
    );
}
