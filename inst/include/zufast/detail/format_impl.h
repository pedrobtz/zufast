/*
 * zufast/detail/format_impl.h -- shortest round-trip digits from Ryu, and
 * the notation zufast writes them in. Internal: zufast/number.h is the
 * interface.
 */
#ifndef ZUFAST_DETAIL_FORMAT_IMPL_H
#define ZUFAST_DETAIL_FORMAT_IMPL_H

#include "portability.h"
#include "digits.h"
#include "vendor_ryu.h"

/* What zuf_int_shortest_*() classified the value as: the `special`
   argument of zuf_int_format_decimal(). */
#define ZUF_INT_FINITE 0
#define ZUF_INT_NAN    1
#define ZUF_INT_INF    2

/* The shortest decimal mantissa and exponent with mantissa * 10^exponent
   == |v| after rounding back, from the bits of v. Zero and non-finite
   values give 0 and 0. Returns ZUF_INT_FINITE, ZUF_INT_NAN or ZUF_INT_INF. */
ZUF_INLINE int zuf_int_shortest_f64(uint64_t bits, uint64_t *mantissa, int32_t *exponent)
{
    uint64_t m = bits & ((UINT64_C(1) << 52) - 1);
    uint32_t e = (uint32_t)((bits >> 52) & 0x7FFu);
    zuf_int_ryu_floating_decimal_64 d;
    if (e == 0x7FFu || (e == 0 && m == 0)) {
        *mantissa = 0;
        *exponent = 0;
        return e != 0x7FFu ? ZUF_INT_FINITE : m ? ZUF_INT_NAN : ZUF_INT_INF;
    }
    if (zuf_int_ryu_d2d_small_int(m, e, &d)) {
        /* integers below 2^53 come back with trailing decimal zeros */
        for (;;) {
            uint64_t q = d.mantissa / 10;
            if (d.mantissa != 10 * q) break;
            d.mantissa = q;
            d.exponent++;
        }
    } else {
        d = zuf_int_ryu_d2d(m, e);
    }
    *mantissa = d.mantissa;
    *exponent = d.exponent;
    return ZUF_INT_FINITE;
}

ZUF_INLINE int zuf_int_shortest_f32(uint32_t bits, uint64_t *mantissa, int32_t *exponent)
{
    uint32_t m = bits & ((1u << 23) - 1u), e = (bits >> 23) & 0xFFu;
    zuf_int_ryu_floating_decimal_32 d;
    if (e == 0xFFu || (e == 0 && m == 0)) {
        *mantissa = 0;
        *exponent = 0;
        return e != 0xFFu ? ZUF_INT_FINITE : m ? ZUF_INT_NAN : ZUF_INT_INF;
    }
    d = zuf_int_ryu_f2d(m, e);
    *mantissa = d.mantissa;
    *exponent = d.exponent;
    return ZUF_INT_FINITE;
}

#define ZUF_INT_FMT_SCIENTIFIC     1u
#define ZUF_INT_FMT_TRAILING_ZERO  2u

/* Write sign, digits and exponent in ECMAScript Number::toString notation
   (design 9.1) into tmp, which must hold 32 bytes; returns the length.
   `special` is ZUF_INT_FINITE, ZUF_INT_NAN or ZUF_INT_INF. */
ZUF_INLINE size_t zuf_int_format_decimal(char *tmp, bool negative, uint64_t mantissa,
                                         int32_t exponent, int special, uint32_t flags)
{
    char buf[20];
    const char *digits;
    int32_t k, n, i;
    size_t len = 0;
    if (special == ZUF_INT_NAN) { memcpy(tmp, "NaN", 3); return 3; }
    if (negative) tmp[len++] = '-';
    if (special == ZUF_INT_INF) { memcpy(tmp + len, "Inf", 3); return len + 3; }
    if (mantissa == 0) {
        /* zero */
        if (flags & ZUF_INT_FMT_SCIENTIFIC) { memcpy(tmp + len, "0e+0", 4); return len + 4; }
        tmp[len++] = '0';
        if (flags & ZUF_INT_FMT_TRAILING_ZERO) { tmp[len++] = '.'; tmp[len++] = '0'; }
        return len;
    }
    {
        /* the digits, two at a time, right-aligned in buf */
        char *p = buf + sizeof buf;
        while (mantissa >= 100) {
            p -= 2;
            zuf_int_write2(p, (uint32_t)(mantissa % 100));
            mantissa /= 100;
        }
        if (mantissa >= 10) {
            p -= 2;
            zuf_int_write2(p, (uint32_t)mantissa);
        } else {
            *--p = (char)('0' + mantissa);
        }
        digits = p;
        k = (int32_t)(buf + sizeof buf - p);
    }
    n = exponent + k;   /* value = 0.d1d2...dk * 10^n */

    if (!(flags & ZUF_INT_FMT_SCIENTIFIC) && n >= k && n <= 21) {
        /* integral: the digits, then zeros */
        memcpy(tmp + len, digits, (size_t)k);
        len += (size_t)k;
        for (i = k; i < n; i++) tmp[len++] = '0';
        if (flags & ZUF_INT_FMT_TRAILING_ZERO) { tmp[len++] = '.'; tmp[len++] = '0'; }
    } else if (!(flags & ZUF_INT_FMT_SCIENTIFIC) && n > 0 && n <= 21) {
        memcpy(tmp + len, digits, (size_t)n);
        len += (size_t)n;
        tmp[len++] = '.';
        memcpy(tmp + len, digits + n, (size_t)(k - n));
        len += (size_t)(k - n);
    } else if (!(flags & ZUF_INT_FMT_SCIENTIFIC) && n > -6 && n <= 0) {
        tmp[len++] = '0';
        tmp[len++] = '.';
        for (i = n; i < 0; i++) tmp[len++] = '0';
        memcpy(tmp + len, digits, (size_t)k);
        len += (size_t)k;
    } else {
        int32_t e = n - 1;
        uint32_t a;
        tmp[len++] = digits[0];
        if (k > 1) {
            tmp[len++] = '.';
            memcpy(tmp + len, digits + 1, (size_t)(k - 1));
            len += (size_t)(k - 1);
        }
        tmp[len++] = 'e';
        tmp[len++] = e < 0 ? '-' : '+';
        a = (uint32_t)(e < 0 ? -e : e);
        if (a >= 100) tmp[len++] = (char)('0' + a / 100);
        if (a >= 10) tmp[len++] = (char)('0' + a / 10 % 10);
        tmp[len++] = (char)('0' + a % 10);
    }
    return len;
}

#endif /* ZUFAST_DETAIL_FORMAT_IMPL_H */
