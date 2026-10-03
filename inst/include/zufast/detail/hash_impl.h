/*
 * zufast/detail/hash_impl.h -- the wrappers between zufast/hash.h and XXH3.
 * Internal.
 */
#ifndef ZUFAST_DETAIL_HASH_IMPL_H
#define ZUFAST_DETAIL_HASH_IMPL_H

#include "portability.h"
#include "vendor_xxhash.h"

/* The size and alignment zuf_hasher reserves; checked against the state. */
#define ZUF_INT_HASHER_SIZE  640
#define ZUF_INT_HASHER_ALIGN 64

ZUF_STATIC_ASSERT(sizeof(XXH3_state_t) <= ZUF_INT_HASHER_SIZE, hash_state_fits);

/* The hasher's bytes are used as an XXH3_state_t, the aligned-storage
   pattern. xxHash accesses them only through XXH3_state_t lvalues, and
   zufast and its callers only through unsigned char, which may alias
   anything, so type-based alias analysis has no pair of accesses it may
   reorder. */
typedef XXH3_state_t zuf_int_xxh3_state;

ZUF_INLINE uint64_t zuf_int_xxh3_64(const void *data, size_t n, uint64_t seed)
{
    return XXH3_64bits_withSeed(data, n, seed);
}

ZUF_INLINE void zuf_int_xxh3_128(const void *data, size_t n, uint64_t seed, uint64_t *low, uint64_t *high)
{
    XXH128_hash_t h = XXH3_128bits_withSeed(data, n, seed);
    *low = h.low64;
    *high = h.high64;
}

ZUF_INLINE void zuf_int_xxh3_init(void *state, uint64_t seed)
{
    zuf_int_xxh3_state *s = (zuf_int_xxh3_state *)state;
    XXH3_INITSTATE(s);
    (void)XXH3_64bits_reset_withSeed(s, seed);
}

ZUF_INLINE void zuf_int_xxh3_update(void *state, const void *data, size_t n)
{
    (void)XXH3_64bits_update((zuf_int_xxh3_state *)state, data, n);
}

ZUF_INLINE uint64_t zuf_int_xxh3_digest64(const void *state)
{
    return XXH3_64bits_digest((const zuf_int_xxh3_state *)state);
}

ZUF_INLINE void zuf_int_xxh3_digest128(const void *state, uint64_t *low, uint64_t *high)
{
    XXH128_hash_t h = XXH3_128bits_digest((const zuf_int_xxh3_state *)state);
    *low = h.low64;
    *high = h.high64;
}

#define ZUF_INT_XXH_VERSION_NUMBER XXH_VERSION_NUMBER

#endif /* ZUFAST_DETAIL_HASH_IMPL_H */
