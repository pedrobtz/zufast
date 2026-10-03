/* Base64: the decoder is strict, so whatever decodes re-encodes to exactly
   the input; any bytes encode and decode back. */
#include "fuzz.h"

static unsigned char out[1 << 16];
static char text[1 << 17];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s = (const char *)data, *e = s + size;
    uint32_t flags;
    size_t len, n;
    zuf_status st;
    FUZZ_CANARY(data, size);
    if (size > sizeof out) return 0;
    for (flags = 0; flags < 4; flags++) {
        st = zuf_base64_decode(s, e, out, sizeof out, &len, flags);
        if (st == ZUF_OK) {
            bool padded = size > 0 && data[size - 1] == '=';
            uint32_t enc_flags = (flags & ZUF_B64_URL) | ((flags & ZUF_B64_NO_PAD) && !padded ? ZUF_B64_NO_PAD : 0);
            FUZZ_CHECK(len <= zuf_base64_decode_bound(size));
            n = zuf_base64_encode(out, len, text, sizeof text, enc_flags);
            FUZZ_CHECK(n == size && memcmp(text, s, size) == 0);
        } else {
            FUZZ_CHECK(len == 0);
        }
        n = zuf_base64_encode(data, size, text, sizeof text, flags);
        FUZZ_CHECK(n <= zuf_base64_encode_bound(size));
        st = zuf_base64_decode(text, text + n, out, sizeof out, &len, flags);
        FUZZ_CHECK(st == ZUF_OK && len == size && memcmp(out, data, size) == 0);
    }
    return 0;
}
