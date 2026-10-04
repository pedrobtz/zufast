/*
 * zufast/detail/portability.h -- compiler and platform macros.
 *
 * Internal: the macros and zuf_int_ helpers here are outside the
 * compatibility promise of design section 4.4.
 */
#ifndef ZUFAST_DETAIL_PORTABILITY_H
#define ZUFAST_DETAIL_PORTABILITY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Every function defined in a zufast header is static inline (design 4.5). */
#define ZUF_INLINE static inline

#if defined(__GNUC__) || defined(__clang__)
#  define ZUF_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define ZUF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define ZUF_ALIGNED(n)  __attribute__((aligned(n)))
#else
#  define ZUF_LIKELY(x)   (x)
#  define ZUF_UNLIKELY(x) (x)
#  define ZUF_ALIGNED(n)
#endif

#define ZUF_INT_CAT2(a, b) a##b
#define ZUF_INT_CAT(a, b)  ZUF_INT_CAT2(a, b)

/* ZUF_STATIC_ASSERT(cond, tag): a compile-time assertion usable at file scope
   in C99 and C++11. `tag` must be an identifier unique to the header. */
#if defined(__cplusplus) && __cplusplus >= 201103L
#  define ZUF_STATIC_ASSERT(cond, tag) static_assert(cond, #tag)
#else
#  define ZUF_STATIC_ASSERT(cond, tag) \
     typedef char ZUF_INT_CAT(zuf_int_static_assert_, tag)[(cond) ? 1 : -1]
#endif

#endif /* ZUFAST_DETAIL_PORTABILITY_H */
