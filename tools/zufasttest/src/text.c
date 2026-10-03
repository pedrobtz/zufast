/* Translation unit 1: text <-> values. Includes everything, as the
   consumer recipe in zufast's README says. */
#include <zufast.h>
#include "zufasttest.h"

static SEXP str(const char *s, size_t n) { return Rf_mkCharLen(s, (int)n); }

/* Takes a string; returns a character vector of results, one per public
   function of the number, datetime and literal areas. */
SEXP zt_text(SEXP x)
{
    const char *first = CHAR(STRING_ELT(x, 0));
    const char *last = first + LENGTH(STRING_ELT(x, 0));
    char buf[512];
    char *p;
    SEXP out = PROTECT(Rf_allocVector(STRSXP, 32));
    int k = 0;
    zuf_num_options opt;
    double d;
    float f;
    int64_t i64;
    uint64_t u64;
    int32_t i32;
    uint32_t u32;
    bool b;
    zuf_datetime dt;
    zuf_timestamp ts;
    int32_t y;
    uint32_t m, day;
    const char *tf, *tl;
    static const char num[] = "12345";
    static const char when[] = "2024-02-29T12:30:45.5+01:00";

    opt.flags = ZUF_NUM_LEADING_PLUS | ZUF_NUM_SKIP_SPACE;
    opt.base = 16;
    opt.decimal_point = '.';

    /* status.h, version.h */
    SET_STRING_ELT(out, k++, Rf_mkChar(zuf_status_string(ZUF_ERR_RANGE)));
    SET_STRING_ELT(out, k++, Rf_mkChar(ZUFAST_VERSION));
    /* number.h: parsing the argument */
    (void)zuf_parse_f64(first, last, &d);
    SET_STRING_ELT(out, k++, str(buf, zuf_format_f64(buf, sizeof buf, d)));
    (void)zuf_parse_f32(first, last, &f);
    SET_STRING_ELT(out, k++, str(buf, zuf_format_f32(buf, sizeof buf, f)));
    (void)zuf_parse_f64_opt(first, last, &d, &opt);
    SET_STRING_ELT(out, k++, str(buf, zuf_format_f64_opt(buf, sizeof buf, d, ZUF_FMT_SCIENTIFIC)));
    (void)zuf_parse_f32_opt(first, last, &f, &opt);
    SET_STRING_ELT(out, k++, str(buf, zuf_format_f32_opt(buf, sizeof buf, f, ZUF_FMT_TRAILING_ZERO)));
    SET_STRING_ELT(out, k++, str(buf, zuf_format_f64_fixed(buf, sizeof buf, d, 3)));
    {
        zuf_decimal dec = zuf_decimal_f64(d);
        zuf_decimal dec32 = zuf_decimal_f32(f);
        p = zuf_write_u64(buf, dec.mantissa);
        *p++ = 'e';
        p = zuf_write_i32(p, dec.exponent);
        *p++ = '/';
        p = zuf_write_u64(p, dec32.mantissa);
        *p++ = dec.negative ? '-' : '+';
        SET_STRING_ELT(out, k++, str(buf, (size_t)(p - buf)));
    }
    /* integers on a fixed input, through every type and the options */
    (void)zuf_parse_i64(num, num + 5, &i64);
    (void)zuf_parse_u64(num, num + 5, &u64);
    (void)zuf_parse_i32(num, num + 5, &i32);
    (void)zuf_parse_u32(num, num + 5, &u32);
    p = zuf_write_i64(buf, i64); *p++ = ' ';
    p = zuf_write_u64(p, u64); *p++ = ' ';
    p = zuf_write_i32(p, i32); *p++ = ' ';
    p = zuf_write_u32(p, u32);
    SET_STRING_ELT(out, k++, str(buf, (size_t)(p - buf)));
    (void)zuf_parse_i64_opt(num, num + 5, &i64, &opt);
    (void)zuf_parse_u64_opt(num, num + 5, &u64, &opt);
    (void)zuf_parse_i32_opt(num, num + 5, &i32, &opt);
    (void)zuf_parse_u32_opt(num, num + 5, &u32, &opt);
    p = zuf_write_i64(buf, i64); *p++ = ' ';
    p = zuf_write_u64(p, u64); *p++ = ' ';
    p = zuf_write_i32(p, i32); *p++ = ' ';
    p = zuf_write_u32(p, u32);
    SET_STRING_ELT(out, k++, str(buf, (size_t)(p - buf)));
    /* datetime.h */
    (void)zuf_parse_datetime(when, when + sizeof when - 1, &dt);
    SET_STRING_ELT(out, k++, str(buf, zuf_format_datetime(buf, sizeof buf, &dt)));
    ts = zuf_datetime_timestamp(&dt);
    p = zuf_write_i64(buf, ts.seconds); *p++ = '.';
    p = zuf_write_u32(p, ts.nanoseconds);
    SET_STRING_ELT(out, k++, str(buf, (size_t)(p - buf)));
    SET_STRING_ELT(out, k++, str(buf, zuf_format_date(buf, sizeof buf, zuf_datetime_days(&dt))));
    (void)zuf_parse_date(when, when + ZUF_DATE_CHARS, &dt);
    zuf_civil_from_days(zuf_days_from_civil(dt.year, dt.month, dt.day) + 1, &y, &m, &day);
    p = zuf_write_i32(buf, y); *p++ = ' ';
    p = zuf_write_u32(p, m); *p++ = ' ';
    p = zuf_write_u32(p, day); *p++ = ' ';
    p = zuf_write_u32(p, zuf_days_in_month(y, 2)); *p++ = ' ';
    *p++ = zuf_is_leap_year(y) ? 'L' : 'N';
    SET_STRING_ELT(out, k++, str(buf, (size_t)(p - buf)));
    /* literal.h, on the argument */
    tf = first; tl = last;
    zuf_trim_space(&tf, &tl);
    SET_STRING_ELT(out, k++, str(tf, (size_t)(tl - tf)));
    SET_STRING_ELT(out, k++, Rf_mkChar(zuf_skip_space(first, last) == first ? "no-space" : "space"));
    {
        zuf_result r = zuf_parse_bool(tf, tl, ZUF_BOOL_R | ZUF_BOOL_YESNO | ZUF_BOOL_DIGIT, &b);
        SET_STRING_ELT(out, k++, Rf_mkChar(r.status ? "not-bool" : b ? "true" : "false"));
    }
    SET_STRING_ELT(out, k++, Rf_mkChar(zuf_equals(tf, tl, "NA", 2) ? "NA" :
                                       zuf_equals_ci(tf, tl, "null", 4) ? "null" : "other"));
    out = Rf_lengthgets(out, k);
    UNPROTECT(1);
    return out;
}
