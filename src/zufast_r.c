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
