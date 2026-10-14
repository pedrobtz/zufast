/*
 * zufast/wide.h -- 128-bit integers (design 9.5).
 *
 * zuf_u128 and zuf_i128 are two-word structs in every build; zuf_i128 is
 * two's complement, with the sign in high. They are distinct types, so
 * mixing them is a compile error, and there are no operators: a consumer
 * writes one body of code whatever the compiler offers.
 *
 * Inside the function bodies, where the compiler defines __SIZEOF_INT128__,
 * multiplication and division use the native type. Defining ZUF_NO_INT128
 * before including this header selects the portable two-word code instead.
 * Both compute the same results; the test suite runs every vector through
 * both. ZUF_NO_INT128 affects this header only.
 *
 * Checked arithmetic (zuf_i128_add() and the others returning bool) returns
 * false on overflow and leaves *out unchanged. The _wrap variants return
 * the result modulo 2^128. Division returns false for a zero divisor and
 * for the one overflowing quotient, MIN / -1. _trunc rounds the quotient
 * toward zero, as C does, and the remainder has the sign of the dividend;
 * _floor rounds toward negative infinity, and the remainder has the sign of
 * the divisor.
 *
 * Conversion to and from double is exact or fails: zuf_i128_to_f64() is
 * false when the value has more than 53 significant bits, and
 * zuf_i128_from_f64() is false for NaN, infinities, values with a fraction
 * and values out of range. Both work on the bits, so no FPU rounding mode
 * is involved.
 *
 * Decimal text follows zuf_parse_i64() and zuf_write_i64() in number.h: an
 * optional '-' for the signed type, then digits; no '+', no whitespace. On
 * ZUF_ERR_INVALID r.ptr equals `first`; on ZUF_ERR_RANGE r.ptr is past the
 * last digit and *out is the saturated bound.
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_WIDE_H
#define ZUFAST_WIDE_H

#include "status.h"
#include "detail/digits.h"

typedef struct { uint64_t low, high; } zuf_u128;
typedef struct { uint64_t low, high; } zuf_i128;   /* two's complement */

#define ZUF_U128_MAX_CHARS 39   /* "340282366920938463463374607431768211455" */
#define ZUF_I128_MAX_CHARS 40   /* "-170141183460469231731687303715884105728" */

#if defined(__SIZEOF_INT128__) && !defined(ZUF_NO_INT128)
#  define ZUF_INT_HAVE_INT128 1
__extension__ typedef __int128 zuf_int_i128_native;
__extension__ typedef unsigned __int128 zuf_int_u128_native;
#else
#  define ZUF_INT_HAVE_INT128 0
#endif

#define ZUF_INT_SIGN64 (UINT64_C(1) << 63)

/* ---- make and split ----------------------------------------------------- */

ZUF_INLINE zuf_u128 zuf_u128_make(uint64_t high, uint64_t low)
{
    zuf_u128 r;
    r.low = low;
    r.high = high;
    return r;
}

ZUF_INLINE zuf_i128 zuf_int_i128_make(uint64_t high, uint64_t low)
{
    zuf_i128 r;
    r.low = low;
    r.high = high;
    return r;
}

ZUF_INLINE uint64_t zuf_u128_low(zuf_u128 v)  { return v.low; }
ZUF_INLINE uint64_t zuf_u128_high(zuf_u128 v) { return v.high; }

ZUF_INLINE zuf_u128 zuf_u128_from_u64(uint64_t v) { return zuf_u128_make(0, v); }

ZUF_INLINE zuf_i128 zuf_i128_from_i64(int64_t v)
{
    return zuf_int_i128_make(v < 0 ? ~UINT64_C(0) : 0, (uint64_t)v);
}

/* The same 128 bits under the other type. */
ZUF_INLINE zuf_i128 zuf_i128_from_u128(zuf_u128 v) { return zuf_int_i128_make(v.high, v.low); }
ZUF_INLINE zuf_u128 zuf_u128_from_i128(zuf_i128 v) { return zuf_u128_make(v.high, v.low); }

/* True, with *out set, when v fits the 64-bit type. */
ZUF_INLINE bool zuf_u128_to_u64(zuf_u128 v, uint64_t *out)
{
    if (v.high != 0) return false;
    *out = v.low;
    return true;
}

