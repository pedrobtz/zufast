/* Hashing: the streaming hasher, fed the input in chunks the input itself
   chooses, gives the one-shot digests, 64- and 128-bit, seeded or not. The
   chunk boundaries are where a streaming hash keeps its state, so they are
   where it breaks. */
#include "fuzz.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    zuf_hasher h;
    zuf_digest128 a, b;
    uint64_t seed = 0;
    size_t pos = 0, k = 0;
    FUZZ_CANARY(data, size);
    if (size >= 9 && (data[0] & 1u)) memcpy(&seed, data + 1, 8);

    zuf_hasher_init(&h, seed);
    while (pos < size) {
        /* chunk lengths from the input itself, 0 included, up to 300 so
           that some cross the 256-byte stripe buffer */
        size_t n = (data[k % size] * 7u + k) % 301u;
        k++;
        if (n > size - pos) n = size - pos;
        zuf_hasher_update(&h, data + pos, n);
        pos += n;
    }
    FUZZ_CHECK(zuf_hasher_digest64(&h) == zuf_hash64_seed(data, size, seed));
    a = zuf_hasher_digest128(&h);
    b = zuf_hash128_seed(data, size, seed);
    FUZZ_CHECK(a.low == b.low && a.high == b.high);
    /* digesting does not consume the state */
    FUZZ_CHECK(zuf_hasher_digest64(&h) == zuf_hash64_seed(data, size, seed));
    if (seed == 0) {
        FUZZ_CHECK(zuf_hash64(data, size) == zuf_hash64_seed(data, size, 0));
        b = zuf_hash128(data, size);
        FUZZ_CHECK(a.low == b.low && a.high == b.high);
    }
    return 0;
}
