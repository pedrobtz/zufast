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

/* ---- hex.h, base64.h, uuid.h ------------------------------------------- */

#define SENTINEL 0xA5
#define SLACK 8

/* list(return value, the whole buffer of cap + SLACK bytes as raw). The
   buffer starts filled with SENTINEL, so writes past cap are visible. */
static SEXP encode_result(size_t ret, const unsigned char *buf, size_t cap)
{
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 2));
    SEXP raw = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)(cap + SLACK)));
    memcpy(RAW(raw), buf, cap + SLACK);
    SET_VECTOR_ELT(out, 0, Rf_ScalarReal((double)ret));
    SET_VECTOR_ELT(out, 1, raw);
    UNPROTECT(2);
    return out;
}

/* list(status, out_len, buffer) */
static SEXP decode_result(zuf_status st, size_t out_len, const unsigned char *buf, size_t cap)
{
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 3));
    SEXP raw = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)(cap + SLACK)));
    memcpy(RAW(raw), buf, cap + SLACK);
    SET_VECTOR_ELT(out, 0, Rf_ScalarInteger((int)st));
    SET_VECTOR_ELT(out, 1, Rf_ScalarReal((double)out_len));
    SET_VECTOR_ELT(out, 2, raw);
    UNPROTECT(2);
    return out;
}

static unsigned char *sentinel_buffer(size_t cap)
{
    unsigned char *buf = (unsigned char *)R_alloc(cap + SLACK, 1);
    memset(buf, SENTINEL, cap + SLACK);
    return buf;
}

SEXP zufast_test_hex_encode(SEXP raw, SEXP cap, SEXP upper)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    size_t r = zuf_hex_encode(RAW(raw), (size_t)XLENGTH(raw), (char *)buf, c, Rf_asLogical(upper));
    return encode_result(r, buf, c);
}

SEXP zufast_test_hex_decode(SEXP raw, SEXP cap)
{
    size_t c = (size_t)Rf_asReal(cap), len = 99;
    unsigned char *buf = sentinel_buffer(c);
    const char *s = (const char *)RAW(raw);
    zuf_status st = zuf_hex_decode(s, s + XLENGTH(raw), buf, c, &len);
    return decode_result(st, len, buf, c);
}

SEXP zufast_test_base64_encode(SEXP raw, SEXP cap, SEXP flags)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    size_t r = zuf_base64_encode(RAW(raw), (size_t)XLENGTH(raw), (char *)buf, c,
                                 (uint32_t)Rf_asInteger(flags));
    return encode_result(r, buf, c);
}

SEXP zufast_test_base64_decode(SEXP raw, SEXP cap, SEXP flags)
{
    size_t c = (size_t)Rf_asReal(cap), len = 99;
    unsigned char *buf = sentinel_buffer(c);
    const char *s = (const char *)RAW(raw);
    zuf_status st = zuf_base64_decode(s, s + XLENGTH(raw), buf, c, &len,
                                      (uint32_t)Rf_asInteger(flags));
    return decode_result(st, len, buf, c);
}

/* c(encode_bound(n), decode_bound(n)) */
SEXP zufast_test_base64_bounds(SEXP n)
{
    size_t k = (size_t)Rf_asReal(n);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, 2));
    REAL(out)[0] = (double)zuf_base64_encode_bound(k);
    REAL(out)[1] = (double)zuf_base64_decode_bound(k);
    UNPROTECT(1);
    return out;
}

/* The encoder's length arithmetic at the edges of size_t, through measuring
   calls (src and dst NULL, cap 0), which must return before the write loop.
   TRUE for each case that returns what it should. */
