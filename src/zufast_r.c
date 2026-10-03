/* The .Call wrappers behind R/. They include <zufast.h> like any consumer. */
#include <math.h>
#include <stdio.h>

#include <zufast.h>

#include "zufast_r.h"

SEXP zufast_info(void)
{
    const char *names[] = {"version", "version_major", "version_minor",
                           "version_patch", "compiler", "vendored", ""};
    const char *vendor_names[] = {"ffc", "ryu", "xxhash", ""};
    SEXP vendored;
    SEXP out = PROTECT(Rf_mkNamed(VECSXP, names));
    SET_VECTOR_ELT(out, 0, Rf_mkString(ZUFAST_VERSION));
    SET_VECTOR_ELT(out, 1, Rf_ScalarInteger(ZUFAST_VERSION_MAJOR));
    SET_VECTOR_ELT(out, 2, Rf_ScalarInteger(ZUFAST_VERSION_MINOR));
    SET_VECTOR_ELT(out, 3, Rf_ScalarInteger(ZUFAST_VERSION_PATCH));
#if defined(__clang__)
    SET_VECTOR_ELT(out, 4, Rf_mkString("clang " __clang_version__));
#elif defined(__GNUC__)
    SET_VECTOR_ELT(out, 4, Rf_mkString("gcc " __VERSION__));
#else
    SET_VECTOR_ELT(out, 4, Rf_mkString("unknown"));
#endif
    vendored = Rf_mkNamed(STRSXP, vendor_names);
    SET_VECTOR_ELT(out, 5, vendored);
    SET_STRING_ELT(vendored, 0, Rf_mkChar(ZUF_INT_FFC_VERSION_STRING));
    SET_STRING_ELT(vendored, 1, Rf_mkChar(ZUF_INT_RYU_VERSION));
    {
        char v[16];
        int xxh = ZUF_INT_XXH_VERSION_NUMBER;
        snprintf(v, sizeof v, "%d.%d.%d", xxh / 10000, xxh / 100 % 100, xxh % 100);
        SET_STRING_ELT(vendored, 2, Rf_mkChar(v));
    }
    UNPROTECT(1);
    return out;
}

/* fast_utf8_valid(): character -> logical per element (NA for NA); raw ->
   a single logical for the whole vector. */
SEXP zufast_utf8_valid(SEXP x)
{
    if (TYPEOF(x) == RAWSXP)
        return Rf_ScalarLogical(zuf_utf8_valid((const char *)RAW(x), (size_t)XLENGTH(x)));
    {
        R_xlen_t i, n = XLENGTH(x);
        SEXP out = PROTECT(Rf_allocVector(LGLSXP, n));
        int *o = LOGICAL(out);
        for (i = 0; i < n; i++) {
            SEXP s = STRING_ELT(x, i);
            o[i] = s == NA_STRING ? NA_LOGICAL
                                  : (int)zuf_utf8_valid(CHAR(s), (size_t)LENGTH(s));
        }
        UNPROTECT(1);
        return out;
    }
}

/* Encode raw -> one string, or each element of a character vector. `kind`
   0 hex (flags: upper), 1 base64 (flags: ZUF_B64_*). */
static SEXP encode_one(const char *src, size_t n, int kind, uint32_t flags)
{
    size_t need = kind == 0 ? zuf_hex_encode(src, n, NULL, 0, flags != 0)
                            : zuf_base64_encode(src, n, NULL, 0, flags);
    char *buf = R_alloc(need + 1, 1);
    if (kind == 0) zuf_hex_encode(src, n, buf, need, flags != 0);
    else zuf_base64_encode(src, n, buf, need, flags);
    return Rf_mkCharLenCE(buf, (int)need, CE_UTF8);
}

