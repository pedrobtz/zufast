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