SEXP zufast_test_base64_limits(void)
{
    const size_t max_ok = (SIZE_MAX / 4) * 3 - 2;  /* the largest n that fits */
    const size_t max_len = (SIZE_MAX / 4) * 4;     /* its padded length */
    const size_t wraps = (SIZE_MAX / 4 + 1) * 3;   /* unpadded length wraps to 0 */
    const size_t over[] = {max_ok + 1, max_ok + 2, max_ok + 3, wraps, SIZE_MAX - 1, SIZE_MAX};
    const uint32_t flags[] = {0, ZUF_B64_NO_PAD, ZUF_B64_URL | ZUF_B64_NO_PAD};
    SEXP out = PROTECT(Rf_allocVector(LGLSXP, 3 + 3 * 6));
    int *o = LOGICAL(out), k = 0;
    size_t i, j;
    o[k++] = zuf_base64_encode(NULL, max_ok, NULL, 0, 0) == max_len;
    o[k++] = zuf_base64_encode(NULL, max_ok, NULL, 0, ZUF_B64_NO_PAD) == max_len - (3 - max_ok % 3) % 3;
    o[k++] = zuf_base64_encode_bound(max_ok + 1) == SIZE_MAX;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 6; j++)
            o[k++] = zuf_base64_encode(NULL, over[j], NULL, 0, flags[i]) == SIZE_MAX;
    UNPROTECT(1);
    return out;
}

/* zufast_encoded_len() on a length given as a double; an R error when the
   encoding is too long for an R string. */
SEXP zufast_test_encoded_len(SEXP n, SEXP kind, SEXP flags)
{
    return Rf_ScalarReal((double)zufast_encoded_len((size_t)Rf_asReal(n), Rf_asInteger(kind),
                                                    (uint32_t)Rf_asInteger(flags)));
}

/* list(status, consumed, bytes) */
SEXP zufast_test_parse_uuid(SEXP raw)
{
    const char *s = (const char *)RAW(raw);
    zuf_uuid u;
    zuf_result r;
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 3));
    SEXP bytes = PROTECT(Rf_allocVector(RAWSXP, 16));
    memset(u.bytes, 0, 16);
    r = zuf_parse_uuid(s, s + XLENGTH(raw), &u);
    memcpy(RAW(bytes), u.bytes, 16);
    SET_VECTOR_ELT(out, 0, Rf_ScalarInteger((int)r.status));
    SET_VECTOR_ELT(out, 1, Rf_ScalarInteger((int)(r.ptr - s)));
    SET_VECTOR_ELT(out, 2, bytes);
    UNPROTECT(2);
    return out;
}

SEXP zufast_test_format_uuid(SEXP raw, SEXP cap, SEXP upper)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    zuf_uuid u;
    size_t r;
    memcpy(u.bytes, RAW(raw), 16);
    r = zuf_format_uuid((char *)buf, c, &u, Rf_asLogical(upper));
    return encode_result(r, buf, c);
}

/* ---- datetime.h --------------------------------------------------------- */

static SEXP datetime_fields(const zuf_datetime *dt)
{
    SEXP f = PROTECT(Rf_allocVector(REALSXP, 10));
    double *v = REAL(f);
    v[0] = dt->year; v[1] = dt->month; v[2] = dt->day;
    v[3] = dt->hour; v[4] = dt->minute; v[5] = dt->second;
    v[6] = dt->nanosecond; v[7] = dt->offset_seconds;
    v[8] = dt->has_time; v[9] = dt->has_offset;
    UNPROTECT(1);
    return f;
}

static void fields_datetime(SEXP f, zuf_datetime *dt)
{
    const double *v = REAL(f);
    memset(dt, 0, sizeof *dt);
    dt->year = (int32_t)v[0]; dt->month = (uint8_t)v[1]; dt->day = (uint8_t)v[2];
    dt->hour = (uint8_t)v[3]; dt->minute = (uint8_t)v[4]; dt->second = (uint8_t)v[5];
    dt->nanosecond = (uint32_t)v[6]; dt->offset_seconds = (int32_t)v[7];
    dt->has_time = v[8] != 0; dt->has_offset = v[9] != 0;
}

