#include "MathRegex/Regex.h"
#include "MathRegex/Make.h"
#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
namespace mreg {

class l_ChStream {
    View m_full;
    U8Reader::iterator m_it;
    bool m_escaped;
public:
    l_ChStream(View str)
        :m_full(str), m_it(U8Reader(str).begin()) {}

    bool eof() const { return m_it.tail_size() == 0; }
    Rune ch() const { return *m_it; }
    void next() { 
        m_escaped = false;
        m_it++;
        if (ch() == '\\') {
            m_escaped = true;
            m_it++;
        }
    }
    bool escaped() const { return m_escaped; }

    bool is(Rune r) const { return !m_escaped && ch() == r; }

    size_t pos() const { return m_full.size() - m_it.tail_size(); }

    Unexpected<RegexSyntaxError> error(size_t pos, std::string &&what) {
        return Unexpected<RegexSyntaxError>(RegexSyntaxError {
            .source = m_full,
            .at = pos,
            .message = std::move(what)
        });
    }
};

using AutOrErr = Expected<Automata, RegexSyntaxError>;

static AutOrErr l_read_term(l_ChStream &chs); // one basic thing, like `a` or `(inner)`
static AutOrErr l_read_with_suff(l_ChStream &chs); // thing with suffix on top, like `a?`
static AutOrErr l_read_compound(l_ChStream &chs); // compound, `a|b`

static AutOrErr l_read_term(l_ChStream &chs) {
    if (chs.is('(')) {
        // group
        size_t begin = chs.pos();
        chs.next();
        auto insides = TRY(l_read_compound(chs));
        if (chs.eof())
            return chs.error(begin, "Unterminated parenthesis");
        if (!chs.is(')'))
            return chs.error(begin, "Unexpected character, expected `)`");
        chs.next();
        return std::move(insides);
    } else if (chs.is('[')) {
        size_t begin = chs.pos();
        chs.next();
        Automata a;
        auto start = a.initial();
        while (!chs.eof() && !chs.is(']')) {
            if (start->first_for_ch(chs.ch()) == start->edges().end())
                start->add_next(chs.ch())->set_terminal(true);
            chs.next();
        }
        if (!chs.is(']'))
            return chs.error(begin, "Unexpected character, expected `]`");
        chs.next();
        return a;
    } else {
        Automata a;
        a.initial()->add_next(chs.ch())->set_terminal(true);
        chs.next();
        return a;
    }
}

static bool l_isdigit(Rune r) {
    return '0' <= r.as_uint && r.as_uint <= '9';
}

static Expected<size_t, RegexSyntaxError> l_get_int(l_ChStream &chs) {
    size_t v = 0;
    bool any = false;
    while (!chs.eof() && l_isdigit(chs.ch())) {
        v = (v * 10 + (chs.ch().as_uint - '0'));
        chs.next();
        any = true;
    }
    if (!any)
        return chs.error(chs.pos(), "A number expected");
    return v;
}

static AutOrErr l_read_with_suff(l_ChStream &chs) {
    auto aut = TRY(l_read_term(chs));
    if (chs.is('?')) {
        chs.next();
        aut = make_optional(std::move(aut));
    } else if (chs.is('*')) {
        chs.next();
        aut = make_optional(make_loop(std::move(aut)));
    } else if (chs.is('+')) {
        chs.next();
        aut = make_loop(std::move(aut));
    } else if (chs.is('{')) {
        size_t begin = chs.pos();
        chs.next();
        size_t min = TRY(l_get_int(chs));
        size_t max = min;
        if (chs.ch() == ',') {
            chs.next();
            max = TRY(l_get_int(chs));
        }
        if (!chs.is('}'))
            return chs.error(begin, "Count not closed after the numbers");
        chs.next();
        aut = make_repeat(std::move(aut), min, max);
    }
    if (!chs.eof() && (chs.is('?') || chs.is('*') || chs.is('+') || chs.is('{'))) {
        return chs.error(chs.pos(), "Duplicate repetition suffix");
    }
    return aut;
}

static AutOrErr l_read_seq(l_ChStream &chs) {
    Automata a = TRY(l_read_with_suff(chs));
    while (!chs.eof() && !chs.is(')') && !chs.is('|')) {
        Automata av = TRY(l_read_with_suff(chs));
        a = make_chain(std::move(a), std::move(av));
    }
    return a;
}

static AutOrErr l_read_compound(l_ChStream &chs) {
    Automata a = TRY(l_read_seq(chs));
    while (!chs.eof() && !chs.is(')')) {
        assert(chs.is('|'));
        chs.next();
        Automata av = TRY(l_read_seq(chs));
        a = make_or(std::move(a), std::move(av));
    }
    return a;
}

AutOrErr regex(View str) {
    l_ChStream chs(str);
    return l_read_compound(chs);
}

};