SEXP zufast_encode(SEXP x, SEXP kind, SEXP flags)
{
    int k = Rf_asInteger(kind);
    uint32_t f = (uint32_t)Rf_asInteger(flags);
    if (TYPEOF(x) == RAWSXP) {
        SEXP out = PROTECT(Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(out, 0, encode_one((const char *)RAW(x), (size_t)XLENGTH(x), k, f));
        UNPROTECT(1);
        return out;
    }
    {
        R_xlen_t i, n = XLENGTH(x);
        SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
        const void *vmax = vmaxget();
        for (i = 0; i < n; i++) {
            SEXP s = STRING_ELT(x, i);
            SET_STRING_ELT(out, i, s == NA_STRING ? NA_STRING
                           : encode_one(CHAR(s), (size_t)LENGTH(s), k, f));
            vmaxset(vmax);   /* release encode_one's R_alloc buffer */
        }
        UNPROTECT(1);
        return out;
    }
}

/* Decode each element of a character vector to a raw vector; NULL where the
   element is NA or not valid. */
SEXP zufast_decode(SEXP x, SEXP kind, SEXP flags)
{
    int k = Rf_asInteger(kind);
    uint32_t f = (uint32_t)Rf_asInteger(flags);
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(VECSXP, n));
    for (i = 0; i < n; i++) {
        SEXP s = STRING_ELT(x, i);
        const char *first, *last;
        size_t cap, len = 0;
        zuf_status st;
        SEXP raw;
        if (s == NA_STRING) continue;
        first = CHAR(s);
        last = first + LENGTH(s);
        cap = k == 0 ? (size_t)LENGTH(s) / 2 : zuf_base64_decode_bound((size_t)LENGTH(s));
        raw = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)cap));
        st = k == 0 ? zuf_hex_decode(first, last, RAW(raw), cap, &len)
                    : zuf_base64_decode(first, last, RAW(raw), cap, &len, f);
        if (st == ZUF_OK) {
            if (len != cap) raw = Rf_xlengthgets(raw, (R_xlen_t)len);
            SET_VECTOR_ELT(out, i, raw);
        }
        UNPROTECT(1);
    }
    UNPROTECT(1);
    return out;
}

/* ---- dates -------------------------------------------------------------- */

/* Parse the whole of a CHARSXP; false unless every byte is consumed. */
static bool parse_whole(SEXP s, bool date_only, zuf_datetime *dt)
{
    const char *first, *last;
    zuf_result r;
    if (s == NA_STRING) return false;
    first = CHAR(s);
    last = first + LENGTH(s);
    r = date_only ? zuf_parse_date(first, last, dt) : zuf_parse_datetime(first, last, dt);
    return r.status == ZUF_OK && r.ptr == last;
}

SEXP zufast_parse_date(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    double *o = REAL(out);
    for (i = 0; i < n; i++) {
        zuf_datetime dt;
        o[i] = parse_whole(STRING_ELT(x, i), true, &dt) ? (double)zuf_datetime_days(&dt) : NA_REAL;
    }
    UNPROTECT(1);
    return out;
}

SEXP zufast_parse_datetime(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    double *o = REAL(out);
    for (i = 0; i < n; i++) {
        zuf_datetime dt;
        if (parse_whole(STRING_ELT(x, i), false, &dt)) {
            zuf_timestamp t = zuf_datetime_timestamp(&dt);
            o[i] = (double)t.seconds + (double)t.nanoseconds / 1e9;
        } else {
            o[i] = NA_REAL;
        }
    }
    UNPROTECT(1);
    return out;
}

/* A list of columns: year, month, day, hour, minute, second, nanosecond,
   offset, has_time, has_offset. */
SEXP zufast_datetime_fields(SEXP x)
{
    const char *names[] = {"year", "month", "day", "hour", "minute", "second",
                           "nanosecond", "offset", "has_time", "has_offset", ""};
    R_xlen_t i, n = XLENGTH(x);
    int k;
    SEXP out = PROTECT(Rf_mkNamed(VECSXP, names));
    for (k = 0; k < 6; k++) SET_VECTOR_ELT(out, k, Rf_allocVector(INTSXP, n));
    SET_VECTOR_ELT(out, 6, Rf_allocVector(INTSXP, n));
    SET_VECTOR_ELT(out, 7, Rf_allocVector(INTSXP, n));
    SET_VECTOR_ELT(out, 8, Rf_allocVector(LGLSXP, n));
    SET_VECTOR_ELT(out, 9, Rf_allocVector(LGLSXP, n));
    for (i = 0; i < n; i++) {
        zuf_datetime dt;
        int v[8];
        bool ok;
        memset(&dt, 0, sizeof dt);
        ok = parse_whole(STRING_ELT(x, i), false, &dt);
        v[0] = dt.year; v[1] = dt.month; v[2] = dt.day; v[3] = dt.hour;
        v[4] = dt.minute; v[5] = dt.second; v[6] = (int)dt.nanosecond;
        v[7] = dt.offset_seconds;
        for (k = 0; k < 8; k++) INTEGER(VECTOR_ELT(out, k))[i] = ok ? v[k] : NA_INTEGER;
        if (ok && !dt.has_offset) INTEGER(VECTOR_ELT(out, 7))[i] = NA_INTEGER;
        if (ok && !dt.has_time) {
            for (k = 3; k < 7; k++) INTEGER(VECTOR_ELT(out, k))[i] = NA_INTEGER;
        }
        LOGICAL(VECTOR_ELT(out, 8))[i] = ok ? dt.has_time : NA_LOGICAL;
        LOGICAL(VECTOR_ELT(out, 9))[i] = ok ? dt.has_offset : NA_LOGICAL;
    }
    UNPROTECT(1);
    return out;
}