ZUF_INLINE bool zuf_i128_to_i64(zuf_i128 v, int64_t *out)
{
    /* fits iff high is the sign extension of low */
    if (v.high != ((v.low & ZUF_INT_SIGN64) ? ~UINT64_C(0) : 0)) return false;
    memcpy(out, &v.low, sizeof *out);
    return true;
}

/* ---- compare ------------------------------------------------------------ */

ZUF_INLINE bool zuf_i128_is_neg(zuf_i128 v) { return (v.high & ZUF_INT_SIGN64) != 0; }

/* -1, 0 or 1. */
ZUF_INLINE int zuf_u128_cmp(zuf_u128 a, zuf_u128 b)
{
    if (a.high != b.high) return a.high < b.high ? -1 : 1;
    if (a.low != b.low) return a.low < b.low ? -1 : 1;
    return 0;
}

ZUF_INLINE int zuf_i128_cmp(zuf_i128 a, zuf_i128 b)
{
    /* flipping the sign bit maps two's complement order onto unsigned order */
    return zuf_u128_cmp(zuf_u128_make(a.high ^ ZUF_INT_SIGN64, a.low),
                        zuf_u128_make(b.high ^ ZUF_INT_SIGN64, b.low));
}

/* ---- internal helpers --------------------------------------------------- */

#if ZUF_INT_HAVE_INT128
ZUF_INLINE zuf_int_u128_native zuf_int_u128_unpack(zuf_u128 v)
{
    return ((zuf_int_u128_native)v.high << 64) | v.low;
}

ZUF_INLINE zuf_u128 zuf_int_u128_pack(zuf_int_u128_native x)
{
    return zuf_u128_make((uint64_t)(x >> 64), (uint64_t)x);
}

ZUF_INLINE zuf_int_i128_native zuf_int_i128_unpack(zuf_i128 v)
{
    return (zuf_int_i128_native)(((zuf_int_u128_native)v.high << 64) | v.low);
}

ZUF_INLINE zuf_i128 zuf_int_i128_pack(zuf_int_i128_native x)
{
    zuf_int_u128_native u = (zuf_int_u128_native)x;
    return zuf_int_i128_make((uint64_t)(u >> 64), (uint64_t)u);
}
#endif

ZUF_INLINE bool zuf_int_u128_is_zero(zuf_u128 v) { return (v.high | v.low) == 0; }

ZUF_INLINE zuf_u128 zuf_int_u128_shl(zuf_u128 v, unsigned s)   /* s in 0..127 */
{
    if (s == 0) return v;
    if (s >= 64) return zuf_u128_make(v.low << (s - 64), 0);
    return zuf_u128_make((v.high << s) | (v.low >> (64 - s)), v.low << s);
}

ZUF_INLINE zuf_u128 zuf_int_u128_shr(zuf_u128 v, unsigned s)   /* s in 0..127 */
{
    if (s == 0) return v;
    if (s >= 64) return zuf_u128_make(0, v.high >> (s - 64));
    return zuf_u128_make(v.high >> s, (v.low >> s) | (v.high << (64 - s)));
}

/* Number of significant bits, 0 for 0. */
ZUF_INLINE unsigned zuf_int_bit_width64(uint64_t v)
{
    unsigned n = 0;
    while (v) { n++; v >>= 1; }
    return n;
}

ZUF_INLINE unsigned zuf_int_u128_bit_width(zuf_u128 v)
{
    return v.high ? 64 + zuf_int_bit_width64(v.high) : zuf_int_bit_width64(v.low);
}

/* Trailing zero bits of a nonzero value. */
ZUF_INLINE unsigned zuf_int_u128_ctz(zuf_u128 v)
{
    uint64_t w = v.low ? v.low : v.high;
    unsigned n = v.low ? 0 : 64;
    while (!(w & 1)) { n++; w >>= 1; }
    return n;
}

/* |v| as unsigned; MIN gives 2^127. */
ZUF_INLINE zuf_u128 zuf_int_i128_magnitude(zuf_i128 v)
{
    zuf_u128 u = zuf_u128_from_i128(v);
    if (!zuf_i128_is_neg(v)) return u;
    return zuf_u128_make(~u.high + (u.low == 0), 0 - u.low);
}

