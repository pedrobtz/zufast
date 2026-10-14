/* The wide.h differential (design 21.1). tools/check-headers builds this
   program three ways: as the compiler configures it, with __SIZEOF_INT128__
   undefined, and with ZUF_NO_INT128 defined. The last two compile the
   two-word fallback; all three must print the same digest. Each build also
   checks properties that need no reference, so a fallback that is wrong in
   every build alike still fails.

   ZUF_PROBE_EXPECT_INT128 is 1 for the native build and 0 for the others:
   the program refuses to compile when the path it was asked for is not the
   one in effect. */
#include <stdio.h>
#include <string.h>

#include <zufast/wide.h>

#if !defined(ZUF_PROBE_EXPECT_INT128)
#  error "define ZUF_PROBE_EXPECT_INT128 to 0 or 1"
#elif ZUF_PROBE_EXPECT_INT128 != ZUF_INT_HAVE_INT128
#  error "wide.h did not select the path this build expects"
#endif

static uint64_t rng = 0x9E3779B97F4A7C15u, digest = 0xCBF29CE484222325u;

static uint64_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static void mix(uint64_t v)
{
    digest = (digest ^ v) * 0x100000001B3u;
    digest ^= digest >> 29;
}

static void mix_u(zuf_u128 v) { mix(zuf_u128_low(v)); mix(zuf_u128_high(v)); }
static void mix_i(zuf_i128 v) { mix_u(zuf_u128_from_i128(v)); }

/* random bit widths, so every division length occurs */
static zuf_u128 value(void)
{
    unsigned w = (unsigned)(next() % 129);
    zuf_u128 v = zuf_u128_make(next(), next());
    if (w == 0) return zuf_u128_make(0, 0);
    if (w > 64) return zuf_u128_make(zuf_u128_high(v) >> (128 - w), zuf_u128_low(v));
    return zuf_u128_make(0, zuf_u128_low(v) >> (64 - w));
}

int main(void)
{
    long i, bad = 0;
    char text[ZUF_I128_MAX_CHARS];
    for (i = 0; i < 200000; i++) {
        zuf_u128 ua = value(), ub = value(), uq, ur, ut;
        zuf_i128 ia = zuf_i128_from_u128(ua), ib = zuf_i128_from_u128(ub), iq, ir, it;
        int64_t r64, x, y;
        uint64_t u64, xa = zuf_u128_low(ua), xb = zuf_u128_low(ub);
        double d;
        char *e;
        memcpy(&x, &xa, 8);
        memcpy(&y, &xb, 8);
        if (next() & 1) ia = zuf_i128_sub_wrap(zuf_i128_from_i64(0), ia);

        mix(zuf_u128_add(ua, ub, &ut)); mix_u(zuf_u128_add_wrap(ua, ub));
        mix(zuf_u128_sub(ua, ub, &ut)); mix_u(zuf_u128_sub_wrap(ua, ub));
        mix(zuf_u128_mul(ua, ub, &ut)); mix_u(zuf_u128_mul_wrap(ua, ub));
        mix(zuf_i128_add(ia, ib, &it)); mix_i(zuf_i128_add_wrap(ia, ib));
        mix(zuf_i128_sub(ia, ib, &it)); mix_i(zuf_i128_sub_wrap(ia, ib));
        mix(zuf_i128_mul(ia, ib, &it)); mix_i(zuf_i128_mul_wrap(ia, ib));
        mix(zuf_i128_neg(ia, &it));
        mix((uint64_t)(zuf_i128_cmp(ia, ib) + 1)); mix((uint64_t)(zuf_u128_cmp(ua, ub) + 1));
        mix_u(zuf_u128_mul_u64_u64(xa, xb)); mix_i(zuf_i128_mul_i64_i64(x, y));

        if (zuf_u128_divmod(ua, ub, &uq, &ur)) {
            mix_u(uq); mix_u(ur);
            if (zuf_u128_cmp(zuf_u128_add_wrap(zuf_u128_mul_wrap(uq, ub), ur), ua) != 0) bad++;
        }
        if (zuf_u128_divmod_u64(ua, xb, &uq, &u64)) { mix_u(uq); mix(u64); }
        if (zuf_i128_divmod_trunc(ia, ib, &iq, &ir)) {
            mix_i(iq); mix_i(ir);
            if (zuf_i128_cmp(zuf_i128_add_wrap(zuf_i128_mul_wrap(iq, ib), ir), ia) != 0) bad++;
        }
        if (zuf_i128_divmod_floor(ia, ib, &iq, &ir)) {
            mix_i(iq); mix_i(ir);
            if (zuf_i128_cmp(zuf_i128_add_wrap(zuf_i128_mul_wrap(iq, ib), ir), ia) != 0) bad++;
        }
        if (zuf_i128_divmod_trunc_i64(ia, y, &iq, &r64)) { mix_i(iq); mix((uint64_t)r64); }
        if (zuf_i128_divmod_floor_i64(ia, y, &iq, &r64)) { mix_i(iq); mix((uint64_t)r64); }

        e = zuf_write_i128(text, ia);
        mix((uint64_t)(e - text));
        if (zuf_parse_i128(text, e, &it).status != ZUF_OK || zuf_i128_cmp(it, ia) != 0) bad++;
        e = zuf_write_u128(text, ua);
        if (zuf_parse_u128(text, e, &ut).status != ZUF_OK || zuf_u128_cmp(ut, ua) != 0) bad++;

        if (zuf_i128_to_f64(ia, &d)) {
            memcpy(&u64, &d, 8);
            mix(u64);
            if (!zuf_i128_from_f64(d, &it) || zuf_i128_cmp(it, ia) != 0) bad++;
        }
        u64 = next();
        memcpy(&d, &u64, 8);
        mix(zuf_i128_from_f64(d, &it));
        mix(zuf_u128_from_f64(d, &ut));
        if (zuf_i128_to_i64(ia, &r64)) mix((uint64_t)r64);
        if (zuf_u128_to_u64(ua, &u64)) mix(u64);
    }
    printf("%016llx\n", (unsigned long long)digest);
    if (bad) fprintf(stderr, "%ld property failures\n", bad);
    return bad != 0;
}
