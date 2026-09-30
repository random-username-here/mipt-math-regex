#include "MathRegex/Utils.h"
namespace mreg {

void U8Reader::iterator::m_scan() {
    if (m_view.empty()) {
        m_char = 0;
        m_char_len = 0;
        return;
    }

    uint8_t ch = m_view[0];

    if ((ch >> 7) == 0) {
        m_char_len = 1;
    } else if ((ch >> 5) == 0b110) {
        m_char_len = 2;
    } else if ((ch >> 4) == 0b1110) {
        m_char_len = 3;
    } else if ((ch >> 3) == 0b11110) {
        m_char_len = 4;
    } else { 
        m_char_len = 1;
        m_char = REPLACEMENT_CHAR;
        return;
    }

    if (m_view.size() < m_char_len) {
        m_char_len = m_view.size();
        m_char = REPLACEMENT_CHAR;
    }

    uint32_t result = ((ch << m_char_len) & 0xff) >> m_char_len; // trim top 1-s
    for (size_t i = 1; i < m_char_len; ++i) {
        if ((uint8_t(m_view[i]) >> 6) != 0b10) {
            m_char = REPLACEMENT_CHAR;
            return;
        }
        result = (result << 6) + (uint8_t(m_view[i]) & 0x7f);
    }
    m_char = result;
}

std::ostream &operator<<(std::ostream &os, Rune c) {
    size_t len = 0;
    if (c.as_uint < 0x80) len = 1;
    else if (c.as_uint < 0x800) len = 2;
    else if (c.as_uint < 0x10000) len = 3;
    else len = 4;
    unsigned initial = (len == 1 ? 0 : ((1 << len) - 1) << (8 - len))
            + (c.as_uint >> (6 * (len-1)));
    os << (char) initial;
    for (size_t i = 1; i < len; ++i) {
        unsigned ext = (1 << 7) + ((c.as_uint >> ((len - i - 1) * 6)) & 0x3f);
        os << (char) ext;
    }
    return os;
}

};
