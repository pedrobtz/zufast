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
 * Shortest formatting writes the fewest significant digits that parse back
 * to the same value (Ryu), in ECMAScript Number::toString notation:
 * positional without a point for integral values below 1e21, positional
 * with a point when the decimal exponent is in (-7, 21), otherwise
 * d.ddde+-x with a lower-case e, an explicit sign and no leading zeros in
 * the exponent. So 1 is "1", 0.1 is "0.1", 1e21 is "1e+21", 1e-7 is
 * "1e-7". Negative zero is "-0", so that the output round-trips bit for
 * bit; non-finite values are NaN, Inf and -Inf, as R writes them.
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_NUMBER_H
#define ZUFAST_NUMBER_H

#include "status.h"
#include "detail/digits.h"
#include "detail/number_impl.h"
#include "detail/format_impl.h"

/* ---- parsing ------------------------------------------------------------ */

typedef struct {
    uint32_t flags;          /* ZUF_NUM_* below; 0 is the default grammar */
    int      base;           /* integers only: 2..36; 0 means 10 */
    char     decimal_point;  /* floating point only: 0 means '.' */
} zuf_num_options;

#define ZUF_NUM_JSON          1u  /* JSON grammar: no leading '+', no leading zeros, no inf/nan */
#define ZUF_NUM_LEADING_PLUS  2u  /* accept a leading '+' */
#define ZUF_NUM_SKIP_SPACE    4u  /* skip leading ASCII whitespace */

/* detail/number_impl.h tests the same bits under its own names. */
ZUF_STATIC_ASSERT(ZUF_NUM_JSON == ZUF_INT_NUM_JSON, number_flag_json);
ZUF_STATIC_ASSERT(ZUF_NUM_LEADING_PLUS == ZUF_INT_NUM_LEADING_PLUS, number_flag_leading_plus);
ZUF_STATIC_ASSERT(ZUF_NUM_SKIP_SPACE == ZUF_INT_NUM_SKIP_SPACE, number_flag_skip_space);

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

/* ---- shortest round-trip ---------------------------------------------- */

/* Capacities that always suffice, with any flags: "-0.000001234567890123456"
   for double (17 digits behind five zeros), "-100000000000000000000.0" for
   float (an integral value just below 1e21 with ZUF_FMT_TRAILING_ZERO). */
#define ZUF_F64_MAX_CHARS 25
#define ZUF_F32_MAX_CHARS 24

#define ZUF_FMT_SCIENTIFIC     1u  /* always d.ddde+-x */
#define ZUF_FMT_TRAILING_ZERO  2u  /* "1.0", not "1", for integral values in positional form */

/* detail/format_impl.h tests the same bits under its own names. */
ZUF_STATIC_ASSERT(ZUF_FMT_SCIENTIFIC == ZUF_INT_FMT_SCIENTIFIC, number_fmt_scientific);
ZUF_STATIC_ASSERT(ZUF_FMT_TRAILING_ZERO == ZUF_INT_FMT_TRAILING_ZERO, number_fmt_trailing_zero);

/* The digits and exponent themselves, for a consumer that formats its own
   way: |v| == mantissa * 10^exponent after rounding to the type, with the
   fewest digits. Zero and non-finite values give mantissa 0, exponent 0. */
typedef struct { uint64_t mantissa; int32_t exponent; bool negative; } zuf_decimal;

ZUF_INLINE zuf_decimal zuf_decimal_f64(double v)
{
    zuf_decimal d;
    uint64_t bits;
    memcpy(&bits, &v, 8);
    (void)zuf_int_shortest_f64(bits, &d.mantissa, &d.exponent);
    d.negative = (bits >> 63) != 0;
    return d;
}

ZUF_INLINE zuf_decimal zuf_decimal_f32(float v)
{
    zuf_decimal d;
    uint32_t bits;
    memcpy(&bits, &v, 4);
    (void)zuf_int_shortest_f32(bits, &d.mantissa, &d.exponent);
    d.negative = (bits >> 31) != 0;
    return d;
}

/* Copy len bytes to dst when they fit. cap == 0 returns before memcpy, so
   that a measuring call with dst == NULL never reaches it. */
ZUF_INLINE size_t zuf_int_emit(char *dst, size_t cap, const char *tmp, size_t len)
{
    if (cap == 0 || cap < len) return len;
    memcpy(dst, tmp, len);
    return len;
}

/* Shortest round-trip text of v; writes only when it fits. */
ZUF_INLINE size_t zuf_format_f64_opt(char *dst, size_t cap, double v, uint32_t flags)
{
    char tmp[32];
    uint64_t bits, mantissa;
    int32_t exponent;
    int special;
    memcpy(&bits, &v, 8);
    special = zuf_int_shortest_f64(bits, &mantissa, &exponent);
    return zuf_int_emit(dst, cap, tmp,
                        zuf_int_format_decimal(tmp, (bits >> 63) != 0, mantissa, exponent, special, flags));
}

ZUF_INLINE size_t zuf_format_f32_opt(char *dst, size_t cap, float v, uint32_t flags)
{
    char tmp[32];
    uint32_t bits;
    uint64_t mantissa;
    int32_t exponent;
    int special;
    memcpy(&bits, &v, 4);
    special = zuf_int_shortest_f32(bits, &mantissa, &exponent);
    return zuf_int_emit(dst, cap, tmp,
                        zuf_int_format_decimal(tmp, (bits >> 31) != 0, mantissa, exponent, special, flags));
}

ZUF_INLINE size_t zuf_format_f64(char *dst, size_t cap, double v)
{
    return zuf_format_f64_opt(dst, cap, v, 0u);
}

ZUF_INLINE size_t zuf_format_f32(char *dst, size_t cap, float v)
{
    return zuf_format_f32_opt(dst, cap, v, 0u);
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
        return zuf_int_emit(dst, cap, s, strlen(s));
    }
    return zuf_int_format_fixed(dst, cap, v, places);
}

#endif /* ZUFAST_NUMBER_H */
