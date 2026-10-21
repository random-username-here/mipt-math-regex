#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
#include <unordered_set>
namespace mreg {

static View l_fsa_tokenize(size_t &pos, View source) {
    // this works with utf8 fine, since we treat them as words
more_spaces:
    if (pos == source.size()) return "";
    if (isspace(source[pos]) && source[pos] != '\n' && source[pos] != ';') {
        ++pos;
        goto more_spaces;
    }
    if (source[pos] == '#') {
        while (pos < source.size() && source[pos] != '\n')
            ++pos;
        if (pos != source.size())
            ++pos;
        goto more_spaces;
    }

    char ch = source[pos];
    if (ch == '>' || ch == '-' || ch == '\n' || ch == ';') {
        ++pos;
        return source.substr(pos-1, 1);
    }
    size_t start = pos;
    while (pos < source.size() && !isspace(source[pos])
           && source[pos] != '#' && source[pos] != '-' && source[pos] != '>' && source[pos] != ';')
        ++pos;
    return source.substr(start, pos - start);
}

static Unexpected<SyntaxError> l_view_err(View base, View word, std::string &&why) {
    return Unexpected<SyntaxError>({ base, (size_t) (word.data() - base.data()), std::move(why) });
}

Expected<Automata, SyntaxError> parse_fsa(View source) {
    size_t pos = 0;
    Automata aut;
    State *prev = nullptr;
    std::unordered_map<View, State*> by_name;
    View edge = "";

    /// Either make a state, or find it
    auto find_or_create = [&](View name) -> Expected<State*, SyntaxError> {
        bool final = false, hidden = false;
        while(1) {
            if (name.empty())
                break;
            else if (name[0] == '@')
                final = true;
            else if (name[0] == '.')
                hidden = true;
            else
                break;
            name = name.substr(1);
        }

        State *st = nullptr;
        if (name.empty() || !by_name.count(name)) {
            st = aut.add_state();
            if (!hidden)
                st->set_label(name);
            if (!name.empty())
                by_name[name] = st;
        } else {
            st = by_name.at(name);
        }
        if (final)
            st->set_terminal(true);
        if (hidden)
            st->set_label("");
        return st;
    };

    auto flush = [&](View edge, View state) -> Expected<State*, SyntaxError> {
        auto to = TRY(find_or_create(state));
        Rune ch = ZERO_CHAR;
        for (Rune r : U8Reader(edge)) {
            if (ch == 0) ch = r;
            else return l_view_err(source, edge, "Multi-character edges are not supported");
        }
        prev->add_edge(ch, to);
        return to;
    };

    // it is a bit of a mess

    while (pos != source.size()) {
        // grab a pair like `- x` or `> y`
        View spec = l_fsa_tokenize(pos, source);
        if (spec == "\n" || spec == ";") {
            prev = nullptr;
            continue;
        }
        View name = "";
        if (spec != ">" && spec != "-")
            name = spec, spec = "";
        else {
            size_t prev = pos;
            name = l_fsa_tokenize(pos, source);
            if (name == "\n" || name == ">" || name == "-" || name == ";") { // `-` at the eol
                if (spec == "-") // fine
                    name = "", pos = prev;
                else
                    return l_view_err(source, name, "Expected a state name after `" + std::string(spec) + "`");
            }
        }

        if (!prev) { // at new line
            if (spec == "-")
                return l_view_err(source, spec, "Need a state name before `-`");
            prev = TRY(find_or_create(name));
            if (spec == ">") {
                if (aut.initial() != nullptr)
                    return l_view_err(source, spec, "Repeating initial state");
                aut.set_initial(prev);
            }
        } else if (spec == "-")  {
            if (!edge.empty()) { // edge chaining
                prev = TRY(flush(edge, ""));
            }
            edge = name;
        } else if (spec == ">") {
            prev = TRY(flush(edge, name));
            edge = "";
        }
    }
    if (!edge.empty())
        return l_view_err(source, edge, "Edge without an end");

    if (!aut.initial())
        return Unexpected<SyntaxError>({ source, 0, "No initial state specified" });
    return aut;
}

void dump_fsa(std::ostream &os, const Automata &aut) {
    std::unordered_set<const State*> visited;
    std::unordered_map<const State*, size_t> temp_names;
    size_t temp_cnt = 0;

    auto print_name = [&](const State *state) {
        if (state->is_terminal())
            os << "@";
        if (!state->label().empty()) {
            os << state->label();
        } else {
            if (!temp_names.count(state))
                temp_names[state] = ++temp_cnt;
            os << ".$" << temp_names.at(state);
        }
    };

    std::function<void(const State*)> visit;
    visit = [&](const State *state) {
        print_name(state);
        if (visited.count(state)) {
            return;
        }
        visited.insert(state);
        bool first = true;
        for (auto e : state->edges()) {
            if (!first) {
                os << "\n";
                print_name(state);
            }
            if (e.ch() != ZERO_CHAR)
                os << " - " << e.ch() << " > ";
            else
                os << " -> "; // eps
            visit(e.to());
            first = false;
        }
    };
    if (aut.initial()) {
        os << "> ";
        visit(aut.initial());
    }
    for (const auto &state : aut.states())
        if (!visited.count(state.get()))
            visit(state.get());
    os << "\n";
}


};
