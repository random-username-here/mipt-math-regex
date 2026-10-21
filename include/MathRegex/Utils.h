///
/// @file
/// @brief Various bits & pieces
///
#pragma once
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <ostream>
#include "span.h"
#include "svector.h"
#include "expected.hpp"
#include "flat_map.hpp"
#include "flat_set.hpp"

namespace mreg {

/// Nice llvm-like implementation of smallvec
template<typename T, size_t MinInlineCap = 0>
using SmallVec = ankerl::svector<T, MinInlineCap>;

template<typename T, size_t InlineSize = 0, typename Cmp = std::less<T>>
using FlatSet = fc::flat_set<SmallVec<T, InlineSize>, Cmp>;

template<typename K, typename V, size_t InlineSize = 0, typename Cmp = std::less<K>>
using FlatMap = fc::flat_map<SmallVec<std::pair<K, V>, InlineSize>, Cmp>;

template<typename T, size_t InlineSize = 0, typename Cmp = std::less<T>>
using FlatMultiSet = fc::flat_set<SmallVec<T, InlineSize>, Cmp>;

template<typename K, typename V, size_t InlineSize = 0, typename Cmp = std::less<K>>
using FlatMultiMap = fc::flat_map<SmallVec<std::pair<K, V>, InlineSize>, Cmp>;

/// Polyfill for C++20's span
template<typename T>
using ArrayRef = tcb::span<T>;

/// The usual string view, just renamed
using View = std::string_view;

/// std::expected and supporting std::unexpected polyfills
/// from c++23
template<typename Val, typename Err>
using Expected = tl::expected<Val, Err>;
template<typename Err>
using Unexpected = tl::unexpected<Err>;

/// unwrap `Expected` or return error
#define TRY(...) ({\
    auto &&_res = (__VA_ARGS__);\
    if (!_res)\
        return Unexpected<\
            std::remove_reference<decltype(_res)>::type::error_type\
        >(std::forward<decltype(_res)>(_res).error());\
    std::forward<decltype(_res)>(_res).value();\
})

/// Standard syntax error
struct SyntaxError {
    View source;
    size_t at;
    std::string message;

    void print(std::ostream &into) const;
};

///
/// A simple variable-sized bitset based on SmallVec.
///
template<size_t InlineBits=0>
class SmallBitSet {
    inline static constexpr size_t m_bit_elems(size_t bits) {
        return (bits + 63) / 64;
    }
    SmallVec<uint64_t, m_bit_elems(InlineBits)> m_words;
    size_t m_bitsize = 0;
public:

    SmallBitSet() = default;
    SmallBitSet(size_t n) { resize(n); }

    template<size_t Other>
    SmallBitSet(const SmallBitSet<Other> &o) {
        resize(o.size());
        for (size_t i = 0; i < o.m_words.size(); ++i)
            m_words[i] = o.m_words[i];
    }

    void resize(size_t bits) {
        m_bitsize = bits; 
        m_words.resize(m_bit_elems(bits), 0);
    }
    size_t size() const { return m_bitsize; }
    void set(size_t i) {
        assert(i < m_bitsize);
        m_words[i/64] |= (uint64_t) 1 << (i % 64);
    }
    void unset(size_t i) {
        assert(i < m_bitsize);
        m_words[i/64] &= ~((uint64_t) 1 << (i % 64));
    }
    bool test(size_t i) const {
        assert(i < m_bitsize);
        return m_words[i/64] & ((uint64_t) 1 << (i % 64));
    }

    size_t word_size() const { return m_words.size(); }
    uint64_t word(size_t i) const { return m_words[i]; }

#define GEN_WORD_OP(OP)\
    SmallBitSet &operator OP##=(const SmallBitSet &o) {\
        if (size() < o.size()) resize(o.size());\
        for (size_t i = 0; i < m_words.size(); ++i)\
            m_words[i] OP##= o.m_words[i];\
        return *this;\
    }\
    SmallBitSet operator OP(const SmallBitSet &o) const {\
        SmallBitSet res = *this; res OP##= o; return res;\
    }
    GEN_WORD_OP(&); GEN_WORD_OP(|); GEN_WORD_OP(^);
