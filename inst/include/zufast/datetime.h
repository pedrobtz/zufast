/*
 * zufast/datetime.h -- ISO 8601 / RFC 3339 dates and timestamps (design 10).
 *
 * Parsed forms:
 *
 *   YYYY-MM-DD
 *   YYYY-MM-DDTHH:MM
 *   YYYY-MM-DDTHH:MM:SS
 *   YYYY-MM-DDTHH:MM:SS.fff         1 to 9 fraction digits; further digits
 *                                   are validated and discarded
 *   ...Z  ...z  ...+HH:MM  ...+HHMM  ...+HH   (or '-'), after any time form
 *
 * with 'T', 't' or a single space between date and time. Validation is
 * complete: year 0000-9999, month 1-12, day valid for the month in the
 * proleptic Gregorian calendar, hour 0-23, minute 0-59, second 0-60 (RFC
 * 3339's leap second), offset within +-23:59. 24:00:00, comma fractions,
 * week and ordinal dates, expanded years and the basic format are rejected.
 *
 * ptr semantics follow std::from_chars: on ZUF_OK, r.ptr is one past the
 * last byte consumed; on ZUF_ERR_INVALID it equals `first`. On
 * ZUF_ERR_INCOMPLETE (the input ends inside a value, as in "2024-01") it
 * equals `last`. A date followed by a byte that cannot start a time (for
 * example "2024-01-01,") parses as a bare date with r.ptr after the day;
 * once a separator is followed by a digit, the time must be well formed.
 *
 * The parser returns fields, not an epoch: has_offset distinguishes "no
 * offset given" from "UTC", and the conversion helpers say what they assume.
 *
 * Calendar arithmetic uses the Euclidean affine functions of Neri and
 * Schneider (2022), "Euclidean affine functions and their application to
 * calendar algorithms", with days counted from 1970-01-01. They are exact
 * for every int32_t day count.
 *
 * Nothing here uses time.h, the locale, errno, allocation or global state;
 * every function may be called from any thread.
 */
#ifndef ZUFAST_DATETIME_H
#define ZUFAST_DATETIME_H

#include "status.h"
#include "detail/digits.h"

typedef struct {
    int32_t  year;
    uint8_t  month, day;
    uint8_t  hour, minute, second;
    uint32_t nanosecond;
    int32_t  offset_seconds;   /* meaningful only when has_offset */
    bool     has_time;         /* false for a bare date */
    bool     has_offset;       /* Z or a numeric offset was present */
} zuf_datetime;

typedef struct { int64_t seconds; uint32_t nanoseconds; } zuf_timestamp;

#define ZUF_DATE_CHARS          10   /* "YYYY-MM-DD", years 0000-9999 */
#define ZUF_DATETIME_MAX_CHARS  42   /* any int32_t year: "-2147483648-MM-DDTHH:MM:SS.nnnnnnnnn+HH:MM";
                                         35 for years 0000-9999 */
#define ZUF_DATE_MAX_CHARS      14   /* any int32_t day count: "+5881580-07-11" */

/* ---- calendar ----------------------------------------------------------- */

/* Shift so that every int32_t day count maps to a non-negative value:
   s = 14704 400-year eras. */
#define ZUF_INT_CAL_ERAS  UINT64_C(14704)
#define ZUF_INT_CAL_K     (UINT64_C(719468) + UINT64_C(146097) * ZUF_INT_CAL_ERAS)
#define ZUF_INT_CAL_L     (UINT64_C(400) * ZUF_INT_CAL_ERAS)

ZUF_INLINE bool zuf_is_leap_year(int32_t year)
{
    return (year & 3) == 0 && (year % 100 != 0 || year % 400 == 0);
}

/* Days in `month` (1-12) of `year`; 0 for a month outside 1-12. */
ZUF_INLINE uint32_t zuf_days_in_month(int32_t year, uint32_t month)
{
    if (month < 1 || month > 12) return 0;
    if (month == 2) return zuf_is_leap_year(year) ? 29u : 28u;
    return 30u + ((month + (month >> 3)) & 1u);
}