/* ---- add, subtract, negate ---------------------------------------------- */

ZUF_INLINE zuf_u128 zuf_u128_add_wrap(zuf_u128 a, zuf_u128 b)
{
    uint64_t low = a.low + b.low;
    return zuf_u128_make(a.high + b.high + (low < a.low), low);
}

ZUF_INLINE zuf_u128 zuf_u128_sub_wrap(zuf_u128 a, zuf_u128 b)
{
    return zuf_u128_make(a.high - b.high - (a.low < b.low), a.low - b.low);
}

ZUF_INLINE bool zuf_u128_add(zuf_u128 a, zuf_u128 b, zuf_u128 *out)
{
    zuf_u128 r = zuf_u128_add_wrap(a, b);
    if (zuf_u128_cmp(r, a) < 0) return false;
    *out = r;
    return true;
}

ZUF_INLINE bool zuf_u128_sub(zuf_u128 a, zuf_u128 b, zuf_u128 *out)
{
    if (zuf_u128_cmp(a, b) < 0) return false;
    *out = zuf_u128_sub_wrap(a, b);
    return true;
}

ZUF_INLINE zuf_i128 zuf_i128_add_wrap(zuf_i128 a, zuf_i128 b)
{
    return zuf_i128_from_u128(zuf_u128_add_wrap(zuf_u128_from_i128(a), zuf_u128_from_i128(b)));
}

ZUF_INLINE zuf_i128 zuf_i128_sub_wrap(zuf_i128 a, zuf_i128 b)
{
    return zuf_i128_from_u128(zuf_u128_sub_wrap(zuf_u128_from_i128(a), zuf_u128_from_i128(b)));
}

ZUF_INLINE bool zuf_i128_add(zuf_i128 a, zuf_i128 b, zuf_i128 *out)
{
    zuf_i128 r = zuf_i128_add_wrap(a, b);
    /* overflow iff the operands share a sign that the result lacks */
    if (~(a.high ^ b.high) & (a.high ^ r.high) & ZUF_INT_SIGN64) return false;
    *out = r;
    return true;
}

ZUF_INLINE bool zuf_i128_sub(zuf_i128 a, zuf_i128 b, zuf_i128 *out)
{
    zuf_i128 r = zuf_i128_sub_wrap(a, b);
    /* overflow iff the operands differ in sign and the result has b's */
    if ((a.high ^ b.high) & (a.high ^ r.high) & ZUF_INT_SIGN64) return false;
    *out = r;
    return true;
}

/* -v; false for MIN. */
ZUF_INLINE bool zuf_i128_neg(zuf_i128 v, zuf_i128 *out)
{
    if (v.high == ZUF_INT_SIGN64 && v.low == 0) return false;
    *out = zuf_i128_sub_wrap(zuf_int_i128_make(0, 0), v);
    return true;
}

/* ---- multiply ----------------------------------------------------------- */

/* The full 128-bit product of two 64-bit values; never overflows. */
ZUF_INLINE zuf_u128 zuf_u128_mul_u64_u64(uint64_t a, uint64_t b)
{
#if ZUF_INT_HAVE_INT128
    return zuf_int_u128_pack((zuf_int_u128_native)a * b);
#else
    uint64_t a0 = a & 0xFFFFFFFFu, a1 = a >> 32, b0 = b & 0xFFFFFFFFu, b1 = b >> 32;
    uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    /* (2^32 - 1) + (2^32 - 1) + (2^32 - 1)^2 = 2^64 - 1: mid cannot overflow */
    uint64_t mid = (p00 >> 32) + (p10 & 0xFFFFFFFFu) + p01;
    return zuf_u128_make(p11 + (p10 >> 32) + (mid >> 32), (mid << 32) | (p00 & 0xFFFFFFFFu));
#endif
}

/* The full 128-bit product of two signed 64-bit values; never overflows. */
ZUF_INLINE zuf_i128 zuf_i128_mul_i64_i64(int64_t a, int64_t b)
{
#if ZUF_INT_HAVE_INT128
    return zuf_int_i128_pack((zuf_int_i128_native)a * b);
#else
    zuf_u128 p = zuf_u128_mul_u64_u64((uint64_t)a, (uint64_t)b);
    /* the unsigned product read each negative operand as itself + 2^64 */
    if (a < 0) p.high -= (uint64_t)b;
    if (b < 0) p.high -= (uint64_t)a;
    return zuf_i128_from_u128(p);
#endif
}

