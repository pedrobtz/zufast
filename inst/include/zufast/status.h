/*
 * zufast/status.h -- the result model shared by every parser (design section 7).
 *
 * ptr semantics follow std::from_chars: on ZUF_OK and ZUF_ERR_RANGE, ptr
 * points one past the last byte consumed; on ZUF_ERR_INVALID it equals
 * `first`. A parser never requires the whole span to be consumed: a caller
 * that needs "the entire span is a value" checks r.ptr == last.
 *
 * Every function is pure and may be called from any thread.
 */
#ifndef ZUFAST_STATUS_H
#define ZUFAST_STATUS_H

#include "detail/portability.h"

/* Enumerator values are permanent. ZUF_OK is 0 and none is negative, so
   `if (r.status)` means "not success". */
typedef enum {
    ZUF_OK = 0,
    ZUF_ERR_INVALID = 1,     /* the input is not a value of the requested kind */
    ZUF_ERR_RANGE = 2,       /* syntactically valid, not representable in the target type */
    ZUF_ERR_INCOMPLETE = 3,  /* the input ends inside a value */
    ZUF_ERR_NO_SPACE = 4     /* the output buffer is too small */
} zuf_status;

typedef struct {
    const char *ptr;     /* where parsing stopped */
    zuf_status  status;
} zuf_result;

/* A static description of `s`; never NULL, also for values outside the enum. */
ZUF_INLINE const char *zuf_status_string(zuf_status s)
{
    switch ((int)s) {
    case ZUF_OK:             return "ok";
    case ZUF_ERR_INVALID:    return "invalid input";
    case ZUF_ERR_RANGE:      return "value out of range";
    case ZUF_ERR_INCOMPLETE: return "incomplete input";
    case ZUF_ERR_NO_SPACE:   return "output buffer too small";
    default:                 return "unknown status";
    }
}

ZUF_INLINE zuf_result zuf_int_result(const char *ptr, zuf_status status)
{
    zuf_result r;
    r.ptr = ptr;
    r.status = status;
    return r;
}

#endif /* ZUFAST_STATUS_H */
