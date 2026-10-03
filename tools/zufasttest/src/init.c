#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include "zufasttest.h"

static const R_CallMethodDef calls[] = {
    {"zt_text", (DL_FUNC) &zt_text, 1},
    {"zt_binary", (DL_FUNC) &zt_binary, 1},
    {NULL, NULL, 0}
};

void attribute_visible R_init_zufasttest(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, calls, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
