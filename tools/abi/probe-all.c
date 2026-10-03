/* Calls every public function: must compile warning-free as C99 and C++11
   (design 21.1). Each area adds its calls here as it lands. */
#include <zufast.h>

int zuf_probe_all(void);
int zuf_probe_all(void)
{
    int acc = 0;

    /* version.h */
    acc += ZUFAST_VERSION_NUMBER >= 0;
    acc += (int)sizeof(ZUFAST_VERSION);

    /* status.h */
    zuf_result r;
    r.ptr = ZUFAST_VERSION;
    r.status = ZUF_OK;
    acc += (int)zuf_status_string(r.status)[0];
    acc += (int)zuf_status_string(ZUF_ERR_NO_SPACE)[0];

    return acc;
}