/* list(status, consumed, fields, timestamp seconds, days) */
SEXP zufast_test_parse_datetime(SEXP raw, SEXP date_only)
{
    const char *s = (const char *)RAW(raw);
    zuf_datetime dt;
    zuf_result r;
    zuf_timestamp t;
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 5));
    memset(&dt, 0xEE, sizeof dt);
    r = Rf_asLogical(date_only) ? zuf_parse_date(s, s + XLENGTH(raw), &dt)
                                : zuf_parse_datetime(s, s + XLENGTH(raw), &dt);
    SET_VECTOR_ELT(out, 0, Rf_ScalarInteger((int)r.status));
    SET_VECTOR_ELT(out, 1, Rf_ScalarInteger((int)(r.ptr - s)));
    if (r.status == ZUF_OK) {
        t = zuf_datetime_timestamp(&dt);
        SET_VECTOR_ELT(out, 2, datetime_fields(&dt));
        SET_VECTOR_ELT(out, 3, Rf_ScalarReal((double)t.seconds));
        SET_VECTOR_ELT(out, 4, Rf_ScalarReal((double)zuf_datetime_days(&dt)));
    }
    UNPROTECT(1);
    return out;
}

SEXP zufast_test_format_datetime(SEXP fields, SEXP cap)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    zuf_datetime dt;
    fields_datetime(fields, &dt);
    return encode_result(zuf_format_datetime((char *)buf, c, &dt), buf, c);
}

SEXP zufast_test_format_date(SEXP days, SEXP cap)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    return encode_result(zuf_format_date((char *)buf, c, (int32_t)Rf_asReal(days)), buf, c);
}

/* civil_from_days for each element: a 3-column integer matrix. Days are
   doubles so the whole int32_t range is reachable. */
SEXP zufast_test_civil_from_days(SEXP days)
{
    R_xlen_t i, n = XLENGTH(days);
    SEXP out = PROTECT(Rf_allocMatrix(REALSXP, (int)n, 3));
    double *o = REAL(out);
    for (i = 0; i < n; i++) {
        int32_t y;
        uint32_t m, d;
        zuf_civil_from_days((int32_t)REAL(days)[i], &y, &m, &d);
        o[i] = y; o[i + n] = m; o[i + 2 * n] = d;
    }
    UNPROTECT(1);
    return out;
}

SEXP zufast_test_days_from_civil(SEXP y, SEXP m, SEXP d)
{
    R_xlen_t i, n = XLENGTH(y);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    for (i = 0; i < n; i++)
        REAL(out)[i] = zuf_days_from_civil((int32_t)REAL(y)[i], (uint32_t)REAL(m)[i],
                                           (uint32_t)REAL(d)[i]);
    UNPROTECT(1);
    return out;
}

/* c(is_leap_year, days_in_month for months 0..13) for one year */
SEXP zufast_test_year_info(SEXP year)
{
    int32_t y = (int32_t)Rf_asReal(year);
    uint32_t m;
    SEXP out = PROTECT(Rf_allocVector(INTSXP, 15));
    INTEGER(out)[0] = zuf_is_leap_year(y);
    for (m = 0; m <= 13; m++) INTEGER(out)[m + 1] = (int)zuf_days_in_month(y, m);
    UNPROTECT(1);
    return out;
}

/* Walk [lo, hi] day by day: each day must round-trip through the civil
   date, and each civil date must be the day after the previous one. Returns
   the number of failures. */
SEXP zufast_test_calendar_walk(SEXP lo, SEXP hi, SEXP step)
{
    int64_t d, a = (int64_t)Rf_asReal(lo), b = (int64_t)Rf_asReal(hi), s = (int64_t)Rf_asReal(step);
    double bad = 0;
    int32_t py = 0;
    uint32_t pm = 0, pd = 0;
    int have_prev = 0;
    for (d = a; d <= b; d += s) {
        int32_t y;
        uint32_t m, dd;
        zuf_civil_from_days((int32_t)d, &y, &m, &dd);
        if (m < 1 || m > 12 || dd < 1 || dd > zuf_days_in_month(y, m)) bad++;
        else if (zuf_days_from_civil(y, m, dd) != (int32_t)d) bad++;
        if (s == 1 && have_prev) {
            int next_ok;
            if (pd < zuf_days_in_month(py, pm)) next_ok = y == py && m == pm && dd == pd + 1;
            else if (pm < 12) next_ok = y == py && m == pm + 1 && dd == 1;
            else next_ok = y == py + 1 && m == 1 && dd == 1;
            if (!next_ok) bad++;
        }
        py = y; pm = m; pd = dd; have_prev = 1;
    }
    return Rf_ScalarReal(bad);
}

