#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
#include "MathRegex/Algorithms.h"

namespace mreg {

class l_Determinizer {

    using Bitset = SmallBitSet<128>;

    FlatSet<Rune> m_alphabet;
    const Automata &m_aut;
    Automata m_result;
    Bitset m_visited;
    SmallBitSet<> m_is_final;
    SmallVec<FlatMap<Rune, Bitset>> m_masks;
    std::unordered_map<const State*, size_t> m_indices;
    std::unordered_map<Bitset, State*> m_mappings;
    size_t m_num_states;

    const State *m_idx(size_t state) {
        return m_aut.states()[state].get();
    }

    void m_build_masks(size_t idx) {
        if (m_visited.test(idx))
            return; // eps-loop or just visited before
        m_visited.set(idx);

        auto state = m_idx(idx);

        SmallVec<Rune, 16> chars;
        SmallVec<size_t, 8> eps;
        for (const auto &edge : state->edges()) { // they are sorted
            if (edge.is_eps()) {
                size_t to = m_indices[edge.to()];
                eps.push_back(to);
                m_build_masks(to);
            } else {
                if (chars.empty() || edge.ch() != chars.back())
                    chars.push_back(edge.ch());
                m_alphabet.insert(edge.ch());
            }
        }

        for (Rune ch : chars) {
            if (ch == ZERO_CHAR) continue;

            Bitset mask(m_num_states);
            for (auto &edge : state->for_char(ch))
                mask.set(m_indices[edge.to()]);
            m_masks[idx][ch] = mask;
        }

        bool is_fin = state->is_terminal();
        for (size_t s : eps) {
            is_fin |= m_is_final.test(s);
            for (const auto &[ch, bitset] : m_masks[s])
                m_masks[idx][ch] |= bitset;
        }
        if (is_fin)
            m_is_final.set(idx);
    }

    State *m_descend(const Bitset &at) {
        if (m_mappings.count(at))
            return m_mappings.at(at);
        auto s = m_result.add_state();
        m_mappings[at] = s;
        
        bool is_fin = false;
        at.for_each_bit([this, &is_fin](size_t state){
            is_fin |= m_is_final.test(state);
        });
        if (is_fin)
            s->set_terminal(true);

        for (Rune r : m_alphabet) {
            Bitset to(m_num_states);
            at.for_each_bit([this, r, &to](size_t state){
                if (auto bs = m_masks.at(state).has(r); bs != nullptr)
                    to |= *bs;
            });
            if (to.is_zero()) 
                continue;
            s->add_edge(r, m_descend(to));
        }

        return s;
    }

public:
    
    l_Determinizer(const Automata &aut) :m_aut(aut) {}

    Automata run() {
        m_num_states = m_aut.states().size();
        for (size_t i = 0; i < m_aut.states().size(); ++i)
            m_indices[m_aut.states()[i].get()] = i;
       
        m_is_final.resize(m_num_states);
        m_visited.resize(m_num_states);
        m_masks.resize(m_num_states);
        for (size_t i = 0; i < m_num_states; ++i) {
            m_build_masks(i);
        }

        Bitset initial(m_num_states);
        initial.set(m_indices[m_aut.initial()]);
        m_result.set_initial(m_descend(initial));
        return std::move(m_result);
    }
};

Automata determinize(const Automata &aut) {
   return l_Determinizer(aut).run();

}


};
