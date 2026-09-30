#include "MathRegex/Utils.h"
#include <catch2/catch_test_macros.hpp>
#include <sstream>
using namespace mreg;

TEST_CASE("Utils: Rune") {
    Rune r(U'┼');
    std::stringstream ss;
    ss << r;
    REQUIRE(ss.str() == "┼");
}

TEST_CASE("Utils: U8Reader") {
    View str = "!ш┼";
    SmallVec<Rune, 16> v;
    for (Rune r : U8Reader(str))
        v.push_back(r);

    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == U'!');
    REQUIRE(v[1] == U'ш');
    REQUIRE(v[2] == U'┼');
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
