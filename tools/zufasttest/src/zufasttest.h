#ifndef ZUFASTTEST_H
#define ZUFASTTEST_H
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
SEXP zt_text(SEXP x);       /* text.c */
SEXP zt_binary(SEXP x);     /* binary.c */
#endif
