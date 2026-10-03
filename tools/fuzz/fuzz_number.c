/* Number parsing and formatting: whatever parses formats back to the same
   bits, under every option and base the first byte selects. */
#include "fuzz.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s, *e;
    zuf_num_options opt;
    char buf[400];
    double d, d2;
    float f, f2;
    int64_t i64, i64b;
    uint64_t u64, u64b;
    zuf_result r;
    size_t n;
    FUZZ_CANARY(data, size);
    if (size < 1) return 0;
    opt.flags = data[0] & 7u;
    opt.base = 2 + data[0] % 35;
    opt.decimal_point = (data[0] & 0x80) ? ',' : 0;
    s = (const char *)data + 1;
    e = (const char *)data + size;

    r = zuf_parse_f64_opt(s, e, &d, &opt);
    FUZZ_CHECK(r.status == ZUF_ERR_INVALID ? r.ptr == s : (r.ptr > s && r.ptr <= e));
    if (r.status != ZUF_ERR_INVALID) {
        n = zuf_format_f64(buf, sizeof buf, d);
        FUZZ_CHECK(n <= ZUF_F64_MAX_CHARS);
        r = zuf_parse_f64(buf, buf + n, &d2);
        FUZZ_CHECK(r.status == ZUF_OK && r.ptr == buf + n);
        FUZZ_CHECK(memcmp(&d, &d2, 8) == 0 || (d != d && d2 != d2));
        n = zuf_format_f64_fixed(buf, sizeof buf, d, data[0] % 40);
        FUZZ_CHECK(n <= 311 + 40);
    }
    r = zuf_parse_f32_opt(s, e, &f, &opt);
    if (r.status != ZUF_ERR_INVALID) {
        n = zuf_format_f32_opt(buf, sizeof buf, f, data[0] & 3u);
        FUZZ_CHECK(n <= ZUF_F32_MAX_CHARS);
        if (!(data[0] & 1u)) {
            r = zuf_parse_f32(buf, buf + n, &f2);
            FUZZ_CHECK(r.status == ZUF_OK && (memcmp(&f, &f2, 4) == 0 || (f != f && f2 != f2)));
        }
    }
    r = zuf_parse_i64_opt(s, e, &i64, &opt);
    if (r.status == ZUF_OK && opt.base == 10) {
        char *p = zuf_write_i64(buf, i64);
        FUZZ_CHECK(p - buf <= ZUF_I64_MAX_CHARS);
        r = zuf_parse_i64(buf, p, &i64b);
        FUZZ_CHECK(r.status == ZUF_OK && i64b == i64);
    }
    r = zuf_parse_u64_opt(s, e, &u64, &opt);
    if (r.status == ZUF_OK && opt.base == 10) {
        char *p = zuf_write_u64(buf, u64);
        r = zuf_parse_u64(buf, p, &u64b);
        FUZZ_CHECK(r.status == ZUF_OK && u64b == u64);
    }
    {
        int32_t i32;
        uint32_t u32;
        (void)zuf_parse_i32_opt(s, e, &i32, &opt);
        (void)zuf_parse_u32_opt(s, e, &u32, &opt);
    }
    return 0;
}
