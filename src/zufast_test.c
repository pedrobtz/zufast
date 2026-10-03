/*
 * The always-compiled test harness (design 21.4). Each zufast_test_ entry
 * point drives one C function directly, with caller-chosen inputs, so that
 * branches the R API cannot reach are still exercised.
 */
#include <zufast.h>

#include "zufast_r.h"

SEXP zufast_test_status_string(SEXP status)
{
    return Rf_mkString(zuf_status_string((zuf_status)Rf_asInteger(status)));
}

/* Doubles carry the operands exactly as long as they are below 2^53; the
   result is returned as four 32-bit halves so nothing is lost. */
SEXP zufast_test_mul128(SEXP a, SEXP b)
{
    zuf_int_u128 p = zuf_int_mul128((uint64_t)Rf_asReal(a), (uint64_t)Rf_asReal(b));
    SEXP out = PROTECT(Rf_allocVector(REALSXP, 4));
    REAL(out)[0] = (double)(uint32_t)p.low;
    REAL(out)[1] = (double)(p.low >> 32);
    REAL(out)[2] = (double)(uint32_t)p.high;
    REAL(out)[3] = (double)(p.high >> 32);
    UNPROTECT(1);
    return out;
}

/* ---- literal.h ---------------------------------------------------------- */

/* c(status, bytes consumed, value) */
SEXP zufast_test_parse_bool(SEXP x, SEXP accept)
{
    SEXP s = STRING_ELT(x, 0);
    const char *first = CHAR(s), *last = first + LENGTH(s);
    bool v = false;
    zuf_result r = zuf_parse_bool(first, last, (uint32_t)Rf_asInteger(accept), &v);
    SEXP out = PROTECT(Rf_allocVector(INTSXP, 3));
    INTEGER(out)[0] = (int)r.status;
    INTEGER(out)[1] = (int)(r.ptr - first);
    INTEGER(out)[2] = r.status == ZUF_OK ? (int)v : NA_INTEGER;
    UNPROTECT(1);
    return out;
}

SEXP zufast_test_equals(SEXP x, SEXP lit, SEXP ci)
{
    SEXP s = STRING_ELT(x, 0), l = STRING_ELT(lit, 0);
    const char *first = CHAR(s), *last = first + LENGTH(s);
    bool r = Rf_asLogical(ci)
        ? zuf_equals_ci(first, last, CHAR(l), (size_t)LENGTH(l))
        : zuf_equals(first, last, CHAR(l), (size_t)LENGTH(l));
    return Rf_ScalarLogical(r);
}

/* c(offset after zuf_skip_space, trimmed start, trimmed end) */
SEXP zufast_test_trim(SEXP x)
{
    SEXP s = STRING_ELT(x, 0);
    const char *first = CHAR(s), *last = first + LENGTH(s);
    const char *f = first, *l = last;
    SEXP out = PROTECT(Rf_allocVector(INTSXP, 3));
    INTEGER(out)[0] = (int)(zuf_skip_space(first, last) - first);
    zuf_trim_space(&f, &l);
    INTEGER(out)[1] = (int)(f - first);
    INTEGER(out)[2] = (int)(l - first);
    UNPROTECT(1);
    return out;
}

/* ---- bits.h ------------------------------------------------------------- */

/* Decode 16-bit patterns: kind 0 binary16, 1 bfloat16. */
SEXP zufast_test_half_decode(SEXP bits, SEXP kind)
{
    R_xlen_t i, n = XLENGTH(bits);
    int k = Rf_asInteger(kind);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    for (i = 0; i < n; i++) {
        uint16_t b = (uint16_t)INTEGER(bits)[i];
        REAL(out)[i] = (double)(k == 0 ? zuf_f16_to_f32(b) : zuf_bf16_to_f32(b));
    }
    UNPROTECT(1);
    return out;
}

