# zufast

zufast is a header-only library of small, fast, portable C99 primitives
for the hot path between external text or bytes and native typed values,
for other R packages to use through `LinkingTo: zufast`:

| Header | What it provides |
|----|----|
| `zufast/number.h` | correctly rounded parsing of `double`, `float` and 32/64-bit integers (any base 2–36, JSON mode); shortest round-trip and exact fixed formatting; integer writers |
| `zufast/datetime.h` | ISO 8601 / RFC 3339 dates and timestamps to fields and back; calendar arithmetic |
| `zufast/literal.h` | booleans with per-spelling flags; literal matching; ASCII whitespace |
| `zufast/uuid.h` | UUID parsing and formatting |
| `zufast/hex.h`, `zufast/base64.h` | hex and strict Base64 (standard and URL alphabets) |
| `zufast/hash.h` | XXH3 64- and 128-bit hashing, one-shot and streaming (not cryptographic) |
| `zufast/bits.h` | binary16 and bfloat16 conversion; endian loads, stores and swaps |
| `zufast/utf8.h` | UTF-8 validation and code-point counting |

Every function is `static inline`, allocates nothing, holds no state,
uses no locale and may be called from any thread. Number parsing is
[ffc.h](https://github.com/kolemannix/ffc.h) (a C port of fast_float),
shortest formatting is [Ryu](https://github.com/ulfjack/ryu), hashing is
[xxHash](https://github.com/Cyan4973/xxHash); the rest is package-owned.

## Installation

Install the released version from CRAN:

``` r

install.packages("zufast")
```

or the development version from GitHub:

``` r

# install.packages("pak")
pak::pak("pedrobtz/zufast")
```

## Using zufast from C

This is the whole recipe; the fixture package `tools/zufasttest` follows
it verbatim and is built, checked and tested on Linux, macOS and Windows
on every change.

**1. `DESCRIPTION`**: add zufast to `LinkingTo`, and nothing else. No
`Imports:`, no `importFrom()`, no `configure`, no `PKG_LIBS`:

    LinkingTo: zufast

zufast is needed when your package is *built*, not when it runs: your
shared object carries its own copy of every zufast function it uses, and
keeps working if zufast is later removed.

**2. `src/Makevars` and `src/Makevars.win`**: hide your symbols, as the
zu family does:

``` make
PKG_CFLAGS = $(C_VISIBILITY)
```

It is defence in depth: every function and every vendored symbol zufast
emits is already `static`, so two packages that both use zufast never
bind to each other’s copies.

**3. Include and call.** Everything, or one area at a time:

``` c
#include <zufast.h>          /* everything */
#include <zufast/number.h>   /* or one area */

/* Parse a whole cell as a double. */
static int cell_to_double(const char *first, const char *last, double *out)
{
    zuf_result r = zuf_parse_f64(first, last, out);
    return r.status == ZUF_OK && r.ptr == last;
}
```

The one idiom to learn: parsers follow `std::from_chars`. They never
require the whole span to be consumed; `r.ptr` is one past the last byte
used, and a caller that needs “the entire span is a value” checks
`r.ptr == last`. On `ZUF_ERR_INVALID`, `r.ptr == first`.

Formatters return the length the value needs, write at most `cap` bytes,
never NUL-terminate, and measure when `cap == 0`; the `ZUF_*_MAX_CHARS`
macros are capacities that always suffice:

``` c
char buf[ZUF_F64_MAX_CHARS];
size_t n = zuf_format_f64(buf, sizeof buf, x);   /* "0.30000000000000004" */
```

Any number of translation units may include zufast; there is no
implementation macro and nothing to link. A unit that includes
`<zufast.h>` reads about fifteen thousand lines of headers, so include
only the areas you use where build time matters. The headers compile as
C99 and as C++11, so cpp11 and Rcpp code can include them too.

**Compatibility.** Within major version 1, everything a release declares
stays, with the same meaning, and enumerator values never change; a
package built against an older zufast keeps working, and rebuilding
against a newer one needs no change. `ZUFAST_VERSION_MAJOR`, `_MINOR`,
`_PATCH` and `ZUFAST_VERSION` say which headers you compiled against.
Names with a `zuf_int_` prefix are internal and outside that promise.
See
[`vignette("linking")`](https://pedrobtz.github.io/zufast/articles/linking.md)
for the contract in full.

## From R

The R functions exist to test, benchmark and demonstrate the C layer:

``` r

library(zufast)
fast_parse_double(c("0.1", "1e-7", " +inf ", "abc"))
fast_format_double(0.1 + 0.2)                       # "0.30000000000000004"
fast_parse_datetime("2024-02-29T12:30:00+01:00")
fast_base64_encode(charToRaw("hello"), url = TRUE)
fast_hash("hello", bits = 128)
fast_info()
```

Two deliberate differences from base R:
[`fast_parse_double()`](https://pedrobtz.github.io/zufast/reference/fast_parse_double.md)
is correctly rounded where
[`as.numeric()`](https://rdrr.io/r/base/numeric.html) is not (they
differ in the last bit for about one decimal in five thousand), and
[`fast_format_double()`](https://pedrobtz.github.io/zufast/reference/fast_format_double.md)
writes the shortest digits that round-trip where
[`as.character()`](https://rdrr.io/r/base/character.html) writes fifteen
significant digits.

## Consumers

Adoption is tracked in each consumer’s repository and waits on zufast
reaching CRAN: [zuxlsx#71](https://github.com/pedrobtz/zuxlsx/issues/71)
(numeric cells),
[zuyaml#14](https://github.com/pedrobtz/zuyaml/issues/14) (scalars),
[zucbor#41](https://github.com/pedrobtz/zucbor/issues/41) (float16,
Base64, dates, UUIDs, big-endian loads),
[zuhttp#79](https://github.com/pedrobtz/zuhttp/issues/79) (Base64, hex),
[zucsv#11](https://github.com/pedrobtz/zucsv/issues/11) (UTF-8,
booleans).

## Licence

MIT for zufast. The vendored libraries keep their own licences (MIT for
ffc.h, Boost 1.0 for Ryu, BSD 2-Clause for xxHash); see
`inst/COPYRIGHTS` and `LICENSE.note`.
