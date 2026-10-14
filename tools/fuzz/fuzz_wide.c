/* 128-bit integers: whatever zuf_parse_i128() / zuf_parse_u128() accept
   writes back and parses to the same value, and the ptr contract holds for
   every status. The first 32 bytes, read as two values, also go through
   division and the checked operations, whose results are checked against
   the wrapping ones. */
#include "fuzz.h"

static int digits_only(const char *p, const char *e)
{
    for (; p < e; p++) if ((unsigned)(*p - '0') >= 10u) return 0;
    return 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s = (const char *)data, *e = s + size;
    char buf[ZUF_I128_MAX_CHARS];
    zuf_result r;
    zuf_i128 v, back;
    zuf_u128 u, uback;
    FUZZ_CANARY(data, size);

    r = zuf_parse_i128(s, e, &v);
    if (r.status == ZUF_OK || r.status == ZUF_ERR_RANGE) {
        FUZZ_CHECK(r.ptr > s && r.ptr <= e);
        FUZZ_CHECK(digits_only(s + (*s == '-'), r.ptr));
        if (r.status == ZUF_OK) {
            char *end = zuf_write_i128(buf, v);
            FUZZ_CHECK(end - buf <= ZUF_I128_MAX_CHARS);
            r = zuf_parse_i128(buf, end, &back);
            FUZZ_CHECK(r.status == ZUF_OK && r.ptr == end && zuf_i128_cmp(back, v) == 0);
        }
    } else {
        FUZZ_CHECK(r.status == ZUF_ERR_INVALID && r.ptr == s);
    }

    r = zuf_parse_u128(s, e, &u);
    if (r.status == ZUF_OK || r.status == ZUF_ERR_RANGE) {
        FUZZ_CHECK(r.ptr > s && r.ptr <= e && digits_only(s, r.ptr));
        if (r.status == ZUF_OK) {
            char *end = zuf_write_u128(buf, u);
            FUZZ_CHECK(end - buf <= ZUF_U128_MAX_CHARS);
            r = zuf_parse_u128(buf, end, &uback);
            FUZZ_CHECK(r.status == ZUF_OK && r.ptr == end && zuf_u128_cmp(uback, u) == 0);
        }
    } else {
        FUZZ_CHECK(r.status == ZUF_ERR_INVALID && r.ptr == s);
    }

    if (size >= 32) {
        uint64_t w[4];
        zuf_u128 a, b, q, rem, t;
        zuf_i128 ia, ib, iq, ir, it;
        memcpy(w, data, 32);
        a = zuf_u128_make(w[0], w[1]);
        b = zuf_u128_make(w[2], w[3] >> (w[0] & 63));   /* vary the divisor's width */
        ia = zuf_i128_from_u128(a);
        ib = zuf_i128_from_u128(b);
        if (zuf_u128_divmod(a, b, &q, &rem)) {
            FUZZ_CHECK(zuf_u128_cmp(rem, b) < 0);
            FUZZ_CHECK(zuf_u128_cmp(zuf_u128_add_wrap(zuf_u128_mul_wrap(q, b), rem), a) == 0);
        }
        if (zuf_i128_divmod_floor(ia, ib, &iq, &ir))
            FUZZ_CHECK(zuf_i128_cmp(zuf_i128_add_wrap(zuf_i128_mul_wrap(iq, ib), ir), ia) == 0);
        if (zuf_u128_mul(a, b, &t)) FUZZ_CHECK(zuf_u128_cmp(t, zuf_u128_mul_wrap(a, b)) == 0);
        if (zuf_i128_mul(ia, ib, &it)) FUZZ_CHECK(zuf_i128_cmp(it, zuf_i128_mul_wrap(ia, ib)) == 0);
        if (zuf_i128_add(ia, ib, &it)) FUZZ_CHECK(zuf_i128_cmp(it, zuf_i128_add_wrap(ia, ib)) == 0);
    }
    return 0;
}