ZUF_INLINE zuf_u128 zuf_u128_mul_wrap(zuf_u128 a, zuf_u128 b)
{
#if ZUF_INT_HAVE_INT128
    return zuf_int_u128_pack(zuf_int_u128_unpack(a) * zuf_int_u128_unpack(b));
#else
    zuf_u128 p = zuf_u128_mul_u64_u64(a.low, b.low);
    p.high += a.low * b.high + a.high * b.low;
    return p;
#endif
}

ZUF_INLINE zuf_i128 zuf_i128_mul_wrap(zuf_i128 a, zuf_i128 b)
{
    /* the low 128 bits of a product do not depend on signedness */
    return zuf_i128_from_u128(zuf_u128_mul_wrap(zuf_u128_from_i128(a), zuf_u128_from_i128(b)));
}

ZUF_INLINE bool zuf_u128_mul(zuf_u128 a, zuf_u128 b, zuf_u128 *out)
{
    zuf_u128 p, c1, c2;
    uint64_t high;
    if (a.high && b.high) return false;
    p = zuf_u128_mul_u64_u64(a.low, b.low);
    c1 = zuf_u128_mul_u64_u64(a.high, b.low);   /* at most one of c1, c2 is nonzero */
    c2 = zuf_u128_mul_u64_u64(a.low, b.high);
    if (c1.high || c2.high) return false;
    high = p.high + c1.low + c2.low;
    if (high < p.high) return false;
    *out = zuf_u128_make(high, p.low);
    return true;
}

ZUF_INLINE bool zuf_i128_mul(zuf_i128 a, zuf_i128 b, zuf_i128 *out)
{
    bool neg = zuf_i128_is_neg(a) != zuf_i128_is_neg(b);
    zuf_u128 m;
    if (!zuf_u128_mul(zuf_int_i128_magnitude(a), zuf_int_i128_magnitude(b), &m)) return false;
    if (m.high & ZUF_INT_SIGN64) {
        /* only -2^127 has a magnitude of 2^127 or more */
        if (!neg || m.high != ZUF_INT_SIGN64 || m.low != 0) return false;
        *out = zuf_i128_from_u128(m);
        return true;
    }
    *out = zuf_i128_from_u128(neg ? zuf_u128_sub_wrap(zuf_u128_make(0, 0), m) : m);
    return true;
}

/* ---- divide ------------------------------------------------------------- */

#if !ZUF_INT_HAVE_INT128
/* Binary long division, one quotient bit per step; b is nonzero. */
ZUF_INLINE void zuf_int_u128_divmod_bits(zuf_u128 a, zuf_u128 b, zuf_u128 *q, zuf_u128 *r)
{
    zuf_u128 quo = zuf_u128_make(0, 0), rem = zuf_u128_make(0, 0);
    unsigned i = zuf_int_u128_bit_width(a);
    while (i-- > 0) {
        uint64_t carry = rem.high >> 63;
        uint64_t bit = i >= 64 ? (a.high >> (i - 64)) & 1 : (a.low >> i) & 1;
        rem = zuf_u128_make((rem.high << 1) | (rem.low >> 63), (rem.low << 1) | bit);
        /* rem < 2b before the subtraction, so one subtraction suffices and
           the wrapped difference is exact */
        if (carry || zuf_u128_cmp(rem, b) >= 0) {
            rem = zuf_u128_sub_wrap(rem, b);
            if (i >= 64) quo.high |= UINT64_C(1) << (i - 64);
            else quo.low |= UINT64_C(1) << i;
        }
    }
    *q = quo;
    *r = rem;
}
#endif

/* a / b and a % b; false when b is 0. */
ZUF_INLINE bool zuf_u128_divmod(zuf_u128 a, zuf_u128 b, zuf_u128 *q, zuf_u128 *r)
{
    if (zuf_int_u128_is_zero(b)) return false;
#if ZUF_INT_HAVE_INT128
    {
        zuf_int_u128_native x = zuf_int_u128_unpack(a), y = zuf_int_u128_unpack(b);
        *q = zuf_int_u128_pack(x / y);
        *r = zuf_int_u128_pack(x % y);
    }
#else
    zuf_int_u128_divmod_bits(a, b, q, r);
#endif
    return true;
}

