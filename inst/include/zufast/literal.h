/*
 * zufast/literal.h -- booleans, literal tokens and ASCII whitespace (design 11).
 *
 * ptr semantics follow std::from_chars: on ZUF_OK, r.ptr is one past the
 * token; on ZUF_ERR_INVALID it equals `first`. The parser never requires the
 * whole span to be consumed: check r.ptr == last for "the entire span is a
 * boolean". When several accepted spellings are prefixes of the input, the
 * longest one wins ("TRUE" over "T").
 *
 * zufast hard-codes no policy: the accepted spellings are chosen by flags,
 * and missing-value tokens are the consumer's to match with zuf_equals().
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_LITERAL_H
#define ZUFAST_LITERAL_H

#include "status.h"

#define ZUF_BOOL_LOWER   1u   /* true  false */
#define ZUF_BOOL_UPPER   2u   /* TRUE  FALSE */
#define ZUF_BOOL_TITLE   4u   /* True  False */
#define ZUF_BOOL_LETTER  8u   /* T F t f */
#define ZUF_BOOL_DIGIT  16u   /* 1 0 */
#define ZUF_BOOL_YESNO  32u   /* yes no y n on off, lower, upper and title case */

/* What as.logical() accepts. */
#define ZUF_BOOL_R      (ZUF_BOOL_LOWER | ZUF_BOOL_UPPER | ZUF_BOOL_TITLE | ZUF_BOOL_LETTER)
/* The YAML 1.2 core schema. */
#define ZUF_BOOL_YAML12 (ZUF_BOOL_LOWER | ZUF_BOOL_UPPER | ZUF_BOOL_TITLE)

/* True when [first, last) is exactly the lit_len bytes at lit. */
ZUF_INLINE bool zuf_equals(const char *first, const char *last, const char *lit, size_t lit_len)
{
    return (size_t)(last - first) == lit_len && (lit_len == 0 || memcmp(first, lit, lit_len) == 0);
}

ZUF_INLINE unsigned char zuf_int_ascii_lower(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (unsigned char)(c + ('a' - 'A')) : c;
}

/* As zuf_equals(), comparing ASCII letters case-insensitively. Bytes above
   0x7F are compared exactly. */
ZUF_INLINE bool zuf_equals_ci(const char *first, const char *last, const char *lit, size_t lit_len)
{
    size_t i;
    if ((size_t)(last - first) != lit_len) return false;
    for (i = 0; i < lit_len; i++) {
        if (zuf_int_ascii_lower((unsigned char)first[i]) != zuf_int_ascii_lower((unsigned char)lit[i]))
            return false;
    }
    return true;
}

ZUF_INLINE bool zuf_int_is_space(unsigned char c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');   /* \t \n \v \f \r */
}

/* The first byte of [first, last) that is not ASCII whitespace, or last. */
ZUF_INLINE const char *zuf_skip_space(const char *first, const char *last)
{
    while (first < last && zuf_int_is_space((unsigned char)*first)) first++;
    return first;
}

/* Narrow [*first, *last) to exclude leading and trailing ASCII whitespace. */
ZUF_INLINE void zuf_trim_space(const char **first, const char **last)
{
    const char *f = zuf_skip_space(*first, *last);
    const char *l = *last;
    while (l > f && zuf_int_is_space((unsigned char)l[-1])) l--;
    *first = f;
    *last = l;
}

/* True when [p, p + n) starts with the len bytes at lit. */
ZUF_INLINE bool zuf_int_has_prefix(const char *p, size_t n, const char *lit, size_t len)
{
    return n >= len && memcmp(p, lit, len) == 0;
}

/* Parse a boolean spelled in one of the families selected by `accept`.
   Dispatch is on the first byte; within it the spellings are tried longest
   first, so the first hit is the longest match. */
ZUF_INLINE zuf_result zuf_parse_bool(const char *first, const char *last, uint32_t accept, bool *out)
{
    size_t n = (size_t)(last - first), len = 0;
    bool value = false;
    if (n == 0) return zuf_int_result(first, ZUF_ERR_INVALID);
    switch (*first) {
    case 't':
        value = true;
        if ((accept & ZUF_BOOL_LOWER) && zuf_int_has_prefix(first, n, "true", 4)) len = 4;
        else if (accept & ZUF_BOOL_LETTER) len = 1;
        break;
    case 'f':
        if ((accept & ZUF_BOOL_LOWER) && zuf_int_has_prefix(first, n, "false", 5)) len = 5;
        else if (accept & ZUF_BOOL_LETTER) len = 1;
        break;
    case 'T':
        value = true;
        if ((accept & ZUF_BOOL_UPPER) && zuf_int_has_prefix(first, n, "TRUE", 4)) len = 4;
        else if ((accept & ZUF_BOOL_TITLE) && zuf_int_has_prefix(first, n, "True", 4)) len = 4;
        else if (accept & ZUF_BOOL_LETTER) len = 1;
        break;
    case 'F':
        if ((accept & ZUF_BOOL_UPPER) && zuf_int_has_prefix(first, n, "FALSE", 5)) len = 5;
        else if ((accept & ZUF_BOOL_TITLE) && zuf_int_has_prefix(first, n, "False", 5)) len = 5;
        else if (accept & ZUF_BOOL_LETTER) len = 1;
        break;
    case '1':
        value = true;
        if (accept & ZUF_BOOL_DIGIT) len = 1;
        break;
    case '0':
        if (accept & ZUF_BOOL_DIGIT) len = 1;
        break;
    default:
        if (!(accept & ZUF_BOOL_YESNO)) break;
        switch (*first) {
        case 'y':
            value = true;
            len = zuf_int_has_prefix(first, n, "yes", 3) ? 3 : 1;
            break;
        case 'Y':
            value = true;
            len = zuf_int_has_prefix(first, n, "YES", 3) || zuf_int_has_prefix(first, n, "Yes", 3) ? 3 : 1;
            break;
        case 'n':
            len = zuf_int_has_prefix(first, n, "no", 2) ? 2 : 1;
            break;
        case 'N':
            len = zuf_int_has_prefix(first, n, "NO", 2) || zuf_int_has_prefix(first, n, "No", 2) ? 2 : 1;
            break;
        case 'o':
            if (zuf_int_has_prefix(first, n, "off", 3)) len = 3;
            else if (zuf_int_has_prefix(first, n, "on", 2)) { len = 2; value = true; }
            break;
        case 'O':
            if (zuf_int_has_prefix(first, n, "OFF", 3) || zuf_int_has_prefix(first, n, "Off", 3)) len = 3;
            else if (zuf_int_has_prefix(first, n, "ON", 2) || zuf_int_has_prefix(first, n, "On", 2)) {
                len = 2;
                value = true;
            }
            break;
        default:
            break;
        }
        break;
    }
    if (len == 0) return zuf_int_result(first, ZUF_ERR_INVALID);
    *out = value;
    return zuf_int_result(first + len, ZUF_OK);
}

#endif /* ZUFAST_LITERAL_H */
