#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

#include "zufast_r.h"

#define CALLDEF(name, n) {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef call_methods[] = {
    CALLDEF(zufast_info, 0),
    CALLDEF(zufast_test_status_string, 1),
    CALLDEF(zufast_test_mul128, 2),
    {NULL, NULL, 0}
};

/* The one exported symbol of zufast.so. Nothing is registered with
   R_RegisterCCallable: consumers include the headers instead (design 4). */
void attribute_visible R_init_zufast(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
