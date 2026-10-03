/*
 * zufast/hash.h -- XXH3 hashing, 64 and 128 bits (design 15).
 *
 * XXH3 IS NOT A CRYPTOGRAPHIC HASH. It is for caches, dictionaries,
 * deduplication, fingerprints and change detection. Anything that needs
 * collision resistance against an adversary uses zucrypt.
 *
 * Results are bit-identical to xxHash's XXH3_64bits_withSeed() and
 * XXH3_128bits_withSeed() on every platform, so a zufast hash is comparable
 * with one computed elsewhere. Streaming over any split of the input gives
 * the one-shot result.
 *
 * Every function is pure (the hasher is caller-owned) and may be called
 * from any thread.
 */
#ifndef ZUFAST_HASH_H
#define ZUFAST_HASH_H

#include "detail/portability.h"
#include "detail/hash_impl.h"

/* A 128-bit digest. (Not named zuf_hash128: that is the function.) */
typedef struct { uint64_t low, high; } zuf_digest128;

ZUF_INLINE uint64_t zuf_hash64_seed(const void *data, size_t n, uint64_t seed)
{
    return zuf_int_xxh3_64(data, n, seed);
}

ZUF_INLINE uint64_t zuf_hash64(const void *data, size_t n)
{
    return zuf_int_xxh3_64(data, n, 0);
}

ZUF_INLINE zuf_digest128 zuf_hash128_seed(const void *data, size_t n, uint64_t seed)
{
    zuf_digest128 h;
    zuf_int_xxh3_128(data, n, seed, &h.low, &h.high);
    return h;
}

ZUF_INLINE zuf_digest128 zuf_hash128(const void *data, size_t n)
{
    return zuf_hash128_seed(data, n, 0);
}

/* Streaming. The state is opaque by size, not by type, so a consumer can
   put it on the stack without naming a vendor type. One hasher yields both
   the 64- and the 128-bit digest of everything passed to update. */
#define ZUF_HASHER_SIZE 640
typedef struct { ZUF_ALIGNED(64) unsigned char opaque[ZUF_HASHER_SIZE]; } zuf_hasher;

ZUF_STATIC_ASSERT(ZUF_HASHER_SIZE == ZUF_INT_HASHER_SIZE, hash_hasher_size);

ZUF_INLINE void zuf_hasher_init(zuf_hasher *h, uint64_t seed)
{
    zuf_int_xxh3_init(h->opaque, seed);
}

ZUF_INLINE void zuf_hasher_update(zuf_hasher *h, const void *data, size_t n)
{
    zuf_int_xxh3_update(h->opaque, data, n);
}

ZUF_INLINE uint64_t zuf_hasher_digest64(const zuf_hasher *h)
{
    return zuf_int_xxh3_digest64(h->opaque);
}

ZUF_INLINE zuf_digest128 zuf_hasher_digest128(const zuf_hasher *h)
{
    zuf_digest128 r;
    zuf_int_xxh3_digest128(h->opaque, &r.low, &r.high);
    return r;
}

#endif /* ZUFAST_HASH_H */
