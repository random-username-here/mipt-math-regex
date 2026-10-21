#include <iostream>
#include <fstream>
#include <sstream>
#include "MathRegex/Algorithms.h"

static mreg::Automata l_read_from_ifs(std::istream &is) {
    std::ostringstream sstr;
    sstr << is.rdbuf();
    std::string s = sstr.str();
    auto aut = mreg::parse_fsa(s);
    if (!aut) {
        aut.error().print(std::cerr);
        exit(1);
    }
    return std::move(aut.value());
}

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        std::cerr << "Expected an action and input file name\n";
        return 1;
    }
    mreg::Automata aut;
    if (std::string_view(argv[2]) != "-") {
        std::ifstream ifs(argv[2]);
        if (!ifs.is_open()) {
            std::cerr << "Cannot read provided input file\n";
            return 1;
        }
        aut = l_read_from_ifs(ifs);
    } else {
        aut = l_read_from_ifs(std::cin);
    }

    std::string_view action = argv[1];

    if (action == "gviz") {
        mreg::dump_gviz(std::cout, aut);
    } else if (action == "reprint") {
        mreg::dump_fsa(std::cout, aut);
    } else if (action == "det") {
        aut = mreg::determinize(aut);
        mreg::dump_fsa(std::cout, aut);
    } else if (action == "min") {
        if (!aut.is_determined()) {
            std::cerr << "Automata must be determinized first\n";
            return 1;
        }
        aut = mreg::minimize(aut);
        mreg::dump_fsa(std::cout, aut);
    } else {
        std::cerr << "Unknown action\n";
        return 1;
    }
    return 0;
}
