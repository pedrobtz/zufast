/*
 * zufast/detail/vendor_xxhash.h -- includes the vendored xxhash.h under
 * zufast's configuration. Internal.
 *
 * XXH_INLINE_ALL makes every xxHash function static and renames it with an
 * XXH_INLINE_ prefix, so it can neither collide with nor bind to another
 * copy of xxHash in the process. XXH_NO_STDLIB drops the malloc-backed
 * state allocators; XXH_DEBUGLEVEL 0 keeps assert() out whatever a
 * consumer defines DEBUGLEVEL to. The vector path is xxHash's compile-time
 * choice (SSE2 on x86-64, NEON on AArch64, both baseline).
 */
#ifndef ZUFAST_DETAIL_VENDOR_XXHASH_H
#define ZUFAST_DETAIL_VENDOR_XXHASH_H

#include "vendor_config.h"

ZUF_INT_VENDOR_BEGIN
#include "../vendor/xxhash.h"
ZUF_INT_VENDOR_END

#endif /* ZUFAST_DETAIL_VENDOR_XXHASH_H */
