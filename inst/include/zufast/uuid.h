/*
 * zufast/uuid.h -- UUID parsing and formatting (design 12).
 *
 * A UUID is sixteen bytes in network order, as RFC 9562 and CBOR tag 37
 * store it, so no byte-order question arises when it is stored or compared.
 * zufast parses and formats UUIDs; it never generates one.
 *
 * zuf_parse_uuid() accepts the 8-4-4-4-12 hyphenated form and the 32-digit
 * form without hyphens, hex digits in either case. Braces and URN prefixes
 * are not accepted. ptr semantics follow std::from_chars: on ZUF_OK, r.ptr
 * is one past the UUID; on ZUF_ERR_INVALID it equals `first`.
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_UUID_H
#define ZUFAST_UUID_H

#include "hex.h"

typedef struct { uint8_t bytes[16]; } zuf_uuid;

#define ZUF_UUID_CHARS 36

ZUF_INLINE bool zuf_int_uuid_hex_bytes(const unsigned char *s, size_t nbytes, uint8_t *out)
{
    const uint8_t *values = zuf_int_hex_values();
    size_t i;
    for (i = 0; i < nbytes; i++) {
        uint8_t hi = values[s[2 * i]], lo = values[s[2 * i + 1]];
        if ((hi | lo) & 0xF0u) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

ZUF_INLINE zuf_result zuf_parse_uuid(const char *first, const char *last, zuf_uuid *out)
{
    const unsigned char *s = (const unsigned char *)first;
    size_t n = (size_t)(last - first);
    zuf_uuid u;
    if (n >= 36 && s[8] == '-' && s[13] == '-' && s[18] == '-' && s[23] == '-') {
        if (zuf_int_uuid_hex_bytes(s, 4, u.bytes) &&
            zuf_int_uuid_hex_bytes(s + 9, 2, u.bytes + 4) &&
            zuf_int_uuid_hex_bytes(s + 14, 2, u.bytes + 6) &&
            zuf_int_uuid_hex_bytes(s + 19, 2, u.bytes + 8) &&
            zuf_int_uuid_hex_bytes(s + 24, 6, u.bytes + 10)) {
            *out = u;
            return zuf_int_result(first + 36, ZUF_OK);
        }
        return zuf_int_result(first, ZUF_ERR_INVALID);
    }
    if (n >= 32 && zuf_int_uuid_hex_bytes(s, 16, u.bytes)) {
        *out = u;
        return zuf_int_result(first + 32, ZUF_OK);
    }
    return zuf_int_result(first, ZUF_ERR_INVALID);
}

/* Write the 36-character hyphenated form. Returns 36; writes only when
   cap >= 36; never NUL-terminates. */
ZUF_INLINE size_t zuf_format_uuid(char *dst, size_t cap, const zuf_uuid *u, bool upper)
{
    const char *digits = zuf_int_hex_digits(upper);
    size_t i;
    char *d = dst;
    if (cap < ZUF_UUID_CHARS) return ZUF_UUID_CHARS;
    for (i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) *d++ = '-';
        *d++ = digits[u->bytes[i] >> 4];
        *d++ = digits[u->bytes[i] & 15];
    }
    return ZUF_UUID_CHARS;
}

#endif /* ZUFAST_UUID_H */
