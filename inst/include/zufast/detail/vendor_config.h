/*
 * zufast/detail/vendor_config.h -- how zufast configures the vendored
 * libraries (design 18). Internal.
 *
 * The block between the markers below is checked by tools/vendor/verify
 * against the `defines` column of tools/vendor/manifest.tsv: change both or
 * neither.
 *
 * Every vendored function is given internal linkage, so that a consumer's
 * shared object contains no global vendor symbol (design 4.5, rule 1).
 */
#ifndef ZUFAST_DETAIL_VENDOR_CONFIG_H
#define ZUFAST_DETAIL_VENDOR_CONFIG_H

/* BEGIN VENDOR DEFINES */
#define FFC_IMPL
#define FFC_LINKAGE static
#define FFC_LINKAGE_EXTERN static
#define RYU_ASSERT(x) ((void)0)
#define XXH_INLINE_ALL
#define XXH_NO_STDLIB
#define XXH_DEBUGLEVEL 0
/* END VENDOR DEFINES */

/* Ryu has no version macro; the pinned tag, as in the manifest. */
#define ZUF_INT_RYU_VERSION "v2.0"

/* The vendored headers are compiled with -Wpedantic and the unused-function
   and missing-initialiser warnings relaxed, and only inside them: GCC and
   clang apply diagnostic pragmas by source location. */
#if defined(__GNUC__) || defined(__clang__)
#  define ZUF_INT_VENDOR_BEGIN                                            \
     _Pragma("GCC diagnostic push")                                       \
     _Pragma("GCC diagnostic ignored \"-Wpedantic\"")                     \
     _Pragma("GCC diagnostic ignored \"-Wunused-function\"")              \
     _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")
#  define ZUF_INT_VENDOR_END _Pragma("GCC diagnostic pop")
#else
#  define ZUF_INT_VENDOR_BEGIN
#  define ZUF_INT_VENDOR_END
#endif

#endif /* ZUFAST_DETAIL_VENDOR_CONFIG_H */