#undef GEN_WORD_OP

    SmallBitSet &invert() {
        for (auto &i : m_words) i = ~i;
    }

    SmallBitSet operator~() const { 
        SmallBitSet res = *this; res.invert(); return res;
    }

    bool is_zero() const {
        bool zero = true;
        for (auto w : m_words) zero &= w == 0;
        return zero;
    }

    operator bool() const { return !is_zero(); }

    void for_each_bit(const std::function<void(size_t)> &f) const {
        for (size_t i = 0; i < m_words.size(); ++i) {
            uint64_t v = m_words[i];
            while (v) {
                size_t bit = __builtin_ctzll(v);
                f(i * 64 + bit);
                v ^= ((uint64_t) 1) << bit;
            }
        }
    }

    template<size_t Sb>
    friend bool operator==(const SmallBitSet<InlineBits> &a, const SmallBitSet<Sb> &b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.word_size(); ++i)
            if (a.word(i) != b.word(i)) return false;
        return true;
    }
    template<size_t Sb>
    friend bool operator!=(const SmallBitSet<InlineBits> &a, const SmallBitSet<Sb> &b) {
        return !(a == b);
    }
};

template<size_t S>
std::ostream &operator<<(std::ostream &os, const SmallBitSet<S> &bs) {
    for (size_t i = 0; i < bs.size(); ++i) {
        if (i && i % 8 == 0) os << '_';
        os << (bs.test(i) ? '1' : '0');
    }
    return os;
}

/// Unicode character
struct Rune {
    uint32_t as_uint;

    Rune() = default;
    constexpr Rune(uint32_t v) :as_uint(v) {}

    #define GEN_CMP(OP)\
        bool operator OP(Rune o) const { return as_uint OP o.as_uint; }
    GEN_CMP(==); GEN_CMP(!=);
    GEN_CMP(<); GEN_CMP(>);
    GEN_CMP(<=); GEN_CMP(>=);
    #undef GEN_CMP
};
std::ostream &operator<<(std::ostream &os, Rune c);

inline const Rune ZERO_CHAR = Rune(0);
inline const Rune REPLACEMENT_CHAR = Rune(0xfffd);

/// A little class to read utf8 codepoint by codepoint
class U8Reader {
    View m_view;
public:

    class iterator {
        View m_view;
        Rune m_char;
        size_t m_char_len;

        void m_scan();

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Rune;
        using pointer = Rune; // impossible to obtain
        using reference = Rune;

        iterator(View v) :m_view(v) { m_scan(); }

        size_t tail_size() const { return m_view.size(); }
        size_t char_size() const { return m_char_len; }

        reference operator*() const { return m_char; }
        iterator &operator++() {
            m_view = m_view.substr(m_char_len);
            m_scan();
            return *this;
        }
        iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        friend bool operator==(iterator a, iterator b) { 
            return a.m_view.data() == b.m_view.data();
        }
        friend bool operator!=(iterator a, iterator b) { 
            return a.m_view.data() != b.m_view.data();
        }
    };

    U8Reader(View str) :m_view(str) {};

    iterator begin() const { return iterator(m_view); }
    iterator end() const {  return iterator(m_view.substr(m_view.size())); }
};

/// straight from boost
template <class T>
inline void hash_combine(std::size_t &seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

};

namespace std {

template<size_t S>
struct hash<mreg::SmallBitSet<S>> {
    size_t operator()(const mreg::SmallBitSet<S> &bs) const {
        size_t hash = 123;
        for (size_t i = 0; i < bs.word_size(); ++i)
            mreg::hash_combine(hash, bs.word(i));
        return hash;
    }
};

template<typename A, typename B>
struct hash<std::pair<A, B>> {
    size_t operator()(const std::pair<A, B> &p) const {
        size_t hash = 123;
        mreg::hash_combine(hash, p.first);
        mreg::hash_combine(hash, p.second);
        return hash;
    }
};
};
