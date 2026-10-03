#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

#include "zufast_r.h"

#define CALLDEF(name, n) {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef call_methods[] = {
    CALLDEF(zufast_info, 0),
    CALLDEF(zufast_utf8_valid, 1),
    CALLDEF(zufast_encode, 3),
    CALLDEF(zufast_decode, 3),
    CALLDEF(zufast_parse_date, 1),
    CALLDEF(zufast_parse_datetime, 1),
    CALLDEF(zufast_datetime_fields, 1),
    CALLDEF(zufast_format_datetime, 2),
    CALLDEF(zufast_format_date, 1),
    CALLDEF(zufast_parse_double, 1),
    CALLDEF(zufast_parse_integer, 1),
    CALLDEF(zufast_format_double, 2),
    CALLDEF(zufast_test_status_string, 1),
    CALLDEF(zufast_test_mul128, 2),
    CALLDEF(zufast_test_parse_bool, 2),
    CALLDEF(zufast_test_equals, 3),
    CALLDEF(zufast_test_trim, 1),
    CALLDEF(zufast_test_half_decode, 2),
    CALLDEF(zufast_test_half_encode, 2),
    CALLDEF(zufast_test_half_encode_bits, 2),
    CALLDEF(zufast_test_fits, 2),
    CALLDEF(zufast_test_endian, 2),
    CALLDEF(zufast_test_utf8_exhaustive, 0),
    CALLDEF(zufast_test_utf8_count, 1),
    CALLDEF(zufast_test_hex_encode, 3),
    CALLDEF(zufast_test_hex_decode, 2),
    CALLDEF(zufast_test_base64_encode, 3),
    CALLDEF(zufast_test_base64_decode, 3),
    CALLDEF(zufast_test_base64_bounds, 1),
    CALLDEF(zufast_test_parse_uuid, 1),
    CALLDEF(zufast_test_format_uuid, 3),
    CALLDEF(zufast_test_parse_datetime, 2),
    CALLDEF(zufast_test_format_datetime, 2),
    CALLDEF(zufast_test_format_date, 2),
    CALLDEF(zufast_test_civil_from_days, 1),
    CALLDEF(zufast_test_days_from_civil, 3),
    CALLDEF(zufast_test_year_info, 1),
    CALLDEF(zufast_test_calendar_walk, 3),
    CALLDEF(zufast_test_parse_num, 5),
    CALLDEF(zufast_test_parse_vs_strtod, 1),
    CALLDEF(zufast_test_write_i32, 1),
    CALLDEF(zufast_test_write_ints, 0),
    CALLDEF(zufast_test_format_fixed, 3),
    CALLDEF(zufast_test_format_fixed_vec, 2),
    CALLDEF(zufast_test_format_shortest, 4),
    CALLDEF(zufast_test_format_f32_vec, 2),
    CALLDEF(zufast_test_decimal_bits, 1),
    CALLDEF(zufast_test_to_f32, 1),
    {NULL, NULL, 0}
};

/* The one exported symbol of zufast.so. Nothing is registered with
   R_RegisterCCallable: consumers include the headers instead (design 4). */
void attribute_visible R_init_zufast(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
