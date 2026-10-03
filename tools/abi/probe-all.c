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

    /* datetime.h */
    {
        static const char text[] = "2024-02-29T12:00:00.5+01:00";
        char out[ZUF_DATETIME_MAX_CHARS];
        char date[ZUF_DATE_MAX_CHARS];
        zuf_datetime dt;
        zuf_timestamp ts;
        int32_t y = 0;
        uint32_t m = 0, d = 0;
        acc += (int)zuf_parse_date(text, text + ZUF_DATE_CHARS, &dt).status;
        acc += (int)zuf_parse_datetime(text, text + sizeof text - 1, &dt).status;
        acc += zuf_datetime_days(&dt);
        ts = zuf_datetime_timestamp(&dt);
        acc += (int)(ts.seconds % 7) + (int)ts.nanoseconds;
        acc += zuf_days_from_civil(2024, 2, 29);
        zuf_civil_from_days(0, &y, &m, &d);
        acc += y + (int)m + (int)d;
        acc += zuf_is_leap_year(2024) + (int)zuf_days_in_month(2024, 2);
        acc += (int)zuf_format_date(date, sizeof date, 0);
        acc += (int)zuf_format_datetime(out, sizeof out, &dt);
    }

    /* number.h */
    {
        static const char text[] = "-12.5e3";
        char out[ZUF_I64_MAX_CHARS + 32];
        double d;
        float f;
        int64_t i64;
        uint64_t u64;
        int32_t i32;
        uint32_t u32;
        zuf_num_options opt;
        const char *e = text + sizeof text - 1;
        opt.flags = ZUF_NUM_JSON | ZUF_NUM_LEADING_PLUS | ZUF_NUM_SKIP_SPACE;
        opt.base = 16;
        opt.decimal_point = ',';
        acc += (int)zuf_parse_f64(text, e, &d).status + (int)zuf_parse_f32(text, e, &f).status;
        acc += (int)zuf_parse_i64(text, e, &i64).status + (int)zuf_parse_u64(text, e, &u64).status;
        acc += (int)zuf_parse_i32(text, e, &i32).status + (int)zuf_parse_u32(text, e, &u32).status;
        acc += (int)zuf_parse_f64_opt(text, e, &d, &opt).status;
        acc += (int)zuf_parse_f32_opt(text, e, &f, &opt).status;
        acc += (int)zuf_parse_i64_opt(text, e, &i64, &opt).status;
        acc += (int)zuf_parse_u64_opt(text, e, &u64, &opt).status;
        acc += (int)zuf_parse_i32_opt(text, e, &i32, &opt).status;
        acc += (int)zuf_parse_u32_opt(text, e, &u32, &opt).status;
        acc += (int)(zuf_write_u64(out, 1) - out) + (int)(zuf_write_i64(out, -1) - out);
        acc += (int)(zuf_write_u32(out, 1) - out) + (int)(zuf_write_i32(out, -1) - out);
        acc += (int)zuf_format_f64_fixed(out, sizeof out, 0.5, 2);
        acc += (int)zuf_format_f64(out, sizeof out, 0.1) + (int)zuf_format_f32(out, sizeof out, 0.1f);
        acc += (int)zuf_format_f64_opt(out, sizeof out, 0.1, ZUF_FMT_SCIENTIFIC | ZUF_FMT_TRAILING_ZERO);
        acc += (int)zuf_format_f32_opt(out, ZUF_F32_MAX_CHARS, 0.1f, 0);
        acc += (int)zuf_decimal_f64(0.1).exponent + (int)zuf_decimal_f32(0.1f).mantissa;
        acc += zuf_decimal_f64(-1.0).negative + ZUF_F64_MAX_CHARS;
    }

    return acc;
}