/* ---- number.h ----------------------------------------------------------- */

#include <stdlib.h>

/* list(status, consumed, value): kind 0 f64, 1 f32 (value a double), 2 i64,
   3 u64, 4 i32, 5 u32 (value a decimal string written by zuf_write_*). The
   value is reported for ZUF_OK and ZUF_ERR_RANGE. */
SEXP zufast_test_parse_num(SEXP raw, SEXP kind, SEXP flags, SEXP base, SEXP decimal_point)
{
    const char *s = (const char *)RAW(raw), *e = s + XLENGTH(raw);
    zuf_num_options opt;
    zuf_result r;
    char buf[32];
    SEXP val = R_NilValue, out;
    int k = Rf_asInteger(kind);
    opt.flags = (uint32_t)Rf_asInteger(flags);
    opt.base = Rf_asInteger(base);
    opt.decimal_point = (char)Rf_asInteger(decimal_point);
    switch (k) {
    case 0: { double v = -1; r = zuf_parse_f64_opt(s, e, &v, &opt); val = Rf_ScalarReal(v); break; }
    case 1: { float v = -1; r = zuf_parse_f32_opt(s, e, &v, &opt); val = Rf_ScalarReal((double)v); break; }
    case 2: { int64_t v = -1; r = zuf_parse_i64_opt(s, e, &v, &opt); *zuf_write_i64(buf, v) = 0; val = Rf_mkString(buf); break; }
    case 3: { uint64_t v = 1; r = zuf_parse_u64_opt(s, e, &v, &opt); *zuf_write_u64(buf, v) = 0; val = Rf_mkString(buf); break; }
    case 4: { int32_t v = -1; r = zuf_parse_i32_opt(s, e, &v, &opt); *zuf_write_i32(buf, v) = 0; val = Rf_mkString(buf); break; }
    default: { uint32_t v = 1; r = zuf_parse_u32_opt(s, e, &v, &opt); *zuf_write_u32(buf, v) = 0; val = Rf_mkString(buf); break; }
    }
    PROTECT(val);
    out = PROTECT(Rf_allocVector(VECSXP, 3));
    SET_VECTOR_ELT(out, 0, Rf_ScalarInteger((int)r.status));
    SET_VECTOR_ELT(out, 1, Rf_ScalarInteger((int)(r.ptr - s)));
    if (r.status == ZUF_OK || r.status == ZUF_ERR_RANGE) SET_VECTOR_ELT(out, 2, val);
    UNPROTECT(2);
    return out;
}

/* The default zuf_parse_f64 / zuf_parse_f32 over a character vector, and
   the C library's strtod / strtof on the same strings, side by side: a
   4-column matrix (zuf f64, strtod, zuf f32, strtof). NA where a parser did
   not consume the whole string. R runs with LC_NUMERIC = "C". */
SEXP zufast_test_parse_vs_strtod(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocMatrix(REALSXP, (int)n, 4));
    double *o = REAL(out);
    for (i = 0; i < n; i++) {
        SEXP s = STRING_ELT(x, i);
        const char *first = CHAR(s), *last = first + LENGTH(s);
        char *end;
        double d;
        float f;
        zuf_result r = zuf_parse_f64(first, last, &d);
        o[i] = (r.status != ZUF_ERR_INVALID && r.ptr == last) ? d : NA_REAL;
        d = strtod(first, &end);
        o[i + n] = end == last ? d : NA_REAL;
        r = zuf_parse_f32(first, last, &f);
        o[i + 2 * n] = (r.status != ZUF_ERR_INVALID && r.ptr == last) ? (double)f : NA_REAL;
        f = strtof(first, &end);
        o[i + 3 * n] = end == last ? (double)f : NA_REAL;
    }
    UNPROTECT(1);
    return out;
}

