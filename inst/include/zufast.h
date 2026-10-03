/*
 * zufast.h -- umbrella header: includes every zufast area header.
 *
 * A consumer declares `LinkingTo: zufast` and nothing else, then
 *
 *     #include <zufast.h>          everything
 *     #include <zufast/status.h>   or one area at a time
 *
 * Every function is static inline: there is nothing to link and no
 * implementation macro to define. See design section 4.
 */
#ifndef ZUFAST_H
#define ZUFAST_H

#include "zufast/version.h"
#include "zufast/status.h"
#include "zufast/literal.h"
#include "zufast/bits.h"
#include "zufast/utf8.h"
#include "zufast/hex.h"
#include "zufast/base64.h"
#include "zufast/uuid.h"
#include "zufast/datetime.h"
#include "zufast/number.h"
#include "zufast/hash.h"

#endif /* ZUFAST_H */
