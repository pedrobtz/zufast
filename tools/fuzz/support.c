/* The non-harness source each fuzz target links: fuzz.yml compiles a
   harness together with at least one source. */
#include "fuzz.h"

int zuf_fuzz_support(void) { return ZUFAST_VERSION_NUMBER; }
