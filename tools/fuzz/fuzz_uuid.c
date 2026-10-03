/* UUIDs: whatever parses formats back to the same bytes. */
#include "fuzz.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char *s = (const char *)data, *e = s + size;
    zuf_uuid u, back;
    char buf[ZUF_UUID_CHARS];
    zuf_result r;
    FUZZ_CANARY(data, size);
    r = zuf_parse_uuid(s, e, &u);
    if (r.status == ZUF_OK) {
        FUZZ_CHECK(r.ptr == s + 36 || r.ptr == s + 32);
        FUZZ_CHECK(zuf_format_uuid(buf, sizeof buf, &u, size & 1) == ZUF_UUID_CHARS);
        r = zuf_parse_uuid(buf, buf + sizeof buf, &back);
        FUZZ_CHECK(r.status == ZUF_OK && memcmp(u.bytes, back.bytes, 16) == 0);
    } else {
        FUZZ_CHECK(r.ptr == s);
    }
    return 0;
}
