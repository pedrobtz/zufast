/* UTF-8: the DFA agrees with a direct transcription of Unicode Table 3-7,
   and the count agrees with the validity flag. */
#include "fuzz.h"

static long ref_count(const unsigned char *s, size_t n)
{
    size_t i = 0, k;
    long count = 0;
    while (i < n) {
        unsigned c = s[i], lo = 0x80, hi = 0xBF;
        size_t len;
        if (c < 0x80) { i++; count++; continue; }
        if (c >= 0xC2 && c <= 0xDF) len = 2;
        else if (c >= 0xE0 && c <= 0xEF) { len = 3; if (c == 0xE0) lo = 0xA0; if (c == 0xED) hi = 0x9F; }
        else if (c >= 0xF0 && c <= 0xF4) { len = 4; if (c == 0xF0) lo = 0x90; if (c == 0xF4) hi = 0x8F; }
        else return -1;
        if (n - i < len || s[i + 1] < lo || s[i + 1] > hi) return -1;
        for (k = 2; k < len; k++) if (s[i + k] < 0x80 || s[i + k] > 0xBF) return -1;
        i += len;
        count++;
    }
    return count;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    long ref;
    bool valid;
    size_t count;
    FUZZ_CANARY(data, size);
    ref = ref_count(data, size);
    count = zuf_utf8_count((const char *)data, size, &valid);
    FUZZ_CHECK(zuf_utf8_valid((const char *)data, size) == (ref >= 0));
    FUZZ_CHECK(valid == (ref >= 0));
    FUZZ_CHECK(ref < 0 || (long)count == ref);
    return 0;
}