/* Days from 1970-01-01 to year-month-day, proleptic Gregorian. month and day
   must be valid for the year, and the result must fit int32_t (years
   -5877641 to 5881580). */
ZUF_INLINE int32_t zuf_days_from_civil(int32_t year, uint32_t month, uint32_t day)
{
    uint64_t j = month <= 2;
    uint64_t y = (uint64_t)((int64_t)year + (int64_t)ZUF_INT_CAL_L) - j;
    uint64_t m = j ? month + 12u : month;
    uint64_t c = y / 100;
    uint64_t y_star = 1461 * y / 4 - c + c / 4;
    uint64_t m_star = (979 * m - 2919) / 32;
    uint64_t n = y_star + m_star + (day - 1u);
    return (int32_t)((int64_t)n - (int64_t)ZUF_INT_CAL_K);
}

/* The civil date of a day count from 1970-01-01; exact for every int32_t. */
ZUF_INLINE void zuf_civil_from_days(int32_t days, int32_t *year, uint32_t *month, uint32_t *day)
{
    uint64_t n = (uint64_t)((int64_t)days + (int64_t)ZUF_INT_CAL_K);
    /* century */
    uint64_t n1 = 4 * n + 3;
    uint64_t c = n1 / 146097;
    uint32_t nc = (uint32_t)(n1 % 146097 / 4);
    /* year of century */
    uint32_t n2 = 4 * nc + 3;
    uint64_t p2 = UINT64_C(2939745) * n2;
    uint32_t z = (uint32_t)(p2 >> 32);
    uint32_t ny = (uint32_t)p2 / 2939745 / 4;
    uint64_t y = 100 * c + z;
    /* month and day */
    uint32_t n3 = 2141 * ny + 197913;
    uint32_t m = n3 >> 16;
    uint32_t d = (n3 & 0xFFFFu) / 2141;
    uint32_t j = ny >= 306;
    *year = (int32_t)((int64_t)y - (int64_t)ZUF_INT_CAL_L + j);
    *month = j ? m - 12 : m;
    *day = d + 1;
}

/* ---- parsing ------------------------------------------------------------ */

ZUF_INLINE bool zuf_int_is_digit(unsigned char c)
{
    return (unsigned)(c - '0') < 10u;
}

/* Two ASCII digits at p to their value, or -1. */
ZUF_INLINE int zuf_int_two_digits(const unsigned char *p)
{
    unsigned a = (unsigned)(p[0] - '0'), b = (unsigned)(p[1] - '0');
    return (a < 10u && b < 10u) ? (int)(a * 10u + b) : -1;
}

/* The 8 bytes at p as a little-endian integer. Written portably; GCC and
   clang compile it to a single load on a little-endian target. */
ZUF_INLINE uint64_t zuf_int_load_le64(const unsigned char *p)
{
    return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) |
           ((uint64_t)p[7] << 56);
}

/* "YYYY-MM-" at p, as SWAR on one 8-byte load: XOR with the template turns
   each digit into its value 0-9 and each '-' into 0, and two masked tests
   reject everything else. Returns false unless the 8 bytes match; on true,
   *year is 0-9999 and *month 0-99. */
ZUF_INLINE bool zuf_int_swar_year_month(const unsigned char *p, uint32_t *year, uint32_t *month)
{
    /* little-endian: byte 0 is the first character */
    const uint64_t tmpl = UINT64_C(0x2D30302D30303030);   /* "0000-00-" */
    const uint64_t high = UINT64_C(0xFFF0F0FFF0F0F0F0);   /* high nibbles of digits, all of '-' */
    const uint64_t add6 = UINT64_C(0x0006060006060606);   /* digit values 10-15 carry into 0x10 */
    const uint64_t bit4 = UINT64_C(0x0010100010101010);
    uint64_t t = zuf_int_load_le64(p) ^ tmpl, y;
    if ((t & high) != 0 || ((t + add6) & bit4) != 0) return false;
    /* four year digits in bytes 0-3: pairs, then the pair of pairs */
    y = t & UINT64_C(0xFFFFFFFF);
    y = (y * 10 + (y >> 8)) & UINT64_C(0x00FF00FF);
    y = (y * 100 + (y >> 16)) & UINT64_C(0xFFFF);
    *year = (uint32_t)y;
    *month = (uint32_t)((t >> 40) & 0xFFu) * 10u + (uint32_t)((t >> 48) & 0xFFu);
    return true;
}

