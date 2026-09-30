///==========================================================================///
/// MathRegex -- regex parsing
///==========================================================================///
#pragma once
#include "MathRegex/Automata.h"

namespace mreg {

struct RegexSyntaxError {
    View source;
    size_t at;
    std::string message;
};

///
/// Parse regex from string.
///
/// Accepted syntax:
///  - any characters for letters
///  - `a|b` for optionals
///  - `(stuff)` for grouping (not capturing groups)
///  - `[abc]` for one character of many
///  - `a*` for repeating however many times
///  - `a+` for repeating at least one time
///  - `a?` for optional
///  - `a{3}` or `a{2,3}` for repeat counts
///
Expected<Automata, RegexSyntaxError> regex(View str);

};
