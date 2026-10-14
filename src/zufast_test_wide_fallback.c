/* The wide.h harness with the portable two-word paths: ZUF_NO_INT128 is
   defined before the header, and only wide.h is included, so this unit holds
   its own copy of every function. See zufast_test_wide_body.h. */
#define ZUF_NO_INT128 1
#include <zufast/wide.h>

#if ZUF_INT_HAVE_INT128
#  error "ZUF_NO_INT128 did not select the fallback"
#endif

#include "zufast_r.h"

#define ZUF_TW(name) zufast_test_wide_fallback_##name
#include "zufast_test_wide_body.h"
