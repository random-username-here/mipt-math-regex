#include <iostream>
#include "MathRegex/Algorithms.h"
#include "MathRegex/Regex.h"
using namespace mreg;

int main(int argc, const char *argv[]) {

    View opt = "mpdka";
    View reg = "";

    // dumbest flag parsing ever
    if (argc == 1) {
        std::cerr << "No regex given\n";
        std::cerr << "Usage: demo [nfa|dfa|mdka] REGEX\n";
        std::cerr << "Does mpdka by default\n";
        return 0;
    } else if (argc == 2) {
        reg = argv[1];
    } else if (argc == 3) {
        opt = argv[1];
        reg = argv[2];
    } else {
        std::cerr << "Too many args given\n";
    }

    auto aut_or_err = mreg::regex(reg);
    if (!aut_or_err) {
        aut_or_err.error().print(std::cout);
        return 0;
    }
    auto aut = std::move(aut_or_err.value());
    mreg::comb_eps(aut);

    if (opt == "nfa") goto dump;
    aut = determinize(aut);
    if (opt == "dfa") goto dump;
    aut = minimize(aut);
    if (opt == "mdka") goto dump;

    std::cerr << "Bad mode name\n";
    return 1;

dump:
    mreg::dump_gviz(std::cout, aut);
    return 0;

    return 0;
}
