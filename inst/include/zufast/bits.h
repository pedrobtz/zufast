/*
 * zufast/bits.h -- half-precision floats and endian helpers (design 16).
 *
 * Loads and stores go through memcpy and shifts, so they are correct at any
 * alignment and on any host byte order; compilers turn them into single
 * instructions. Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_BITS_H
#define ZUFAST_BITS_H

#include "detail/portability.h"

ZUF_STATIC_ASSERT(sizeof(float) == 4, bits_float_is_binary32);
ZUF_STATIC_ASSERT(sizeof(double) == 8, bits_double_is_binary64);

ZUF_INLINE uint32_t zuf_int_f32_bits(float v)  { uint32_t b; memcpy(&b, &v, 4); return b; }
ZUF_INLINE float    zuf_int_bits_f32(uint32_t b) { float v; memcpy(&v, &b, 4); return v; }
ZUF_INLINE uint64_t zuf_int_f64_bits(double v) { uint64_t b; memcpy(&b, &v, 8); return b; }

/* ---- byte swaps -------------------------------------------------------- */

ZUF_INLINE uint16_t zuf_bswap16(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}

ZUF_INLINE uint32_t zuf_bswap32(uint32_t v)
{
    return (v >> 24) | ((v >> 8) & 0x0000FF00u) | ((v << 8) & 0x00FF0000u) | (v << 24);
}

ZUF_INLINE uint64_t zuf_bswap64(uint64_t v)
{
    return ((uint64_t)zuf_bswap32((uint32_t)v) << 32) | zuf_bswap32((uint32_t)(v >> 32));
}

/* ---- loads -------------------------------------------------------------- */

ZUF_INLINE uint16_t zuf_load_le16(const void *p)
{
    unsigned char b[2];
    memcpy(b, p, 2);
    return (uint16_t)(b[0] | (b[1] << 8));
}