/* zuf_write_i32 for an integer vector. */
SEXP zufast_test_write_i32(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        char buf[ZUF_I32_MAX_CHARS + 1];
        char *e = zuf_write_i32(buf, INTEGER(x)[i]);
        SET_STRING_ELT(out, i, Rf_mkCharLen(buf, (int)(e - buf)));
    }
    UNPROTECT(1);
    return out;
}

/* Write every value 2^k - 1, 2^k, 10^k - 1, 10^k and their negatives with
   the 64-bit and 32-bit writers, and compare each with a naive reference.
   Returns the number of mismatches. */
static size_t naive_u64(char *dst, uint64_t v)
{
    char tmp[24];
    size_t n = 0, i;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    for (i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
    return n;
}

SEXP zufast_test_write_ints(void)
{
    double bad = 0;
    int k, d;
    uint64_t vals[4 * 64 + 8];
    size_t nv = 0, i;
    uint64_t p10 = 1;
    for (k = 0; k < 64; k++) {
        vals[nv++] = (UINT64_C(1) << k);
        vals[nv++] = (UINT64_C(1) << k) - 1;
    }
    for (k = 0; k < 20; k++) {
        vals[nv++] = p10;
        vals[nv++] = p10 - 1;
        if (k < 19) p10 *= 10;
    }
    vals[nv++] = UINT64_MAX;
    vals[nv++] = UINT64_MAX - 1;
    for (i = 0; i < nv; i++) {
        for (d = -1; d <= 1; d++) {
            uint64_t v = vals[i] + (uint64_t)(int64_t)d;
            char a[32], b[32];
            size_t la, lb;
            la = (size_t)(zuf_write_u64(a, v) - a);
            lb = naive_u64(b, v);
            bad += la != lb || memcmp(a, b, la) != 0;
            {   /* as int64, both signs */
                int64_t sv = (int64_t)v;
                la = (size_t)(zuf_write_i64(a, sv) - a);
                if (sv < 0) { b[0] = '-'; lb = 1 + naive_u64(b + 1, 0u - (uint64_t)sv); }
                else lb = naive_u64(b, (uint64_t)sv);
                bad += la != lb || memcmp(a, b, la) != 0 || la > ZUF_I64_MAX_CHARS;
            }
            {
                uint32_t u = (uint32_t)v;
                int32_t s = (int32_t)u;
                la = (size_t)(zuf_write_u32(a, u) - a);
                lb = naive_u64(b, u);
                bad += la != lb || memcmp(a, b, la) != 0 || la > ZUF_U32_MAX_CHARS;
                la = (size_t)(zuf_write_i32(a, s) - a);
                if (s < 0) { b[0] = '-'; lb = 1 + naive_u64(b + 1, (uint64_t)(-(int64_t)s)); }
                else lb = naive_u64(b, (uint64_t)s);
                bad += la != lb || memcmp(a, b, la) != 0 || la > ZUF_I32_MAX_CHARS;
            }
        }
    }
    return Rf_ScalarReal(bad);
}

SEXP zufast_test_format_fixed(SEXP x, SEXP places, SEXP cap)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    size_t r = zuf_format_f64_fixed((char *)buf, c, Rf_asReal(x), Rf_asInteger(places));
    return encode_result(r, buf, c);
}

/* zuf_format_f64_fixed for a vector, at the needed size. */
SEXP zufast_test_format_fixed_vec(SEXP x, SEXP places)
{
    R_xlen_t i, n = XLENGTH(x);
    int p = Rf_asInteger(places);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        size_t len = zuf_format_f64_fixed(NULL, 0, REAL(x)[i], p);
        char *buf = R_alloc(len, 1);
        zuf_format_f64_fixed(buf, len, REAL(x)[i], p);
        SET_STRING_ELT(out, i, Rf_mkCharLen(buf, (int)len));
    }
    UNPROTECT(1);
    return out;
}

