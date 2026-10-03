/* Translation unit 2: bytes. Includes only the area headers it uses, the
   other way the README allows, plus the umbrella for the rest. */
#include <zufast/hex.h>
#include <zufast/base64.h>
#include <zufast/uuid.h>
#include <zufast/hash.h>
#include <zufast/bits.h>
#include <zufast/utf8.h>
#include <zufast.h>
#include "zufasttest.h"

static SEXP str(const char *s, size_t n) { return Rf_mkCharLen(s, (int)n); }

/* Takes a raw vector; returns a character vector of results, one per public
   function of the byte-oriented areas. */
SEXP zt_binary(SEXP x)
{
    const unsigned char *data = RAW(x);
    size_t n = (size_t)XLENGTH(x), len = 0;
    char buf[1024];
    unsigned char bytes[512];
    SEXP out = PROTECT(Rf_allocVector(STRSXP, 32));
    int k = 0;
    zuf_uuid u;
    zuf_hasher h;
    zuf_digest128 d128;
    bool valid;

    if (n > 128) n = 128;
    /* hex.h */
    len = zuf_hex_encode(data, n, buf, sizeof buf, false);
    SET_STRING_ELT(out, k++, str(buf, len));
    SET_STRING_ELT(out, k++, Rf_mkChar(zuf_hex_decode(buf, buf + len, bytes, sizeof bytes, &len) == ZUF_OK &&
                                       len == n && memcmp(bytes, data, n) == 0 ? "hex-ok" : "hex-bad"));
    /* base64.h */
    len = zuf_base64_encode(data, n, buf, sizeof buf, ZUF_B64_URL | ZUF_B64_NO_PAD);
    SET_STRING_ELT(out, k++, str(buf, len));
    {
        size_t enc = len, dec = 0;
        bool ok = zuf_base64_decode(buf, buf + enc, bytes, sizeof bytes, &dec,
                                    ZUF_B64_URL | ZUF_B64_NO_PAD) == ZUF_OK &&
                  dec == n && memcmp(bytes, data, n) == 0 &&
                  zuf_base64_encode_bound(n) >= enc && zuf_base64_decode_bound(enc) >= n;
        SET_STRING_ELT(out, k++, Rf_mkChar(ok ? "b64-ok" : "b64-bad"));
    }
    /* uuid.h: the first 16 bytes */
    memset(u.bytes, 0, 16);
    memcpy(u.bytes, data, n < 16 ? n : 16);
    len = zuf_format_uuid(buf, sizeof buf, &u, true);
    SET_STRING_ELT(out, k++, str(buf, len));
    {
        zuf_uuid back;
        zuf_result r = zuf_parse_uuid(buf, buf + len, &back);
        SET_STRING_ELT(out, k++, Rf_mkChar(r.status == ZUF_OK && memcmp(back.bytes, u.bytes, 16) == 0
                                           ? "uuid-ok" : "uuid-bad"));
    }
    /* hash.h */
    {
        uint64_t a = zuf_hash64(data, n), b = zuf_hash64_seed(data, n, 0);
        unsigned char be[16];
        d128 = zuf_hash128(data, n);
        zuf_store_be64(be, a);
        len = zuf_hex_encode(be, 8, buf, sizeof buf, false);
        SET_STRING_ELT(out, k++, str(buf, len));
        zuf_store_be64(be, d128.high);
        zuf_store_be64(be + 8, d128.low);
        len = zuf_hex_encode(be, 16, buf, sizeof buf, false);
        SET_STRING_ELT(out, k++, str(buf, len));
        zuf_hasher_init(&h, 0);
        zuf_hasher_update(&h, data, n / 2);
        zuf_hasher_update(&h, data + n / 2, n - n / 2);
        SET_STRING_ELT(out, k++, Rf_mkChar(
            zuf_hasher_digest64(&h) == a && a == b &&
            zuf_hasher_digest128(&h).low == zuf_hash128_seed(data, n, 0).low ? "stream-ok" : "stream-bad"));
    }
    /* bits.h */
    {
        unsigned char w[8];
        int ok = 1;
        zuf_store_le16(w, 0x0102); ok &= zuf_load_le16(w) == 0x0102 && zuf_load_be16(w) == 0x0201;
        zuf_store_be16(w, 0x0102); ok &= zuf_bswap16(zuf_load_le16(w)) == 0x0102;
        zuf_store_le32(w, 0x01020304u); ok &= zuf_load_be32(w) == zuf_bswap32(0x01020304u);
        zuf_store_be32(w, 0x01020304u); ok &= zuf_load_le32(w) == 0x04030201u;
        zuf_store_le64(w, 1); ok &= zuf_load_be64(w) == zuf_bswap64(1);
        zuf_store_be64(w, 1); ok &= zuf_load_le64(w) == UINT64_C(0x0100000000000000);
        ok &= zuf_f16_to_f32(zuf_f32_to_f16(1.5f)) == 1.5f && zuf_f32_fits_f16(0.25f) && !zuf_f32_fits_f16(0.1f);
        ok &= zuf_bf16_to_f32(zuf_f32_to_bf16(2.0f)) == 2.0f && zuf_f64_fits_f32(0.5) && !zuf_f64_fits_f32(0.1);
        SET_STRING_ELT(out, k++, Rf_mkChar(ok ? "bits-ok" : "bits-bad"));
    }
    /* utf8.h */
    (void)zuf_utf8_count((const char *)data, n, &valid);
    SET_STRING_ELT(out, k++, Rf_mkChar(zuf_utf8_valid((const char *)data, n) && valid ? "utf8" : "not-utf8"));
    out = Rf_lengthgets(out, k);
    UNPROTECT(1);
    return out;
}