/* The cold path for a fixed-width field that is short or malformed: match
   [p, end) against a pattern of 'd' (digit) and literal bytes, and return
   ZUF_ERR_INVALID (ptr `first`) on a mismatch within the available bytes,
   ZUF_ERR_INCOMPLETE (ptr `last`) when every available byte fits. */
ZUF_INLINE zuf_result zuf_int_field_error(const unsigned char *p, const unsigned char *end,
                                          const char *pattern, const char *first, const char *last)
{
    for (; *pattern && p < end; pattern++, p++) {
        if (*pattern == 'd' ? !zuf_int_is_digit(*p) : *p != (unsigned char)*pattern)
            return zuf_int_result(first, ZUF_ERR_INVALID);
    }
    return *pattern ? zuf_int_result(last, ZUF_ERR_INCOMPLETE) : zuf_int_result(first, ZUF_ERR_INVALID);
}

ZUF_INLINE void zuf_int_clear_datetime(zuf_datetime *dt)
{
    memset(dt, 0, sizeof *dt);
}

/* YYYY-MM-DD only. */
ZUF_INLINE zuf_result zuf_parse_date(const char *first, const char *last, zuf_datetime *out)
{
    const unsigned char *p = (const unsigned char *)first, *end = (const unsigned char *)last;
    uint32_t year, month;
    int day;
    if (end - p < 10 || !zuf_int_swar_year_month(p, &year, &month) || (day = zuf_int_two_digits(p + 8)) < 0)
        return zuf_int_field_error(p, end, "dddd-dd-dd", first, last);
    if (month < 1 || month > 12 || day < 1 || (uint32_t)day > zuf_days_in_month((int32_t)year, month))
        return zuf_int_result(first, ZUF_ERR_INVALID);
    zuf_int_clear_datetime(out);
    out->year = (int32_t)year;
    out->month = (uint8_t)month;
    out->day = (uint8_t)day;
    return zuf_int_result(first + 10, ZUF_OK);
}

