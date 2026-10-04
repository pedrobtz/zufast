/* Declarations of the .Call entry points registered in init.c. */
#ifndef ZUFAST_R_H
#define ZUFAST_R_H

#include <stddef.h>
#include <stdint.h>

#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

/* zufast_r.c */
SEXP zufast_info(void);
SEXP zufast_utf8_valid(SEXP x);
SEXP zufast_encode(SEXP x, SEXP kind, SEXP flags);
SEXP zufast_decode(SEXP x, SEXP kind, SEXP flags);
SEXP zufast_parse_date(SEXP x);
SEXP zufast_parse_datetime(SEXP x);
SEXP zufast_datetime_fields(SEXP x);
SEXP zufast_format_datetime(SEXP x, SEXP digits);
SEXP zufast_format_date(SEXP x);
SEXP zufast_parse_double(SEXP x);
SEXP zufast_parse_integer(SEXP x);
SEXP zufast_format_double(SEXP x, SEXP flags);
SEXP zufast_hash(SEXP x, SEXP bits, SEXP seed);
size_t zufast_encoded_len(size_t n, int kind, uint32_t flags);

/* zufast_test.c */
SEXP zufast_test_status_string(SEXP status);
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
SEXP zufast_test_base64_limits(void);
SEXP zufast_test_encoded_len(SEXP n, SEXP kind, SEXP flags);
SEXP zufast_test_parse_uuid(SEXP raw);
SEXP zufast_test_format_uuid(SEXP raw, SEXP cap, SEXP upper);
SEXP zufast_test_parse_datetime(SEXP raw, SEXP date_only);
SEXP zufast_test_format_datetime(SEXP fields, SEXP cap);
SEXP zufast_test_format_date(SEXP days, SEXP cap);
SEXP zufast_test_civil_from_days(SEXP days);
SEXP zufast_test_days_from_civil(SEXP y, SEXP m, SEXP d);
SEXP zufast_test_year_info(SEXP year);
SEXP zufast_test_calendar_walk(SEXP lo, SEXP hi, SEXP step);
SEXP zufast_test_parse_num(SEXP raw, SEXP kind, SEXP flags, SEXP base, SEXP decimal_point);
SEXP zufast_test_parse_vs_strtod(SEXP x);
SEXP zufast_test_write_i32(SEXP x);
SEXP zufast_test_write_ints(void);
SEXP zufast_test_format_fixed(SEXP x, SEXP places, SEXP cap);
SEXP zufast_test_format_fixed_vec(SEXP x, SEXP places);
SEXP zufast_test_format_shortest(SEXP x, SEXP flags, SEXP kind, SEXP cap);
SEXP zufast_test_format_f32_vec(SEXP x, SEXP flags);
SEXP zufast_test_decimal_bits(SEXP hex);
SEXP zufast_test_to_f32(SEXP x);
SEXP zufast_test_xxh3_vectors(SEXP len, SEXP seed, SEXP kind);
SEXP zufast_test_xxh3_streaming(SEXP maxlen);

#endif
