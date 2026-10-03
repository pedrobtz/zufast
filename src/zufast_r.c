/* The .Call wrappers behind R/. They include <zufast.h> like any consumer. */
#include <zufast.h>

#include "zufast_r.h"

SEXP zufast_info(void)
{
    const char *names[] = {"version", "version_major", "version_minor",
                           "version_patch", "compiler", ""};
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