/* Every form in the header comment. */
ZUF_INLINE zuf_result zuf_parse_datetime(const char *first, const char *last, zuf_datetime *out)
{
    const unsigned char *p, *end = (const unsigned char *)last;
    zuf_datetime dt;
    int hour, minute, second = 0;
    uint32_t nano = 0;
    zuf_result r = zuf_parse_date(first, last, &dt);
    if (r.status) return r;
    p = (const unsigned char *)r.ptr;

    /* A bare date unless a separator is followed by a digit. */
    if (p == end || (*p != 'T' && *p != 't' && *p != ' ')) {
        *out = dt;
        return r;
    }
    if (p + 1 == end) {
        if (*p == ' ') { *out = dt; return r; }
        return zuf_int_result(last, ZUF_ERR_INCOMPLETE);
    }
    if (!zuf_int_is_digit(p[1])) {
        if (*p == ' ') { *out = dt; return r; }
        return zuf_int_result(first, ZUF_ERR_INVALID);
    }
    p++;

    if (end - p < 5 || (hour = zuf_int_two_digits(p)) < 0 || p[2] != ':' ||
        (minute = zuf_int_two_digits(p + 3)) < 0)
        return zuf_int_field_error(p, end, "dd:dd", first, last);
    p += 5;
    if (p < end && *p == ':') {
        if (end - p < 3 || (second = zuf_int_two_digits(p + 1)) < 0)
            return zuf_int_field_error(p, end, ":dd", first, last);
        p += 3;
        if (p < end && *p == '.') {
            int digits = 0;
            p++;
            while (p < end && zuf_int_is_digit(*p)) {
                if (digits < 9) nano = nano * 10u + (uint32_t)(*p - '0');
                digits++;
                p++;
            }
            if (digits == 0) {
                if (p == end) return zuf_int_result(last, ZUF_ERR_INCOMPLETE);
                return zuf_int_result(first, ZUF_ERR_INVALID);
            }
            for (; digits < 9; digits++) nano *= 10u;
        }
    }
    if (hour > 23 || minute > 59 || second > 60) return zuf_int_result(first, ZUF_ERR_INVALID);

    dt.has_time = true;
    dt.hour = (uint8_t)hour;
    dt.minute = (uint8_t)minute;
    dt.second = (uint8_t)second;
    dt.nanosecond = nano;

    if (p < end && (*p == 'Z' || *p == 'z')) {
        dt.has_offset = true;
        dt.offset_seconds = 0;
        p++;
    } else if (p < end && (*p == '+' || *p == '-')) {
        int sign = *p == '-' ? -1 : 1, oh, om = 0;
        p++;
        if (end - p < 2 || (oh = zuf_int_two_digits(p)) < 0)
            return zuf_int_field_error(p, end, "dd", first, last);
        p += 2;
        if (p < end && *p == ':') {
            if (end - p < 3 || (om = zuf_int_two_digits(p + 1)) < 0)
                return zuf_int_field_error(p, end, ":dd", first, last);
            p += 3;
        } else if (p < end && zuf_int_is_digit(*p)) {
            if (end - p < 2 || (om = zuf_int_two_digits(p)) < 0)
                return zuf_int_field_error(p, end, "dd", first, last);
            p += 2;
        }
        if (oh > 23 || om > 59) return zuf_int_result(first, ZUF_ERR_INVALID);
        dt.has_offset = true;
        dt.offset_seconds = sign * (oh * 3600 + om * 60);
    }
    *out = dt;
    return zuf_int_result((const char *)p, ZUF_OK);
}

/* ---- conversion --------------------------------------------------------- */

/* The date part as days from 1970-01-01. */
ZUF_INLINE int32_t zuf_datetime_days(const zuf_datetime *dt)
{
    return zuf_days_from_civil(dt->year, dt->month, dt->day);
}

/* Seconds and nanoseconds since 1970-01-01T00:00:00Z. Applies
   offset_seconds when has_offset; otherwise treats the wall time as UTC. A
   leap second (:60) counts as the first second of the next minute. */
ZUF_INLINE zuf_timestamp zuf_datetime_timestamp(const zuf_datetime *dt)
{
    zuf_timestamp t;
    t.seconds = (int64_t)zuf_datetime_days(dt) * 86400 +
                (int64_t)dt->hour * 3600 + (int64_t)dt->minute * 60 + dt->second;
    if (dt->has_offset) t.seconds -= dt->offset_seconds;
    t.nanoseconds = dt->nanosecond;
    return t;
}

/* ---- formatting --------------------------------------------------------- */

/* The length zuf_int_format_year() writes for year. */
ZUF_INLINE size_t zuf_int_year_len(int32_t year)
{
    uint32_t v;
    size_t n = 0;
    if (year >= 0 && year <= 9999) return 4;
    v = year < 0 ? 0u - (uint32_t)year : (uint32_t)year;
    do { n++; v /= 10u; } while (v);
    return 1 + (n < 5 ? 5 : n);
}

/* Write the n low decimal digits of v to d, two at a time from the end. */
ZUF_INLINE void zuf_int_write_digits(char *d, uint32_t v, size_t n)
{
    while (n >= 2) {
        n -= 2;
        zuf_int_write2(d + n, v % 100u);
        v /= 100u;
    }
    if (n) d[0] = (char)('0' + v % 10u);
}

