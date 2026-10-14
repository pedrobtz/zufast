/* Dates: whatever parses formats back to the same fields; the calendar
   round-trips any int32 day count taken from the input. Half the inputs,
   by length parity, parse with ZUF_DT_EXPANDED_YEAR; the round trip always
   reads the formatter's output with it, since that output may be expanded. */
#include "fuzz.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s = (const char *)data, *e = s + size;
    zuf_datetime dt, back;
    char buf[64];
    size_t n;
    zuf_result r;
    uint32_t flags = (size & 1) ? ZUF_DT_EXPANDED_YEAR : 0u;
    FUZZ_CANARY(data, size);

    r = zuf_parse_datetime_opt(s, e, &dt, flags);
    if (r.status == ZUF_OK) {
        FUZZ_CHECK(r.ptr > s && r.ptr <= e);
        n = zuf_format_datetime(buf, sizeof buf, &dt);
        FUZZ_CHECK(n <= ZUF_DATETIME_MAX_CHARS);
        r = zuf_parse_datetime_opt(buf, buf + n, &back, ZUF_DT_EXPANDED_YEAR);
        FUZZ_CHECK(r.status == ZUF_OK && r.ptr == buf + n);
        FUZZ_CHECK(back.year == dt.year && back.month == dt.month && back.day == dt.day &&
                   back.hour == dt.hour && back.minute == dt.minute && back.second == dt.second &&
                   back.nanosecond == dt.nanosecond && back.has_time == dt.has_time &&
                   back.has_offset == dt.has_offset && back.offset_seconds == dt.offset_seconds);
        (void)zuf_datetime_timestamp(&dt);
    } else if (r.status == ZUF_ERR_INVALID) {
        FUZZ_CHECK(r.ptr == s);
    } else {
        FUZZ_CHECK(r.status == ZUF_ERR_INCOMPLETE && r.ptr == e);
    }
    r = zuf_parse_date_opt(s, e, &dt, flags);
    FUZZ_CHECK(r.status != ZUF_OK || r.ptr == s + ZUF_DATE_CHARS || r.ptr == s + ZUF_DATE_EXPANDED_CHARS);

    if (size >= 4) {
        int32_t days, y;
        uint32_t m, d;
        memcpy(&days, data, 4);
        zuf_civil_from_days(days, &y, &m, &d);
        FUZZ_CHECK(m >= 1 && m <= 12 && d >= 1 && d <= zuf_days_in_month(y, m));
        FUZZ_CHECK(zuf_days_from_civil(y, m, d) == days);
        n = zuf_format_date(buf, sizeof buf, days);
        FUZZ_CHECK(n <= ZUF_DATE_MAX_CHARS);
        if (y >= -999999 && y <= 999999) {
            r = zuf_parse_date_opt(buf, buf + n, &back, ZUF_DT_EXPANDED_YEAR);
            FUZZ_CHECK(r.status == ZUF_OK && r.ptr == buf + n);
            FUZZ_CHECK(back.year == y && back.month == m && back.day == d);
        }
    }
    return 0;
}
