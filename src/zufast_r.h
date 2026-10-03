/* Declarations of the .Call entry points registered in init.c. */
#ifndef ZUFAST_R_H
#define ZUFAST_R_H

#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

/* zufast_r.c */
SEXP zufast_info(void);
SEXP zufast_utf8_valid(SEXP x);
SEXP zufast_encode(SEXP x, SEXP kind, SEXP flags);
SEXP zufast_decode(SEXP x, SEXP kind, SEXP flags);

/* zufast_test.c */
SEXP zufast_test_status_string(SEXP status);
SEXP zufast_test_mul128(SEXP a, SEXP b);
SEXP zufast_test_parse_bool(SEXP x, SEXP accept);
SEXP zufast_test_equals(SEXP x, SEXP lit, SEXP ci);
SEXP zufast_test_trim(SEXP x);
SEXP zufast_test_half_decode(SEXP bits, SEXP kind);
SEXP zufast_test_half_encode(SEXP x, SEXP kind);
SEXP zufast_test_half_encode_bits(SEXP bits, SEXP kind);
SEXP zufast_test_fits(SEXP x, SEXP width);
SEXP zufast_test_endian(SEXP raw, SEXP offset);
SEXP zufast_test_utf8_exhaustive(void);
SEXP zufast_test_utf8_count(SEXP raw);
SEXP zufast_test_hex_encode(SEXP raw, SEXP cap, SEXP upper);
SEXP zufast_test_hex_decode(SEXP raw, SEXP cap);
SEXP zufast_test_base64_encode(SEXP raw, SEXP cap, SEXP flags);
SEXP zufast_test_base64_decode(SEXP raw, SEXP cap, SEXP flags);
SEXP zufast_test_base64_bounds(SEXP n);
SEXP zufast_test_parse_uuid(SEXP raw);
SEXP zufast_test_format_uuid(SEXP raw, SEXP cap, SEXP upper);

#endif
