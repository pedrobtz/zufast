/* Includes everything and uses nothing: must compile without a single
   unused-function warning (design 4.5, 21.1). */
#include <zufast.h>

int zuf_probe_none(void);
int zuf_probe_none(void) { return 0; }