/* Shortest formatting: kind 0 double, 1 float (x converted). */
SEXP zufast_test_format_shortest(SEXP x, SEXP flags, SEXP kind, SEXP cap)
{
    size_t c = (size_t)Rf_asReal(cap);
    unsigned char *buf = sentinel_buffer(c);
    uint32_t f = (uint32_t)Rf_asInteger(flags);
    size_t r = Rf_asInteger(kind) == 0
        ? zuf_format_f64_opt((char *)buf, c, Rf_asReal(x), f)
        : zuf_format_f32_opt((char *)buf, c, (float)Rf_asReal(x), f);
    return encode_result(r, buf, c);
}

/* Vectorised shortest float formatting (x converted to float). */
SEXP zufast_test_format_f32_vec(SEXP x, SEXP flags)
{
    R_xlen_t i, n = XLENGTH(x);
    uint32_t f = (uint32_t)Rf_asInteger(flags);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        char buf[ZUF_F32_MAX_CHARS];
        size_t len = zuf_format_f32_opt(buf, sizeof buf, (float)REAL(x)[i], f);
        SET_STRING_ELT(out, i, Rf_mkCharLen(buf, (int)len));
    }
    UNPROTECT(1);
    return out;
}

/* zuf_decimal_f64 for doubles given as 16-digit hex bit patterns:
   a character vector of "<negative> <mantissa> <exponent>". */
SEXP zufast_test_decimal_bits(SEXP hex)
{
    R_xlen_t i, n = XLENGTH(hex);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        uint64_t bits = 0;
        const char *h = CHAR(STRING_ELT(hex, i));
        double v;
        zuf_decimal d;
        char buf[64], *p = buf;
        int k;
        for (k = 0; k < 16; k++) {
            char ch = h[k];
            bits = bits * 16 + (uint64_t)(ch <= '9' ? ch - '0' : ch - 'a' + 10);
        }
        memcpy(&v, &bits, 8);
        d = zuf_decimal_f64(v);
        *p++ = d.negative ? '1' : '0';
        *p++ = ' ';
        p = zuf_write_u64(p, d.mantissa);
        *p++ = ' ';
        p = zuf_write_i32(p, d.exponent);
        SET_STRING_ELT(out, i, Rf_mkCharLen(buf, (int)(p - buf)));
    }
    UNPROTECT(1);
    return out;
}

/* (double)(float)x for each element. */
SEXP zufast_test_to_f32(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    for (i = 0; i < n; i++) REAL(out)[i] = (double)(float)REAL(x)[i];
    UNPROTECT(1);
    return out;
}

/* ---- hash.h ------------------------------------------------------------- */

/* xxHash's sanity buffer (tests/sanity_test.c, fillTestBuffer). */
#define SANITY_BUFFER_SIZE (4096 + 64 + 1)

static const unsigned char *sanity_buffer(void)
{
    static unsigned char buf[SANITY_BUFFER_SIZE];
    static int ready = 0;
    if (!ready) {
        uint64_t gen = 2654435761U;
        size_t i;
        for (i = 0; i < SANITY_BUFFER_SIZE; i++) {
            buf[i] = (unsigned char)(gen >> 56);
            gen *= UINT64_C(11400714785074694797);
        }
        ready = 1;
    }
    return buf;
}

static uint64_t parse_hex64(const char *h)
{
    uint64_t v = 0;
    for (; *h; h++) v = v * 16 + (uint64_t)(*h <= '9' ? *h - '0' : (*h | 0x20) - 'a' + 10);
    return v;
}

/* Hash a prefix of the sanity buffer for each (len, seed): kind 64 gives
   "<hash>", kind 128 gives "<low> <high>", in lower-case hex. */