ZUF_INLINE uint32_t zuf_load_le32(const void *p)
{
    unsigned char b[4];
    memcpy(b, p, 4);
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

ZUF_INLINE uint64_t zuf_load_le64(const void *p)
{
    const unsigned char *b = (const unsigned char *)p;
    return (uint64_t)zuf_load_le32(b) | ((uint64_t)zuf_load_le32(b + 4) << 32);
}

ZUF_INLINE uint16_t zuf_load_be16(const void *p)
{
    unsigned char b[2];
    memcpy(b, p, 2);
    return (uint16_t)((b[0] << 8) | b[1]);
}

ZUF_INLINE uint32_t zuf_load_be32(const void *p)
{
    unsigned char b[4];
    memcpy(b, p, 4);
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

ZUF_INLINE uint64_t zuf_load_be64(const void *p)
{
    const unsigned char *b = (const unsigned char *)p;
    return ((uint64_t)zuf_load_be32(b) << 32) | (uint64_t)zuf_load_be32(b + 4);
}

/* ---- stores ------------------------------------------------------------- */

ZUF_INLINE void zuf_store_le16(void *p, uint16_t v)
{
    unsigned char b[2];
    b[0] = (unsigned char)v;
    b[1] = (unsigned char)(v >> 8);
    memcpy(p, b, 2);
}

ZUF_INLINE void zuf_store_le32(void *p, uint32_t v)
{
    unsigned char b[4];
    b[0] = (unsigned char)v;
    b[1] = (unsigned char)(v >> 8);
    b[2] = (unsigned char)(v >> 16);
    b[3] = (unsigned char)(v >> 24);
    memcpy(p, b, 4);
}

ZUF_INLINE void zuf_store_le64(void *p, uint64_t v)
{
    unsigned char *b = (unsigned char *)p;
    zuf_store_le32(b, (uint32_t)v);
    zuf_store_le32(b + 4, (uint32_t)(v >> 32));
}

ZUF_INLINE void zuf_store_be16(void *p, uint16_t v)
{
    unsigned char b[2];
    b[0] = (unsigned char)(v >> 8);
    b[1] = (unsigned char)v;
    memcpy(p, b, 2);
}

ZUF_INLINE void zuf_store_be32(void *p, uint32_t v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v >> 24);
    b[1] = (unsigned char)(v >> 16);
    b[2] = (unsigned char)(v >> 8);
    b[3] = (unsigned char)v;
    memcpy(p, b, 4);
}

ZUF_INLINE void zuf_store_be64(void *p, uint64_t v)
{
    unsigned char *b = (unsigned char *)p;
    zuf_store_be32(b, (uint32_t)(v >> 32));
    zuf_store_be32(b + 4, (uint32_t)v);
}

/* ---- binary16 ----------------------------------------------------------- */

/* IEEE 754 binary16 to float; exact for every bit pattern. NaN payloads are
   kept in the top mantissa bits. */
ZUF_INLINE float zuf_f16_to_f32(uint16_t h)
{
    uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
    uint32_t exp = (h >> 10) & 0x1Fu;
    uint32_t mant = h & 0x3FFu;
    if (exp == 0) {
        /* zero or subnormal: mant * 2^-24, exact in binary32 */
        float v = (float)mant * 5.9604644775390625e-8f;
        return sign ? -v : v;
    }
    if (exp == 31) return zuf_int_bits_f32(sign | 0x7F800000u | (mant << 13));
    return zuf_int_bits_f32(sign | ((exp + 112u) << 23) | (mant << 13));
}

/* Float to binary16, round to nearest with ties to even; values that round
   beyond 65504 become infinity. A NaN stays a NaN, made quiet, with the top
   payload bits kept. */
ZUF_INLINE uint16_t zuf_f32_to_f16(float v)
{
    uint32_t x = zuf_int_f32_bits(v);
    uint16_t sign = (uint16_t)((x >> 16) & 0x8000u);
    x &= 0x7FFFFFFFu;
    if (x >= 0x7F800000u) {
        if (x > 0x7F800000u) return (uint16_t)(sign | 0x7E00u | ((x >> 13) & 0x3FFu));
        return (uint16_t)(sign | 0x7C00u);
    }
    if (x >= 0x477FF000u) return (uint16_t)(sign | 0x7C00u);   /* >= 65520 */
    if (x < 0x38800000u) {                                     /* < 2^-14: subnormal or zero */
        uint32_t e = x >> 23, m, shift, hm, rem, half;
        if (e < 102u) return sign;                             /* < 2^-25 rounds to zero */
        m = (x & 0x7FFFFFu) | 0x800000u;
        shift = 126u - e;                                      /* 14..24 */
        hm = m >> shift;
        rem = m & ((1u << shift) - 1u);
        half = 1u << (shift - 1u);
        if (rem > half || (rem == half && (hm & 1u))) hm++;
        return (uint16_t)(sign | hm);
    }
    x -= 0x38000000u;                                           /* rebias 127 -> 15 */
    x += 0xFFFu + ((x >> 13) & 1u);
    return (uint16_t)(sign | (x >> 13));
}

/* ---- bfloat16 ----------------------------------------------------------- */

ZUF_INLINE float zuf_bf16_to_f32(uint16_t b)
{
    return zuf_int_bits_f32((uint32_t)b << 16);
}

/* Float to bfloat16, round to nearest with ties to even; a NaN stays a quiet
   NaN. */
ZUF_INLINE uint16_t zuf_f32_to_bf16(float v)
{
    uint32_t x = zuf_int_f32_bits(v);
    if ((x & 0x7FFFFFFFu) > 0x7F800000u) return (uint16_t)((x >> 16) | 0x40u);
    x += 0x7FFFu + ((x >> 16) & 1u);
    return (uint16_t)(x >> 16);
}

/* ---- width predicates (CBOR preferred serialisation, RFC 8949 4.2.2) ---- */

/* True when v survives float -> binary16 -> float bit for bit. */
ZUF_INLINE bool zuf_f32_fits_f16(float v)
{
    return zuf_int_f32_bits(zuf_f16_to_f32(zuf_f32_to_f16(v))) == zuf_int_f32_bits(v);
}

/* True when v survives double -> float -> double bit for bit. */
ZUF_INLINE bool zuf_f64_fits_f32(double v)
{
    if (v == v && (v > 3.4028234663852886e38 || v < -3.4028234663852886e38)) {
        /* finite beyond FLT_MAX, or infinite: the conversion is only defined
           for the infinities */
        return v - v != 0.0;   /* true for +-Inf (Inf - Inf is NaN), false otherwise */
    }
    return zuf_int_f64_bits((double)(float)v) == zuf_int_f64_bits(v);
}

#endif /* ZUFAST_BITS_H */
