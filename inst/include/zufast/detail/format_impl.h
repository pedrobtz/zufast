/*
 * zufast/detail/format_impl.h -- shortest round-trip digits from Ryu, and
 * the notation zufast writes them in. Internal: zufast/number.h is the
 * interface.
 */
#ifndef ZUFAST_DETAIL_FORMAT_IMPL_H
#define ZUFAST_DETAIL_FORMAT_IMPL_H

#include "portability.h"
#include "vendor_ryu.h"

/* The shortest decimal mantissa and exponent with mantissa * 10^exponent
   == |v| after rounding back. Zero and non-finite values give 0 and 0. */
ZUF_INLINE void zuf_int_shortest_f64(double v, uint64_t *mantissa, int32_t *exponent)
{
    uint64_t bits, m;
    uint32_t e;
    zuf_int_ryu_floating_decimal_64 d;
    memcpy(&bits, &v, 8);
    m = bits & ((UINT64_C(1) << 52) - 1);
    e = (uint32_t)((bits >> 52) & 0x7FFu);
    if (e == 0x7FFu || (e == 0 && m == 0)) {
        *mantissa = 0;
        *exponent = 0;
        return;
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
}

ZUF_INLINE void zuf_int_shortest_f32(float v, uint64_t *mantissa, int32_t *exponent)
{
    uint32_t bits, m, e;
    zuf_int_ryu_floating_decimal_32 d;
    memcpy(&bits, &v, 4);
    m = bits & ((1u << 23) - 1u);
    e = (bits >> 23) & 0xFFu;
    if (e == 0xFFu || (e == 0 && m == 0)) {
        *mantissa = 0;
        *exponent = 0;
        return;
    }
    d = zuf_int_ryu_f2d(m, e);
    *mantissa = d.mantissa;
    *exponent = d.exponent;
}

#define ZUF_INT_FMT_SCIENTIFIC     1u
#define ZUF_INT_FMT_TRAILING_ZERO  2u

/* Write sign, digits and exponent in ECMAScript Number::toString notation
   (design 9.1) into tmp, which must hold 32 bytes; returns the length.
   `special` is 0 for a finite value, 1 for NaN, 2 for infinity. */
ZUF_INLINE size_t zuf_int_format_decimal(char *tmp, bool negative, uint64_t mantissa,
                                         int32_t exponent, int special, uint32_t flags)
{
    char digits[20];
    int32_t k = 0, n, i;
    size_t len = 0;
    if (special == 1) { memcpy(tmp, "NaN", 3); return 3; }
    if (negative) tmp[len++] = '-';
    if (special == 2) { memcpy(tmp + len, "Inf", 3); return len + 3; }
    if (mantissa == 0) {
        /* zero */
        if (flags & ZUF_INT_FMT_SCIENTIFIC) { memcpy(tmp + len, "0e+0", 4); return len + 4; }
        tmp[len++] = '0';
        if (flags & ZUF_INT_FMT_TRAILING_ZERO) { tmp[len++] = '.'; tmp[len++] = '0'; }
        return len;
    }
    {
        char rev[20];
        while (mantissa) { rev[k++] = (char)('0' + mantissa % 10); mantissa /= 10; }
        for (i = 0; i < k; i++) digits[i] = rev[k - 1 - i];
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
