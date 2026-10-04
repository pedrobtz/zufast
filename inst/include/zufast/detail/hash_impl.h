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

/* The type of zuf_hasher's state member, under a name a public header may
   use. */
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

ZUF_INLINE void zuf_int_xxh3_init(zuf_int_xxh3_state *state, uint64_t seed)
{
    XXH3_INITSTATE(state);
    (void)XXH3_64bits_reset_withSeed(state, seed);
}

ZUF_INLINE void zuf_int_xxh3_update(zuf_int_xxh3_state *state, const void *data, size_t n)
{
    (void)XXH3_64bits_update(state, data, n);
}

ZUF_INLINE uint64_t zuf_int_xxh3_digest64(const zuf_int_xxh3_state *state)
{
    return XXH3_64bits_digest(state);
}

ZUF_INLINE void zuf_int_xxh3_digest128(const zuf_int_xxh3_state *state, uint64_t *low, uint64_t *high)
{
    XXH128_hash_t h = XXH3_128bits_digest(state);
    *low = h.low64;
    *high = h.high64;
}

#define ZUF_INT_XXH_VERSION_NUMBER XXH_VERSION_NUMBER

#endif /* ZUFAST_DETAIL_HASH_IMPL_H */
