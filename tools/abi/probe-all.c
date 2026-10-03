/* Calls every public function: must compile warning-free as C99 and C++11
   (design 21.1). Each area adds its calls here as it lands. */
#include <zufast.h>

int zuf_probe_all(void);
int zuf_probe_all(void)
{
    int acc = 0;

    /* version.h */
    acc += ZUFAST_VERSION_NUMBER >= 0;
    acc += (int)sizeof(ZUFAST_VERSION);

    /* status.h */
    zuf_result r;
    r.ptr = ZUFAST_VERSION;
    r.status = ZUF_OK;
    acc += (int)zuf_status_string(r.status)[0];
    acc += (int)zuf_status_string(ZUF_ERR_NO_SPACE)[0];

    /* literal.h */
    {
        static const char text[] = " TRUE ";
        const char *f = text, *l = text + 6;
        bool b = false;
        zuf_trim_space(&f, &l);
        acc += (int)zuf_parse_bool(f, l, ZUF_BOOL_R | ZUF_BOOL_DIGIT | ZUF_BOOL_YESNO, &b).status;
        acc += b;
        acc += zuf_equals(f, l, "TRUE", 4);
        acc += zuf_equals_ci(f, l, "true", 4);
        acc += (int)(zuf_skip_space(text, text + 6) - text);
        acc += ZUF_BOOL_YAML12;
    }

    /* bits.h */
    {
        unsigned char buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        acc += (int)zuf_f16_to_f32(zuf_f32_to_f16(1.5f));
        acc += (int)zuf_bf16_to_f32(zuf_f32_to_bf16(2.5f));
        acc += zuf_f32_fits_f16(0.5f) + zuf_f64_fits_f32(0.25);
        acc += (int)(zuf_load_le16(buf) + zuf_load_be16(buf));
        acc += (int)(zuf_load_le32(buf) + zuf_load_be32(buf));
        acc += (int)(zuf_load_le64(buf) + zuf_load_be64(buf));
        zuf_store_le16(buf, 1); zuf_store_be16(buf, 2);
        zuf_store_le32(buf, 3); zuf_store_be32(buf, 4);
        zuf_store_le64(buf, 5); zuf_store_be64(buf, 6);
        acc += (int)(zuf_bswap16(1) + zuf_bswap32(2) + zuf_bswap64(3));
        acc += buf[7];
    }

    /* utf8.h */
    {
        bool valid = false;
        acc += zuf_utf8_valid("abc", 3);
        acc += (int)zuf_utf8_count("abc", 3, &valid);
        acc += valid;
    }

    /* hex.h, base64.h, uuid.h */
    {
        static const char hex[] = "00ff";
        static const char b64[] = "AP8=";
        static const char uuid_text[] = "123e4567-e89b-12d3-a456-426614174000";
        unsigned char bytes[8] = {0, 0xFF, 0, 0, 0, 0, 0, 0};
        char text[ZUF_UUID_CHARS];
        size_t len = 0;
        zuf_uuid u;
        acc += (int)zuf_hex_encode(bytes, 2, text, sizeof text, false);
        acc += (int)zuf_hex_decode(hex, hex + 4, bytes, sizeof bytes, &len);
        acc += (int)zuf_base64_encode_bound(3) + (int)zuf_base64_decode_bound(4);
        acc += (int)zuf_base64_encode(bytes, 2, text, sizeof text, ZUF_B64_URL | ZUF_B64_NO_PAD);
        acc += (int)zuf_base64_decode(b64, b64 + 4, bytes, sizeof bytes, &len, 0);
        acc += (int)zuf_parse_uuid(uuid_text, uuid_text + ZUF_UUID_CHARS, &u).status;
        acc += (int)zuf_format_uuid(text, sizeof text, &u, true);
        acc += (int)len;
    }

    return acc;
}
