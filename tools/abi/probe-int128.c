/* The no-int128 differential (design 21.1). Every vendored library selects
   its 128-bit multiply on __SIZEOF_INT128__; tools/check-headers builds this
   program once as the compiler configures it and once with that macro
   undefined, which compiles the portable fallbacks instead, and requires the
   two to print the same digest. Each build also round-trips its own output,
   so a fallback that is wrong in both builds alike still fails.

   ZUF_PROBE_EXPECT_INT128 is 1 for the native build and 0 for the fallback
   build: the program refuses to compile when the configuration it was asked
   for is not the one in effect. */
#include <stdio.h>
#include <string.h>

#include <zufast.h>

#if !defined(ZUF_PROBE_EXPECT_INT128)
#  error "define ZUF_PROBE_EXPECT_INT128 to 0 or 1"
#elif ZUF_PROBE_EXPECT_INT128 && !defined(__SIZEOF_INT128__)
#  error "native build without __SIZEOF_INT128__: the differential would compare a fallback with itself"
#elif !ZUF_PROBE_EXPECT_INT128 && defined(__SIZEOF_INT128__)
#  error "fallback build with __SIZEOF_INT128__ still defined"
#endif

static uint64_t rng = 0x9E3779B97F4A7C15u;

static uint64_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

int main(void)
{
    zuf_hasher out;   /* everything printed or computed is hashed here */
    unsigned char buf[512];
    char s[64];
    long i, bad = 0;
    zuf_digest128 d;

    zuf_hasher_init(&out, 0);
    for (i = 0; i < 200000; i++) {
        uint64_t bits = next();
        uint32_t fbits = (uint32_t)(bits >> 32);
        double v, back = 0;
        float f, fback = 0;
        uint64_t u = next() >> (next() & 63), uback = 0;
        size_t n;
        memcpy(&v, &bits, sizeof v);
        memcpy(&f, &fbits, sizeof f);

        /* Ryu: shortest digits, then ffc: correctly rounded parse back */
        n = zuf_format_f64(s, sizeof s, v);
        zuf_hasher_update(&out, s, n);
        if (v == v) {
            if (zuf_parse_f64(s, s + n, &back).ptr != s + n || memcmp(&back, &v, sizeof v) != 0) bad++;
        }
        n = zuf_format_f32(s, sizeof s, f);
        zuf_hasher_update(&out, s, n);
        if (f == f) {
            if (zuf_parse_f32(s, s + n, &fback).ptr != s + n || memcmp(&fback, &f, sizeof f) != 0) bad++;
        }

        /* long decimal inputs reach ffc's big-integer path */
        n = (size_t)snprintf(s, sizeof s, "%llu%llue-%u", (unsigned long long)next(),
                             (unsigned long long)next(), (unsigned)(next() % 360));
        (void)zuf_parse_f64(s, s + n, &back);
        zuf_hasher_update(&out, &back, sizeof back);

        n = (size_t)(zuf_write_u64(s, u) - s);
        if (zuf_parse_u64(s, s + n, &uback).status != ZUF_OK || uback != u) bad++;
    }

    /* XXH3 over every length to 512, one-shot and streamed */
    for (i = 0; i < (long)sizeof buf; i++) buf[i] = (unsigned char)next();
    for (i = 0; i <= (long)sizeof buf; i++) {
        uint64_t h64 = zuf_hash64_seed(buf, (size_t)i, (uint64_t)i);
        zuf_hasher h;
        d = zuf_hash128_seed(buf, (size_t)i, (uint64_t)i);
        zuf_hasher_update(&out, &h64, sizeof h64);
        zuf_hasher_update(&out, &d, sizeof d);
        zuf_hasher_init(&h, (uint64_t)i);
        zuf_hasher_update(&h, buf, (size_t)i / 2);
        zuf_hasher_update(&h, buf + i / 2, (size_t)(i - i / 2));
        if (zuf_hasher_digest64(&h) != h64) bad++;
    }

    d = zuf_hasher_digest128(&out);
    printf("%016llx%016llx\n", (unsigned long long)d.high, (unsigned long long)d.low);
    if (bad) fprintf(stderr, "%ld round-trip failures\n", bad);
    return bad != 0;
}