SEXP zufast_test_xxh3_vectors(SEXP len, SEXP seed, SEXP kind)
{
    R_xlen_t i, n = XLENGTH(len);
    int k = Rf_asInteger(kind);
    const unsigned char *buf = sanity_buffer();
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        size_t l = (size_t)INTEGER(len)[i];
        uint64_t s = parse_hex64(CHAR(STRING_ELT(seed, i)));
        char text[40];
        if (k == 64) {
            hex64(text, zuf_hash64_seed(buf, l, s));
        } else {
            zuf_digest128 h = zuf_hash128_seed(buf, l, s);
            hex64(text, h.low);
            text[16] = ' ';
            hex64(text + 17, h.high);
        }
        SET_STRING_ELT(out, i, Rf_mkChar(text));
    }
    UNPROTECT(1);
    return out;
}

static int same128(zuf_digest128 a, zuf_digest128 b)
{
    return a.low == b.low && a.high == b.high;
}

/* For every length 0..maxlen: the zufast one-shot equals XXH3 called
   directly, the unseeded functions equal seed 0, and streaming equals
   one-shot at every split point (and in single bytes); then the same for a
   1 MiB buffer at a few split points. Seeds 0 and a non-zero seed. Returns
   the number of failures. */
SEXP zufast_test_xxh3_streaming(SEXP maxlen)
{
    size_t max = (size_t)Rf_asInteger(maxlen), l, split, i;
    const uint64_t seeds[2] = {0, UINT64_C(0x9E3779B185EBCA8D)};
    size_t big = (size_t)1 << 20;
    unsigned char *mem = (unsigned char *)R_alloc(big, 1);
    double bad = 0;
    int si;
    {
        uint64_t gen = 1;
        for (i = 0; i < big; i++) { gen = gen * UINT64_C(6364136223846793005) + 1; mem[i] = (unsigned char)(gen >> 33); }
    }
    for (si = 0; si < 2; si++) {
        uint64_t seed = seeds[si];
        for (l = 0; l <= max; l++) {
            uint64_t h64 = zuf_hash64_seed(mem, l, seed);
            zuf_digest128 h128 = zuf_hash128_seed(mem, l, seed);
            XXH128_hash_t direct = XXH3_128bits_withSeed(mem, l, seed);
            bad += h64 != XXH3_64bits_withSeed(mem, l, seed);
            bad += h128.low != direct.low64 || h128.high != direct.high64;
            if (seed == 0) {
                bad += zuf_hash64(mem, l) != h64;
                bad += !same128(zuf_hash128(mem, l), h128);
            }
            for (split = 0; split <= l; split++) {
                zuf_hasher h;
                zuf_hasher_init(&h, seed);
                zuf_hasher_update(&h, mem, split);
                zuf_hasher_update(&h, mem + split, l - split);
                bad += zuf_hasher_digest64(&h) != h64;
                bad += !same128(zuf_hasher_digest128(&h), h128);
            }
            {
                zuf_hasher h;
                zuf_hasher_init(&h, seed);
                for (i = 0; i < l; i++) zuf_hasher_update(&h, mem + i, 1);
                bad += zuf_hasher_digest64(&h) != h64;
            }
        }
        {
            const size_t splits[] = {0, 1, 63, 64, 65, 1023, 1024, 4096, 65537, 999999};
            uint64_t h64 = zuf_hash64_seed(mem, big, seed);
            zuf_digest128 h128 = zuf_hash128_seed(mem, big, seed);
            bad += h64 != XXH3_64bits_withSeed(mem, big, seed);
            for (i = 0; i < sizeof splits / sizeof splits[0]; i++) {
                zuf_hasher h;
                size_t a = splits[i], b = a + (big - a) / 3;
                zuf_hasher_init(&h, seed);
                zuf_hasher_update(&h, mem, a);
                zuf_hasher_update(&h, mem + a, b - a);
                zuf_hasher_update(&h, mem + b, big - b);
                bad += zuf_hasher_digest64(&h) != h64;
                bad += !same128(zuf_hasher_digest128(&h), h128);
            }
        }
    }
    return Rf_ScalarReal(bad);
}
