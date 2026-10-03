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