/* Format seconds since the epoch (POSIXct) in UTC. digits < 0: microsecond
   resolution, shortest of 0, 3 or 6 fraction digits; 0-9: exactly that
   many, rounded to nearest. */
SEXP zufast_format_datetime(SEXP x, SEXP digits)
{
    static const double pow10[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};
    R_xlen_t i, n = XLENGTH(x);
    int dig = Rf_asInteger(digits);
    double scale = dig < 0 ? 1e6 : pow10[dig];
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        double v = REAL(x)[i], secs, units;
        int64_t total_units, isec, days, rem;
        int32_t y;
        uint32_t m, d, frac;
        zuf_datetime dt;
        char buf[ZUF_DATETIME_MAX_CHARS + 16];
        size_t len;
        /* beyond the int32 day range the calendar does not reach */
        if (!R_FINITE(v) || v > 1.8e14 || v < -1.8e14) {
            SET_STRING_ELT(out, i, NA_STRING);
            continue;
        }
        secs = floor(v);
        units = floor((v - secs) * scale + 0.5);
        total_units = (int64_t)units;
        isec = (int64_t)secs;
        if (total_units >= (int64_t)scale) { isec++; total_units -= (int64_t)scale; }
        days = isec >= 0 ? isec / 86400 : -((-isec + 86399) / 86400);
        rem = isec - days * 86400;
        zuf_civil_from_days((int32_t)days, &y, &m, &d);
        memset(&dt, 0, sizeof dt);
        dt.year = y; dt.month = (uint8_t)m; dt.day = (uint8_t)d;
        dt.hour = (uint8_t)(rem / 3600); dt.minute = (uint8_t)(rem / 60 % 60);
        dt.second = (uint8_t)(rem % 60);
        dt.has_time = true;
        dt.has_offset = true;
        frac = (uint32_t)total_units;
        if (dig < 0) {
            dt.nanosecond = frac * 1000u;
            len = zuf_format_datetime(buf, sizeof buf, &dt);
        } else {
            /* the seconds without a fraction, then exactly `dig` digits */
            int k;
            len = zuf_format_datetime(buf, sizeof buf, &dt) - 1;   /* drop the Z */
            if (dig > 0) {
                buf[len++] = '.';
                for (k = dig - 1; k >= 0; k--) {
                    buf[len + (size_t)k] = (char)('0' + frac % 10u);
                    frac /= 10u;
                }
                len += (size_t)dig;
            }
            buf[len++] = 'Z';
        }
        SET_STRING_ELT(out, i, Rf_mkCharLenCE(buf, (int)len, CE_UTF8));
    }
    UNPROTECT(1);
    return out;
}

SEXP zufast_format_date(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        double v = REAL(x)[i];
        char buf[ZUF_DATE_MAX_CHARS];
        size_t len;
        if (!R_FINITE(v) || v > 2147483647.0 || v < -2147483648.0) {
            SET_STRING_ELT(out, i, NA_STRING);
            continue;
        }
        len = zuf_format_date(buf, sizeof buf, (int32_t)floor(v));
        SET_STRING_ELT(out, i, Rf_mkCharLenCE(buf, (int)len, CE_UTF8));
    }
    UNPROTECT(1);
    return out;
}

/* ---- numbers ------------------------------------------------------------ */

