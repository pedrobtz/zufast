/*
 * zufast/version.h -- the version of the zufast headers a unit is compiled against.
 *
 * zufast is header-only: a consumer carries its own copy of every function it
 * uses, so these macros describe the code compiled into the consumer, not a
 * library loaded at run time; there is no ABI.
 */
#ifndef ZUFAST_VERSION_H
#define ZUFAST_VERSION_H

#define ZUFAST_VERSION_MAJOR 0
#define ZUFAST_VERSION_MINOR 1
#define ZUFAST_VERSION_PATCH 0

#define ZUFAST_VERSION "0.1.0"

/* A single comparable number: 10000 * major + 100 * minor + patch. */
#define ZUFAST_VERSION_NUMBER \
    (ZUFAST_VERSION_MAJOR * 10000 + ZUFAST_VERSION_MINOR * 100 + ZUFAST_VERSION_PATCH)

#endif /* ZUFAST_VERSION_H */
