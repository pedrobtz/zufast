/*
 * zufast/number.h -- number parsing and formatting (design 8, 9).
 *
 * PARSING. ptr semantics follow std::from_chars: on ZUF_OK and ZUF_ERR_RANGE,
 * r.ptr is one past the last byte consumed; on ZUF_ERR_INVALID it equals
 * `first`. A parser never requires the whole span to be consumed: a caller
 * that needs "the entire span is a number" checks r.ptr == last.
 *
 * The default grammar is fast_float's `general`: an optional '-', digits
 * with an optional '.' and fraction, an optional exponent; "nan", "inf" and
 * "infinity" in any case, with a sign. No leading '+', no leading
 * whitespace, no hex floats, no "nan(...)" payloads. Integers: an optional
 * '-' for signed types, then digits in the base, no prefix.
 *
 * Results are correctly rounded: the nearest double (float) to the decimal
 * value, ties to even, on every platform and in every FPU rounding mode.
 * This is not R's as.numeric(), which may differ in the last bit.
 *
 * On ZUF_ERR_RANGE a value is still written: +-infinity or +-0 for floating
 * point, the saturated bound for integers.
 *
 * FORMATTING. Every zuf_format_* function returns the full length the value
 * needs, writes at most `cap` bytes, and never NUL-terminates; cap == 0
 * measures. The zuf_write_* functions are the unchecked fast path for a
 * caller that has reserved ZUF_*_MAX_CHARS bytes.
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_NUMBER_H
#define ZUFAST_NUMBER_H

#include "status.h"
#include "detail/digits.h"
#include "detail/number_impl.h"

/* ---- parsing ------------------------------------------------------------ */

typedef struct {
    uint32_t flags;          /* ZUF_NUM_* below; 0 is the default grammar */
    int      base;           /* integers only: 2..36; 0 means 10 */
    char     decimal_point;  /* floating point only: 0 means '.' */
} zuf_num_options;

#define ZUF_NUM_JSON          1u  /* JSON grammar: no leading '+', no leading zeros, no inf/nan */
#define ZUF_NUM_LEADING_PLUS  2u  /* accept a leading '+' */
#define ZUF_NUM_SKIP_SPACE    4u  /* skip leading ASCII whitespace */

ZUF_INLINE zuf_result zuf_parse_f64_opt(const char *first, const char *last, double *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_f64(first, last, out, opt ? opt->flags : 0u, opt ? opt->decimal_point : 0);
}

ZUF_INLINE zuf_result zuf_parse_f32_opt(const char *first, const char *last, float *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_f32(first, last, out, opt ? opt->flags : 0u, opt ? opt->decimal_point : 0);
}

ZUF_INLINE zuf_result zuf_parse_i64_opt(const char *first, const char *last, int64_t *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_i64(first, last, out, opt ? opt->flags : 0u, opt ? opt->base : 10);
}

ZUF_INLINE zuf_result zuf_parse_u64_opt(const char *first, const char *last, uint64_t *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_u64(first, last, out, opt ? opt->flags : 0u, opt ? opt->base : 10);
}

ZUF_INLINE zuf_result zuf_parse_i32_opt(const char *first, const char *last, int32_t *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_i32(first, last, out, opt ? opt->flags : 0u, opt ? opt->base : 10);
}

ZUF_INLINE zuf_result zuf_parse_u32_opt(const char *first, const char *last, uint32_t *out,
                                        const zuf_num_options *opt)
{
    return zuf_int_parse_u32(first, last, out, opt ? opt->flags : 0u, opt ? opt->base : 10);
}

ZUF_INLINE zuf_result zuf_parse_f64(const char *first, const char *last, double *out)
{
    return zuf_int_parse_f64(first, last, out, 0u, 0);
}

ZUF_INLINE zuf_result zuf_parse_f32(const char *first, const char *last, float *out)
{
    return zuf_int_parse_f32(first, last, out, 0u, 0);
}

ZUF_INLINE zuf_result zuf_parse_i64(const char *first, const char *last, int64_t *out)
{
    return zuf_int_parse_i64(first, last, out, 0u, 10);
}

ZUF_INLINE zuf_result zuf_parse_u64(const char *first, const char *last, uint64_t *out)
{
    return zuf_int_parse_u64(first, last, out, 0u, 10);
}

ZUF_INLINE zuf_result zuf_parse_i32(const char *first, const char *last, int32_t *out)
{
    return zuf_int_parse_i32(first, last, out, 0u, 10);
}

ZUF_INLINE zuf_result zuf_parse_u32(const char *first, const char *last, uint32_t *out)
{
    return zuf_int_parse_u32(first, last, out, 0u, 10);
}

/* ---- integer writers ---------------------------------------------------- */

#define ZUF_U64_MAX_CHARS 20
#define ZUF_I64_MAX_CHARS 20   /* "-9223372036854775808" */
#define ZUF_U32_MAX_CHARS 10
#define ZUF_I32_MAX_CHARS 11

/* Write the decimal digits of v; returns one past the last byte written. */
ZUF_INLINE char *zuf_write_u64(char *dst, uint64_t v)
{
    char tmp[ZUF_U64_MAX_CHARS];
    char *p = tmp + sizeof tmp;
    size_t len;
    while (v >= 100) {
        uint32_t r = (uint32_t)(v % 100);
        v /= 100;
        p -= 2;
        zuf_int_write2(p, r);
    }
    if (v >= 10) {
        p -= 2;
        zuf_int_write2(p, (uint32_t)v);
    } else {
        *--p = (char)('0' + v);
    }
    len = (size_t)(tmp + sizeof tmp - p);
    memcpy(dst, p, len);
    return dst + len;
}

ZUF_INLINE char *zuf_write_i64(char *dst, int64_t v)
{
    if (v < 0) {
        *dst++ = '-';
        return zuf_write_u64(dst, 0u - (uint64_t)v);
    }
    return zuf_write_u64(dst, (uint64_t)v);
}

ZUF_INLINE char *zuf_write_u32(char *dst, uint32_t v)
{
    return zuf_write_u64(dst, v);
}

ZUF_INLINE char *zuf_write_i32(char *dst, int32_t v)
{
    return zuf_write_i64(dst, v);
}

/* ---- fixed notation ----------------------------------------------------- */

/* printf("%.*f", places, v) on a libc that prints the exact binary value:
   every digit exact, ties to even, no floating-point arithmetic, the same on
   every platform. A negative `places` means 6. Non-finite values are written
   with R's spellings: NaN, Inf, -Inf. A buffer of 311 + places bytes always
   suffices. When the return value exceeds cap, the first cap bytes of dst
   are unspecified. */
ZUF_INLINE size_t zuf_format_f64_fixed(char *dst, size_t cap, double v, int places)
{
    if (v != v || v - v != 0.0) {   /* NaN or infinite */
        const char *s = v != v ? "NaN" : v > 0 ? "Inf" : "-Inf";
        size_t len = strlen(s);
        if (cap >= len) memcpy(dst, s, len);
        return len;
    }
    return zuf_int_format_fixed(dst, cap, v, places);
}

#endif /* ZUFAST_NUMBER_H */