/* Encode doubles (each exactly a float) as 16-bit patterns. */
SEXP zufast_test_half_encode(SEXP x, SEXP kind)
{
    R_xlen_t i, n = XLENGTH(x);
    int k = Rf_asInteger(kind);
    SEXP out = PROTECT(Rf_allocVector(INTSXP, n));
    for (i = 0; i < n; i++) {
        float v = (float)REAL(x)[i];
        INTEGER(out)[i] = (int)(k == 0 ? zuf_f32_to_f16(v) : zuf_f32_to_bf16(v));
    }
    UNPROTECT(1);
    return out;
}

/* Encode floats given by their bit patterns (as doubles, 0..2^32-1). */
SEXP zufast_test_half_encode_bits(SEXP bits, SEXP kind)
{
    R_xlen_t i, n = XLENGTH(bits);
    int k = Rf_asInteger(kind);
    SEXP out = PROTECT(Rf_allocVector(INTSXP, n));
    for (i = 0; i < n; i++) {
        float v = zuf_int_bits_f32((uint32_t)REAL(bits)[i]);
        INTEGER(out)[i] = (int)(k == 0 ? zuf_f32_to_f16(v) : zuf_f32_to_bf16(v));
    }
    UNPROTECT(1);
    return out;
}

/* width 16: zuf_f32_fits_f16((float)x); width 32: zuf_f64_fits_f32(x). */
SEXP zufast_test_fits(SEXP x, SEXP width)
{
    R_xlen_t i, n = XLENGTH(x);
    int w = Rf_asInteger(width);
    SEXP out = PROTECT(Rf_allocVector(LGLSXP, n));
    for (i = 0; i < n; i++) {
        double v = REAL(x)[i];
        LOGICAL(out)[i] = w == 16 ? zuf_f32_fits_f16((float)v) : zuf_f64_fits_f32(v);
    }
    UNPROTECT(1);
    return out;
}

static void hex64(char *dst, uint64_t v)
{
    static const char digits[] = "0123456789abcdef";
    int i;
    for (i = 15; i >= 0; i--) { dst[i] = digits[v & 15]; v >>= 4; }
    dst[16] = '\0';
}

/* Loads at byte offset `off` of a raw vector, every width and order, as hex
   strings, plus the bytes written back by each store and swap identity. */
SEXP zufast_test_endian(SEXP raw, SEXP offset)
{
    const unsigned char *p = RAW(raw) + Rf_asInteger(offset);
    unsigned char buf[8];
    char hex[17];
    uint64_t v[6];
    int i, ok = 1;
    SEXP out = PROTECT(Rf_allocVector(STRSXP, 7));
    v[0] = zuf_load_le16(p); v[1] = zuf_load_be16(p);
    v[2] = zuf_load_le32(p); v[3] = zuf_load_be32(p);
    v[4] = zuf_load_le64(p); v[5] = zuf_load_be64(p);
    for (i = 0; i < 6; i++) { hex64(hex, v[i]); SET_STRING_ELT(out, i, Rf_mkChar(hex)); }

    zuf_store_le16(buf, (uint16_t)v[0]); ok &= memcmp(buf, p, 2) == 0;
    zuf_store_be16(buf, (uint16_t)v[1]); ok &= memcmp(buf, p, 2) == 0;
    zuf_store_le32(buf, (uint32_t)v[2]); ok &= memcmp(buf, p, 4) == 0;
    zuf_store_be32(buf, (uint32_t)v[3]); ok &= memcmp(buf, p, 4) == 0;
    zuf_store_le64(buf, v[4]); ok &= memcmp(buf, p, 8) == 0;
    zuf_store_be64(buf, v[5]); ok &= memcmp(buf, p, 8) == 0;
    ok &= zuf_bswap16((uint16_t)v[0]) == v[1];
    ok &= zuf_bswap32((uint32_t)v[2]) == v[3];
    ok &= zuf_bswap64(v[4]) == v[5];
    SET_STRING_ELT(out, 6, Rf_mkChar(ok ? "ok" : "mismatch"));
    UNPROTECT(1);
    return out;
}

