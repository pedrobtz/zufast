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

typedef struct {
    const char *text;
    unsigned char len;
    unsigned char value;
    unsigned char flag;
} zuf_int_bool_spelling;

/* Parse a boolean spelled in one of the families selected by `accept`. */
ZUF_INLINE zuf_result zuf_parse_bool(const char *first, const char *last, uint32_t accept, bool *out)
{
    static const zuf_int_bool_spelling spellings[] = {
        {"true", 4, 1, ZUF_BOOL_LOWER},  {"false", 5, 0, ZUF_BOOL_LOWER},
        {"TRUE", 4, 1, ZUF_BOOL_UPPER},  {"FALSE", 5, 0, ZUF_BOOL_UPPER},
        {"True", 4, 1, ZUF_BOOL_TITLE},  {"False", 5, 0, ZUF_BOOL_TITLE},
        {"T", 1, 1, ZUF_BOOL_LETTER},    {"F", 1, 0, ZUF_BOOL_LETTER},
        {"t", 1, 1, ZUF_BOOL_LETTER},    {"f", 1, 0, ZUF_BOOL_LETTER},
        {"1", 1, 1, ZUF_BOOL_DIGIT},     {"0", 1, 0, ZUF_BOOL_DIGIT},
        {"yes", 3, 1, ZUF_BOOL_YESNO},   {"no", 2, 0, ZUF_BOOL_YESNO},
        {"Yes", 3, 1, ZUF_BOOL_YESNO},   {"No", 2, 0, ZUF_BOOL_YESNO},
        {"YES", 3, 1, ZUF_BOOL_YESNO},   {"NO", 2, 0, ZUF_BOOL_YESNO},
        {"y", 1, 1, ZUF_BOOL_YESNO},     {"n", 1, 0, ZUF_BOOL_YESNO},
        {"Y", 1, 1, ZUF_BOOL_YESNO},     {"N", 1, 0, ZUF_BOOL_YESNO},
        {"on", 2, 1, ZUF_BOOL_YESNO},    {"off", 3, 0, ZUF_BOOL_YESNO},
        {"On", 2, 1, ZUF_BOOL_YESNO},    {"Off", 3, 0, ZUF_BOOL_YESNO},
        {"ON", 2, 1, ZUF_BOOL_YESNO},    {"OFF", 3, 0, ZUF_BOOL_YESNO}
    };
    size_t n = (size_t)(last - first), i, best_len = 0;
    int best_value = -1;
    for (i = 0; i < sizeof(spellings) / sizeof(spellings[0]); i++) {
        const zuf_int_bool_spelling *s = &spellings[i];
        if ((accept & s->flag) && s->len <= n && s->len > best_len &&
            memcmp(first, s->text, s->len) == 0) {
            best_len = s->len;
            best_value = s->value;
        }
    }
    if (best_value < 0) return zuf_int_result(first, ZUF_ERR_INVALID);
    *out = best_value != 0;
    return zuf_int_result(first + best_len, ZUF_OK);
}

#endif /* ZUFAST_LITERAL_H */
