/* Declarations of the .Call entry points registered in init.c. */
#ifndef ZUFAST_R_H
#define ZUFAST_R_H

#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

/* zufast_r.c */
SEXP zufast_info(void);

/* zufast_test.c */
SEXP zufast_test_status_string(SEXP status);
SEXP zufast_test_mul128(SEXP a, SEXP b);

#endif