ZUF_INLINE bool zuf_u128_divmod_u64(zuf_u128 a, uint64_t b, zuf_u128 *q, uint64_t *r)
{
    zuf_u128 rr;
    if (!zuf_u128_divmod(a, zuf_u128_from_u64(b), q, &rr)) return false;
    *r = rr.low;
    return true;
}

ZUF_INLINE bool zuf_i128_divmod_trunc(zuf_i128 a, zuf_i128 b, zuf_i128 *q, zuf_i128 *r)
{
    if ((b.high | b.low) == 0) return false;
    if (a.high == ZUF_INT_SIGN64 && a.low == 0 && b.high == ~UINT64_C(0) && b.low == ~UINT64_C(0))
        return false;   /* MIN / -1 */
#if ZUF_INT_HAVE_INT128
    {
        zuf_int_i128_native x = zuf_int_i128_unpack(a), y = zuf_int_i128_unpack(b);
        *q = zuf_int_i128_pack(x / y);
        *r = zuf_int_i128_pack(x % y);
    }
#else
    {
        zuf_u128 uq, ur, zero = zuf_u128_make(0, 0);
        zuf_int_u128_divmod_bits(zuf_int_i128_magnitude(a), zuf_int_i128_magnitude(b), &uq, &ur);
        if (zuf_i128_is_neg(a) != zuf_i128_is_neg(b)) uq = zuf_u128_sub_wrap(zero, uq);
        if (zuf_i128_is_neg(a)) ur = zuf_u128_sub_wrap(zero, ur);
        *q = zuf_i128_from_u128(uq);
        *r = zuf_i128_from_u128(ur);
    }
#endif
    return true;
}

ZUF_INLINE bool zuf_i128_divmod_floor(zuf_i128 a, zuf_i128 b, zuf_i128 *q, zuf_i128 *r)
{
    zuf_i128 tq, tr;
    if (!zuf_i128_divmod_trunc(a, b, &tq, &tr)) return false;
    if ((tr.high | tr.low) != 0 && zuf_i128_is_neg(tr) != zuf_i128_is_neg(b)) {
        tq = zuf_i128_sub_wrap(tq, zuf_i128_from_i64(1));
        tr = zuf_i128_add_wrap(tr, b);
    }
    *q = tq;
    *r = tr;
    return true;
}

/* The same with a 64-bit divisor, whose remainder always fits int64_t. */
ZUF_INLINE bool zuf_i128_divmod_trunc_i64(zuf_i128 a, int64_t b, zuf_i128 *q, int64_t *r)
{
    zuf_i128 rr;
    int64_t r64 = 0;
    if (!zuf_i128_divmod_trunc(a, zuf_i128_from_i64(b), q, &rr)) return false;
    (void)zuf_i128_to_i64(rr, &r64);
    *r = r64;
    return true;
}

ZUF_INLINE bool zuf_i128_divmod_floor_i64(zuf_i128 a, int64_t b, zuf_i128 *q, int64_t *r)
{
    zuf_i128 rr;
    int64_t r64 = 0;
    if (!zuf_i128_divmod_floor(a, zuf_i128_from_i64(b), q, &rr)) return false;
    (void)zuf_i128_to_i64(rr, &r64);
    *r = r64;
    return true;
}

/* ---- double ------------------------------------------------------------- */

ZUF_INLINE uint64_t zuf_int_wide_f64_bits(double v) { uint64_t b; memcpy(&b, &v, 8); return b; }
ZUF_INLINE double   zuf_int_wide_bits_f64(uint64_t b) { double v; memcpy(&v, &b, 8); return v; }

/* The magnitude m as a double, when exact. */
ZUF_INLINE bool zuf_int_u128_to_f64(zuf_u128 m, double *out)
{
    unsigned k;
    zuf_u128 s;
    if (zuf_int_u128_is_zero(m)) { *out = 0.0; return true; }
    k = zuf_int_u128_ctz(m);
    s = zuf_int_u128_shr(m, k);
    if (s.high || s.low >= (UINT64_C(1) << 53)) return false;
    /* (double)s is exact below 2^53; scaling by 2^k never rounds */
    *out = (double)s.low * zuf_int_wide_bits_f64((uint64_t)(1023 + k) << 52);
    return true;
}