/* Write a year in the len == zuf_int_year_len(year) bytes at d: four
   digits for 0-9999, otherwise ISO 8601 expanded form with a sign and at
   least five digits. */
ZUF_INLINE void zuf_int_format_year(char *d, int32_t year, size_t len)
{
    if (len == 4) {
        zuf_int_write_digits(d, (uint32_t)year, 4);
        return;
    }
    d[0] = year < 0 ? '-' : '+';
    zuf_int_write_digits(d + 1, year < 0 ? 0u - (uint32_t)year : (uint32_t)year, len - 1);
}

/* YYYY-MM-DD for a day count from 1970-01-01. Returns the length (10 for
   years 0000-9999); writes only when it fits; never NUL-terminates. */
ZUF_INLINE size_t zuf_format_date(char *dst, size_t cap, int32_t days)
{
    int32_t y;
    uint32_t m, d;
    size_t len;
    zuf_civil_from_days(days, &y, &m, &d);
    len = zuf_int_year_len(y) + 6;
    if (cap < len) return len;
    {
        size_t k = len - 6;
        zuf_int_format_year(dst, y, k);
        dst[k] = '-';
        zuf_int_write2(dst + k + 1, m);
        dst[k + 3] = '-';
        zuf_int_write2(dst + k + 4, d);
    }
    return len;
}

/* RFC 3339. The date alone when !has_time. The fraction has 0, 3, 6 or 9
   digits, the shortest group that represents nanosecond exactly. The
   offset is Z when it is 0, +HH:MM otherwise (seconds of the offset are
   dropped), and absent when !has_offset. Fields outside their ranges are
   reduced modulo their width, never written out of bounds. Returns the
   length; writes only when it fits; never NUL-terminates. */
ZUF_INLINE size_t zuf_format_datetime(char *dst, size_t cap, const zuf_datetime *dt)
{
    size_t ylen = zuf_int_year_len(dt->year), len = ylen + 6, k;
    uint32_t nano = dt->nanosecond % 1000000000u, frac_digits = 0, frac = 0;
    if (dt->has_time) {
        len += 9;
        if (nano) {
            if (nano % 1000000u == 0)   { frac_digits = 3; frac = nano / 1000000u; }
            else if (nano % 1000u == 0) { frac_digits = 6; frac = nano / 1000u; }
            else                        { frac_digits = 9; frac = nano; }
            len += 1 + frac_digits;
        }
        if (dt->has_offset) len += dt->offset_seconds / 60 == 0 ? 1 : 6;
    }
    if (cap < len) return len;

    zuf_int_format_year(dst, dt->year, ylen);
    k = ylen;
    dst[k] = '-';
    zuf_int_write2(dst + k + 1, dt->month % 100u);
    dst[k + 3] = '-';
    zuf_int_write2(dst + k + 4, dt->day % 100u);
    k += 6;
    if (!dt->has_time) return len;
    dst[k] = 'T';
    zuf_int_write2(dst + k + 1, dt->hour % 100u);
    dst[k + 3] = ':';
    zuf_int_write2(dst + k + 4, dt->minute % 100u);
    dst[k + 6] = ':';
    zuf_int_write2(dst + k + 7, dt->second % 100u);
    k += 9;
    if (frac_digits) {
        dst[k++] = '.';
        zuf_int_write_digits(dst + k, frac, frac_digits);
        k += frac_digits;
    }
    if (dt->has_offset) {
        int32_t minutes = dt->offset_seconds / 60;
        if (minutes == 0) {
            dst[k] = 'Z';
        } else {
            uint32_t a = minutes < 0 ? (uint32_t)(-(int64_t)minutes) : (uint32_t)minutes;
            dst[k] = minutes < 0 ? '-' : '+';
            zuf_int_write2(dst + k + 1, (a / 60u) % 100u);
            dst[k + 3] = ':';
            zuf_int_write2(dst + k + 4, a % 60u);
        }
    }
    return len;
}

#endif /* ZUFAST_DATETIME_H */
