/*
 * zufast/utf8.h -- UTF-8 validation (design 17).
 *
 * Bjoern Hoehrmann's DFA decoder ("Flexible and Economical UTF-8 Decoder",
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/), whose state table is
 * transcribed below under its MIT notice:
 *
 *   Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included
 *   in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Overlong forms, surrogates (U+D800..U+DFFF) and code points above U+10FFFF
 * are rejected, as R's validUTF8() rejects them. Every function is pure and
 * may be called from any thread.
 */
#ifndef ZUFAST_UTF8_H
#define ZUFAST_UTF8_H

#include "detail/portability.h"

#define ZUF_INT_UTF8_ACCEPT 0
#define ZUF_INT_UTF8_REJECT 12

ZUF_INLINE const uint8_t *zuf_int_utf8_dfa(void)
{
    static const uint8_t table[] = {
        /* byte -> character class */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00..1F */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20..3F */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40..5F */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60..7F */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 80..9F */
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, /* A0..BF */
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* C0..DF */
        10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8, /* E0..FF */
        /* state + class -> state */
        0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
        12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
        12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
        12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
        12,36,12,12,12,12,12,12,12,12,12,12
    };
    return table;
}

/* True when the n bytes at data are well-formed UTF-8. */
ZUF_INLINE bool zuf_utf8_valid(const char *data, size_t n)
{
    const uint8_t *dfa = zuf_int_utf8_dfa();
    const unsigned char *p = (const unsigned char *)data;
    const unsigned char *end = p + n;
    uint32_t state = ZUF_INT_UTF8_ACCEPT;
    while (p < end) {
        /* ASCII fast path, eight bytes at a time, between code points */
        if (state == ZUF_INT_UTF8_ACCEPT) {
            while (end - p >= 8) {
                uint64_t w;
                memcpy(&w, p, 8);
                if (w & UINT64_C(0x8080808080808080)) break;
                p += 8;
            }
            if (p == end) break;
        }
        state = dfa[256 + state + dfa[*p++]];
        if (state == ZUF_INT_UTF8_REJECT) return false;
    }
    return state == ZUF_INT_UTF8_ACCEPT;
}

/* The number of code points in the n bytes at data. *valid (when not NULL)
   is set to whether the bytes are well-formed; when they are not, the count
   is of the complete code points before the first error. */
ZUF_INLINE size_t zuf_utf8_count(const char *data, size_t n, bool *valid)
{
    const uint8_t *dfa = zuf_int_utf8_dfa();
    const unsigned char *p = (const unsigned char *)data;
    const unsigned char *end = p + n;
    uint32_t state = ZUF_INT_UTF8_ACCEPT;
    size_t count = 0;
    while (p < end) {
        state = dfa[256 + state + dfa[*p++]];
        if (state == ZUF_INT_UTF8_ACCEPT) count++;
        else if (state == ZUF_INT_UTF8_REJECT) break;
    }
    if (valid) *valid = state == ZUF_INT_UTF8_ACCEPT;
    return count;
}

#endif /* ZUFAST_UTF8_H */