ZUF_INLINE bool zuf_u128_to_f64(zuf_u128 v, double *out)
{
    return zuf_int_u128_to_f64(v, out);
}

ZUF_INLINE bool zuf_i128_to_f64(zuf_i128 v, double *out)
{
    double d;
    if (!zuf_int_u128_to_f64(zuf_int_i128_magnitude(v), &d)) return false;
    *out = zuf_i128_is_neg(v) ? -d : d;
    return true;
}

/* |v| for an integral finite double below 2^(max_exp + 1), with *neg set;
   false otherwise. -0.0 gives 0. */
ZUF_INLINE bool zuf_int_f64_magnitude(double v, int max_exp, zuf_u128 *mag, bool *neg)
{
    uint64_t bits = zuf_int_wide_f64_bits(v), frac = bits & ((UINT64_C(1) << 52) - 1), m;
    int e = (int)((bits >> 52) & 0x7FF);
    *neg = (bits >> 63) != 0;
    if (e == 0 && frac == 0) { *mag = zuf_u128_make(0, 0); return true; }
    if (e == 0x7FF || e == 0) return false;   /* inf, nan, subnormal */
    e -= 1023;
    if (e < 0 || e > max_exp) return false;
    m = frac | (UINT64_C(1) << 52);
    if (e < 52) {
        if (m & ((UINT64_C(1) << (52 - e)) - 1)) return false;   /* a fraction */
        *mag = zuf_u128_make(0, m >> (52 - e));
    } else {
        *mag = zuf_int_u128_shl(zuf_u128_make(0, m), (unsigned)(e - 52));
    }
    return true;
}

ZUF_INLINE bool zuf_u128_from_f64(double v, zuf_u128 *out)
{
    zuf_u128 m;
    bool neg;
    if (!zuf_int_f64_magnitude(v, 127, &m, &neg)) return false;
    if (neg && !zuf_int_u128_is_zero(m)) return false;
    *out = m;
    return true;
}

ZUF_INLINE bool zuf_i128_from_f64(double v, zuf_i128 *out)
{
    zuf_u128 m;
    bool neg;
    if (!zuf_int_f64_magnitude(v, 127, &m, &neg)) return false;
    if (m.high & ZUF_INT_SIGN64) {
        /* 2^127 and above: only -2^127 fits */
        if (!neg || m.high != ZUF_INT_SIGN64 || m.low != 0) return false;
        *out = zuf_i128_from_u128(m);
        return true;
    }
    *out = zuf_i128_from_u128(neg ? zuf_u128_sub_wrap(zuf_u128_make(0, 0), m) : m);
    return true;
}

/* ---- decimal text ------------------------------------------------------- */

#define ZUF_INT_E19 UINT64_C(10000000000000000000)

ZUF_INLINE uint64_t zuf_int_pow10_u64(unsigned k)   /* k in 0..19 */
{
    uint64_t p = 1;
    while (k--) p *= 10;
    return p;
}

/* Digits at [p, end) into a magnitude; *ovf is set once it passes 2^128 - 1,
   after which the digits are still consumed. Returns one past the last digit. */
ZUF_INLINE const unsigned char *zuf_int_u128_digits(const unsigned char *p, const unsigned char *end,
                                                    zuf_u128 *mag, bool *ovf)
{
    zuf_u128 acc = zuf_u128_make(0, 0);
    *ovf = false;
    while (p < end && (unsigned)(*p - '0') < 10u) {
        uint64_t chunk = 0;
        unsigned k = 0;
        /* up to 19 digits at a time: 10^19 - 1 < 2^64 */
        while (k < 19 && p < end && (unsigned)(*p - '0') < 10u) {
            chunk = chunk * 10 + (uint64_t)(*p - '0');
            k++;
            p++;
        }
        if (!*ovf) {
            uint64_t scale = zuf_int_pow10_u64(k);
            zuf_u128 lo = zuf_u128_mul_u64_u64(acc.low, scale), hi = zuf_u128_mul_u64_u64(acc.high, scale);
            uint64_t high = lo.high + hi.low, low = lo.low + chunk;
            if (hi.high || high < lo.high) { *ovf = true; continue; }
            if (low < chunk) {   /* carry out of the low word */
                if (high == ~UINT64_C(0)) { *ovf = true; continue; }
                high++;
            }
            acc = zuf_u128_make(high, low);
        }
    }
    *mag = acc;
    return p;
}