/* ---- utf8.h ------------------------------------------------------------- */

/* A straightforward reference validator (Unicode Table 3-7), independent of
   the DFA. Returns the number of code points, or -1 when invalid. */
static long ref_utf8_count(const unsigned char *s, size_t n)
{
    size_t i = 0;
    long count = 0;
    while (i < n) {
        unsigned c = s[i];
        size_t len, k;
        unsigned lo = 0x80, hi = 0xBF;
        if (c < 0x80) { i++; count++; continue; }
        else if (c >= 0xC2 && c <= 0xDF) len = 2;
        else if (c >= 0xE0 && c <= 0xEF) {
            len = 3;
            if (c == 0xE0) lo = 0xA0;
            if (c == 0xED) hi = 0x9F;
        } else if (c >= 0xF0 && c <= 0xF4) {
            len = 4;
            if (c == 0xF0) lo = 0x90;
            if (c == 0xF4) hi = 0x8F;
        } else return -1;
        if (n - i < len) return -1;
        if (s[i + 1] < lo || s[i + 1] > hi) return -1;
        for (k = 2; k < len; k++) if (s[i + k] < 0x80 || s[i + k] > 0xBF) return -1;
        i += len;
        count++;
    }
    return count;
}

static int utf8_agrees(const unsigned char *s, size_t n)
{
    long ref = ref_utf8_count(s, n);
    bool valid;
    size_t count = zuf_utf8_count((const char *)s, n, &valid);
    if (zuf_utf8_valid((const char *)s, n) != (ref >= 0)) return 0;
    if (valid != (ref >= 0)) return 0;
    if (ref >= 0 && (long)count != ref) return 0;
    return 1;
}

/* Compare the DFA with the reference over every sequence of 1 to 3 bytes and
   every 4-byte sequence whose last two bytes are drawn from the boundary
   set; returns the number of disagreements and the number of cases. */
SEXP zufast_test_utf8_exhaustive(void)
{
    static const unsigned char edge[] = {0x00, 0x41, 0x7F, 0x80, 0x8F, 0x90,
                                         0x9F, 0xA0, 0xBF, 0xC0, 0xC2, 0xE0,
                                         0xF0, 0xF4, 0xF5, 0xFF};
    unsigned char s[12];
    unsigned a, b, c, d;
    double bad = 0, cases = 0;
    for (a = 0; a < 256; a++) {
        s[0] = (unsigned char)a;
        bad += !utf8_agrees(s, 1); cases++;
        for (b = 0; b < 256; b++) {
            s[1] = (unsigned char)b;
            bad += !utf8_agrees(s, 2); cases++;
            for (c = 0; c < 256; c++) {
                s[2] = (unsigned char)c;
                bad += !utf8_agrees(s, 3); cases++;
            }
            for (c = 0; c < sizeof(edge); c++) {
                s[2] = edge[c];
                for (d = 0; d < sizeof(edge); d++) {
                    s[3] = edge[d];
                    bad += !utf8_agrees(s, 4); cases++;
                    /* the same bytes after eight ASCII bytes, through the
                       fast path */
                    memcpy(s + 8, s, 4);
                    memset(s, 'a', 8);
                    bad += !utf8_agrees(s, 12); cases++;
                    memcpy(s, s + 8, 4);
                }
            }
        }
    }
    {
        SEXP out = PROTECT(Rf_allocVector(REALSXP, 2));
        REAL(out)[0] = bad;
        REAL(out)[1] = cases;
        UNPROTECT(1);
        return out;
    }
}

/* c(valid, count) for a raw vector, through zuf_utf8_count. */
SEXP zufast_test_utf8_count(SEXP raw)
{
    bool valid;
    size_t n = zuf_utf8_count((const char *)RAW(raw), (size_t)XLENGTH(raw), &valid);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, 2));
    REAL(out)[0] = valid;
    REAL(out)[1] = (double)n;
    UNPROTECT(1);
    return out;
}
