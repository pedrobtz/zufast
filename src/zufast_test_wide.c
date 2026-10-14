/* The wide.h harness with the native 128-bit paths, where the compiler has
   them. See zufast_test_wide_body.h. */
#include <zufast/wide.h>

#include "zufast_r.h"

#define ZUF_TW(name) zufast_test_wide_##name
#include "zufast_test_wide_body.h"