ZUF_INLINE zuf_result zuf_parse_u128(const char *first, const char *last, zuf_u128 *out)
{
    const unsigned char *p = (const unsigned char *)first, *end = (const unsigned char *)last;
    zuf_u128 mag;
    bool ovf;
    if (p == end || (unsigned)(*p - '0') >= 10u) return zuf_int_result(first, ZUF_ERR_INVALID);
    p = zuf_int_u128_digits(p, end, &mag, &ovf);
    if (ovf) {
        *out = zuf_u128_make(~UINT64_C(0), ~UINT64_C(0));
        return zuf_int_result((const char *)p, ZUF_ERR_RANGE);
    }
    *out = mag;
    return zuf_int_result((const char *)p, ZUF_OK);
}

ZUF_INLINE zuf_result zuf_parse_i128(const char *first, const char *last, zuf_i128 *out)
{
    const unsigned char *p = (const unsigned char *)first, *end = (const unsigned char *)last;
    zuf_u128 mag;
    bool ovf, neg = p < end && *p == '-';
    if (neg) p++;
    if (p == end || (unsigned)(*p - '0') >= 10u) return zuf_int_result(first, ZUF_ERR_INVALID);
    p = zuf_int_u128_digits(p, end, &mag, &ovf);
    /* the magnitude limit is 2^127 - 1, or 2^127 for a negative value */
    if (ovf || ((mag.high & ZUF_INT_SIGN64) && (!neg || mag.high != ZUF_INT_SIGN64 || mag.low != 0))) {
        *out = neg ? zuf_int_i128_make(ZUF_INT_SIGN64, 0) : zuf_int_i128_make(~ZUF_INT_SIGN64, ~UINT64_C(0));
        return zuf_int_result((const char *)p, ZUF_ERR_RANGE);
    }
    *out = zuf_i128_from_u128(neg ? zuf_u128_sub_wrap(zuf_u128_make(0, 0), mag) : mag);
    return zuf_int_result((const char *)p, ZUF_OK);
}

/* The decimal digits of v, unpadded. */
ZUF_INLINE char *zuf_int_wide_write_u64(char *dst, uint64_t v)
{
    size_t n = 1;
    uint64_t t = v;
    while (t >= 10) { t /= 10; n++; }
    zuf_int_write_digits64(dst, v, n);
    return dst + n;
}

/* Write the decimal digits of v; returns one past the last byte written.
   dst must have room for ZUF_U128_MAX_CHARS bytes. */
ZUF_INLINE char *zuf_write_u128(char *dst, zuf_u128 v)
{
    zuf_u128 q;
    uint64_t low19, mid19;
    if (v.high == 0) return zuf_int_wide_write_u64(dst, v.low);
    (void)zuf_u128_divmod_u64(v, ZUF_INT_E19, &q, &low19);
    if (q.high == 0 && q.low < ZUF_INT_E19) {
        dst = zuf_int_wide_write_u64(dst, q.low);
    } else {
        (void)zuf_u128_divmod_u64(q, ZUF_INT_E19, &q, &mid19);
        dst = zuf_int_wide_write_u64(dst, q.low);
        zuf_int_write_digits64(dst, mid19, 19);
        dst += 19;
    }
    zuf_int_write_digits64(dst, low19, 19);
    return dst + 19;
}

/* As zuf_write_u128(), with a '-' for a negative value; dst must have room
   for ZUF_I128_MAX_CHARS bytes. */
ZUF_INLINE char *zuf_write_i128(char *dst, zuf_i128 v)
{
    if (zuf_i128_is_neg(v)) *dst++ = '-';
    return zuf_write_u128(dst, zuf_int_i128_magnitude(v));
}

#endif /* ZUFAST_WIDE_H */
