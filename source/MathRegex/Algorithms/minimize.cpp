#include "MathRegex/Automata.h"
#include "MathRegex/Utils.h"
#include "MathRegex/Algorithms.h"
#include "xxhash.h"
#include <type_traits>
#include <unordered_map>

namespace mreg {

struct XXH128 { uint64_t low64, high64; }
;
#define GEN_XXH128_OP(OP)\
    bool operator OP(const XXH128 &a, const XXH128 &b) {\
        return std::pair{a.low64, a.high64} OP std::pair{b.low64, b.high64};\
    }

GEN_XXH128_OP(==); GEN_XXH128_OP(!=); 
GEN_XXH128_OP(<); GEN_XXH128_OP(>);
GEN_XXH128_OP(<=); GEN_XXH128_OP(>=);

#undef GEN_XXH128_OP

class l_XXHasher {
    XXH3_state_t *s = nullptr;
public:
    l_XXHasher() {
        s = XXH3_createState();
    }

    void reset(){ XXH3_128bits_reset(s); }

    template<typename T>
    void add(const T &v) {
        static_assert(std::is_pod_v<T>, "Hashed type must be simple");
        XXH3_128bits_update(s, &v, sizeof(v));
    }

    XXH128 digest() {
        auto h = XXH3_128bits_digest(s);
        return { h.low64, h.high64 };
    }

    l_XXHasher(const l_XXHasher &o) = delete;
    l_XXHasher &operator=(const l_XXHasher &o) = delete;
    l_XXHasher &operator=(l_XXHasher &&xh) { s = std::exchange(xh.s, nullptr); return *this; }
    l_XXHasher(l_XXHasher &&xh) { s = std::exchange(xh.s, nullptr); }
    ~l_XXHasher() { XXH3_freeState(s); }
};


class l_Minimizer {

    const Automata &m_aut;
    Automata m_result;
    SmallVec<SmallVec<const State*>> m_classes;
    std::unordered_map<const State*, size_t> m_class;


    void m_split_classes() {

        m_classes.resize(2);
        for (const auto &s : m_aut.states()) {
            size_t cls = s->is_terminal() ? 0 : 1;
            m_classes[cls].push_back(s.get());
            m_class[s.get()] = cls;
        }

        bool made_new = true;
        l_XXHasher hasher;

        while (made_new) {
            made_new = false;
            for (size_t cls = 0; cls < m_classes.size(); ++cls) {
                FlatMap<XXH128, size_t> per_hash;
                bool had_prev = false;
                for (size_t sti = m_classes[cls].size()-1; sti != (size_t) -1; --sti) {
                    auto state = m_classes[cls][sti];
                    hasher.reset();
                    for (auto e : state->edges()) {
                        hasher.add(e.ch().as_uint);
                        hasher.add(m_class[e.to()]);
                    }
                    auto cur = hasher.digest();
                    if (!per_hash.count(cur)) {
                        if (per_hash.empty()) {
                            per_hash[cur] = cls;
                        } else {
                            made_new = true;
                            per_hash[cur] = m_classes.size();
                            m_classes.push_back({});
                        }
                    }
                    size_t new_class = per_hash.at(cur);
                    if (new_class != m_class[state]) {
                        m_classes[cls].erase(m_classes[cls].begin() + sti);
                        m_classes[new_class].push_back(state);
                        m_class[state] = new_class;
                    }
                }
            }
        }
    }

    void m_gen_new() {
        SmallVec<State*> per_class(m_classes.size());
        for (size_t cls = 0; cls < m_classes.size(); ++cls) {
            if (m_classes[cls].empty()) continue; // a case when all are terminal
            per_class[cls] = m_result.add_state();
        }
        for (size_t cls = 0; cls < m_classes.size(); ++cls) {
            if (m_classes[cls].empty()) continue;
            for (const auto &e : m_classes[cls][0]->edges())
                per_class[cls]->add_edge(e.ch(), per_class[m_class[e.to()]]);
        }
        per_class[0]->set_terminal(true); // 0 is the final ones
        m_result.set_initial(per_class[m_class[m_aut.initial()]]);
    }

public:

    Automata run() {
        m_split_classes();
        m_gen_new();
        return std::move(m_result);
    }
    
    l_Minimizer(const Automata &aut) :m_aut(aut), m_result(nullptr) { }
};

Automata minimize(const Automata &aut) {
    return l_Minimizer(aut).run();
}

};