/* The R-facing grammar (design 8.5): a leading '+' is accepted and ASCII
   whitespace is trimmed from both ends; everything else must be consumed. */
SEXP zufast_parse_double(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    double *o = REAL(out);
    zuf_num_options opt;
    opt.flags = ZUF_NUM_LEADING_PLUS;
    opt.base = 10;
    opt.decimal_point = '.';
    for (i = 0; i < n; i++) {
        SEXP s = STRING_ELT(x, i);
        const char *first, *last;
        double v;
        zuf_result r;
        o[i] = NA_REAL;
        if (s == NA_STRING) continue;
        first = CHAR(s);
        last = first + LENGTH(s);
        zuf_trim_space(&first, &last);
        r = zuf_parse_f64_opt(first, last, &v, &opt);
        if (r.status != ZUF_ERR_INVALID && r.ptr == last) o[i] = v;
    }
    UNPROTECT(1);
    return out;
}

/* NA for anything that is not a whole decimal integer in R's integer range
   (NA_integer_ itself, INT_MIN, is out of range). */
SEXP zufast_parse_integer(SEXP x)
{
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(INTSXP, n));
    int *o = INTEGER(out);
    zuf_num_options opt;
    opt.flags = ZUF_NUM_LEADING_PLUS;
    opt.base = 10;
    opt.decimal_point = '.';
    for (i = 0; i < n; i++) {
        SEXP s = STRING_ELT(x, i);
        const char *first, *last;
        int32_t v;
        zuf_result r;
        o[i] = NA_INTEGER;
        if (s == NA_STRING) continue;
        first = CHAR(s);
        last = first + LENGTH(s);
        zuf_trim_space(&first, &last);
        r = zuf_parse_i32_opt(first, last, &v, &opt);
        if (r.status == ZUF_OK && r.ptr == last && v != INT32_MIN) o[i] = v;
    }
    UNPROTECT(1);
    return out;
}

SEXP zufast_format_double(SEXP x, SEXP flags)
{
    R_xlen_t i, n = XLENGTH(x);
    uint32_t f = (uint32_t)Rf_asInteger(flags);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        double v = REAL(x)[i];
        char buf[ZUF_F64_MAX_CHARS];
        size_t len;
        if (ISNA(v)) {
            SET_STRING_ELT(out, i, NA_STRING);
            continue;
        }
        len = zuf_format_f64_opt(buf, sizeof buf, v, f);
        SET_STRING_ELT(out, i, Rf_mkCharLenCE(buf, (int)len, CE_UTF8));
    }
    UNPROTECT(1);
    return out;
}

/* ---- hashing ------------------------------------------------------------ */

static void hex64_be(char *dst, uint64_t v)
{
    static const char digits[] = "0123456789abcdef";
    int i;
    for (i = 15; i >= 0; i--) { dst[i] = digits[v & 15]; v >>= 4; }
}

/* XXH3 as lower-case hex in xxHash's canonical (big-endian) form: 16 digits
   for 64 bits, 32 (high half first) for 128. */
static SEXP hash_one(const void *data, size_t n, int bits, uint64_t seed)
{
    char buf[32];
    if (bits == 64) {
        hex64_be(buf, zuf_hash64_seed(data, n, seed));
        return Rf_mkCharLen(buf, 16);
    } else {
        zuf_digest128 h = zuf_hash128_seed(data, n, seed);
        hex64_be(buf, h.high);
        hex64_be(buf + 16, h.low);
        return Rf_mkCharLen(buf, 32);
    }
}

SEXP zufast_hash(SEXP x, SEXP bits, SEXP seed)
{
    int b = Rf_asInteger(bits);
    uint64_t s = (uint64_t)Rf_asReal(seed);
    if (TYPEOF(x) == RAWSXP) {
        SEXP out = PROTECT(Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(out, 0, hash_one(RAW(x), (size_t)XLENGTH(x), b, s));
        UNPROTECT(1);
        return out;
    }
    {
        R_xlen_t i, n = XLENGTH(x);
        SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
        for (i = 0; i < n; i++) {
            SEXP e = STRING_ELT(x, i);
            SET_STRING_ELT(out, i, e == NA_STRING ? NA_STRING
                           : hash_one(CHAR(e), (size_t)LENGTH(e), b, s));
        }
        UNPROTECT(1);
        return out;
    }
}
