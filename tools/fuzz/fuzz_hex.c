/* Hex: whatever decodes re-encodes to the input up to case; any bytes
   encode and decode back. */
#include "fuzz.h"

static unsigned char out[1 << 16];
static char text[1 << 17];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s = (const char *)data, *e = s + size;
    size_t len, n, i;
    zuf_status st;
    FUZZ_CANARY(data, size);
    if (size > sizeof out) return 0;
    st = zuf_hex_decode(s, e, out, sizeof out, &len);
    if (st == ZUF_OK) {
        n = zuf_hex_encode(out, len, text, sizeof text, false);
        FUZZ_CHECK(n == size);
        for (i = 0; i < n; i++) {
            char c = s[i];
            if (c >= 'A' && c <= 'F') c = (char)(c + 32);
            FUZZ_CHECK(text[i] == c);
        }
    }
    n = zuf_hex_encode(data, size, text, sizeof text, size & 1);
    st = zuf_hex_decode(text, text + n, out, sizeof out, &len);
    FUZZ_CHECK(st == ZUF_OK && len == size && memcmp(out, data, size) == 0);
    return 0;
}
