/*
 * Shared by the libFuzzer targets in tools/fuzz (design 21.5). Each target
 * compiles the zufast headers directly, with no R, under ASan and UBSan.
 *
 * FUZZ_CHECK fails the run on a broken invariant. FUZZ_CANARY, compiled in
 * with -DZUF_FUZZ_CANARY, writes out of bounds on the input "ZUF-CANARY":
 * running a canary build on that input must crash, which proves the
 * instrumentation is live.
 */
#ifndef ZUF_FUZZ_H
#define ZUF_FUZZ_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zufast.h>

#define FUZZ_CHECK(cond) do { if (!(cond)) __builtin_trap(); } while (0)

#ifdef ZUF_FUZZ_CANARY
#  define FUZZ_CANARY(data, size)                                        \
     do {                                                                \
         if ((size) == 10 && memcmp((data), "ZUF-CANARY", 10) == 0) {    \
             volatile char small[4];                                     \
             volatile char *volatile p = small;                          \
             volatile size_t i = (size);                                 \
             p[i] = 1; /* past the end: ASan's stack-buffer-overflow */  \
         }                                                               \
     } while (0)
#else
#  define FUZZ_CANARY(data, size) ((void)0)
#endif

int zuf_fuzz_support(void);

#endif
