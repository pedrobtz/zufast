# CLAUDE.md

<!-- markdownlint-disable-next-line MD013 -->
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Shared conventions

The portfolio's shared rules are synced into `.claude/shared/` from
`pedrobtz/packages:claude/shared/`. Never edit them here. Where this file and
a shared file disagree, this file wins for this package.

@.claude/shared/workflow.md
@.claude/shared/r-package.md
@.claude/shared/ci.md
@.claude/shared/native-code.md
@.claude/shared/correctness.md
@.claude/shared/assurance.md

## What this is

zufast is a header-only library of small, portable C99 primitives for the hot
path between external text or bytes and native values: number parsing and
formatting, ISO 8601 dates, booleans, UUIDs, hex, Base64, XXH3, float16 and
endian helpers, UTF-8 validation and 128-bit integers. Other packages consume
it through `LinkingTo: zufast` alone and carry their own copy of the code, so
zufast is never a run-time dependency. The `fast_*()` R functions exist to
test, benchmark and demonstrate the C layer. Zero hard runtime dependencies.
Time zones, UUID generation, cryptography, containers and Unicode
normalisation are permanent non-goals (design §2); anything new must pass the
admission test there, with a named consumer.

[`.agents/design.md`](.agents/design.md) is the specification and outranks
this file. Benchmark results live in
[`.agents/benchmarks.md`](.agents/benchmarks.md).

## Current state

Verified 2026-11-11. 0.1.0 is on CRAN (published 2026-10-21). The default
branch is `0.1.0.9000` and carries the 0.1.1 work in `NEWS.md`: ISO week
dates, expanded-year parsing (`ZUF_DT_EXPANDED_YEAR`) and `zufast/wide.h`.
All three stage issues are closed; 0.1.1 is not yet submitted. Still
deferred, until a consumer asks: varints, CRC32C, HTTP dates, `_opt` forms of
the 128-bit parsers, week-date parsing and SIMD paths (design §25).

## Versions and dependents

The current version's parent issue is
[#34](https://github.com/pedrobtz/zufast/issues/34) for `v0.1.1`, with
`zutempo` as its named consumer. Sibling packages that build against the
headers, listed in `tools/revdep/consumers.txt`: zubin, zudedup, zumsgpack,
zunpy, zupdf, zusvg and zutoml. Adoption issues in zuxlsx, zuyaml, zucbor,
zuhttp and zucsv are tracked in design §3.2.

## Commands

All are run from the package root and are occasional; the everyday loop is
the shared `devtools` set.

```sh
tools/check-headers           # header gate: standalone C99/C++11, -Werror (§21.1)
tools/run-symbol-audit        # compiled-code NOTE audit of the full-use probe (§21.3)
tools/run-fuzz [target ...]   # libFuzzer + ASan/UBSan, canary first (§21.5)
tools/run-parse-corpus        # Nigel Tao's number corpus; network (§21.4)
tools/run-revdep [name ...]   # build every consumer against this tree
tools/run-benchmarks          # release build; record in .agents/benchmarks.md
tools/vendor/verify           # vendor tree against manifest and checksums
```

`tools/vendor/fetch` (network, maintainer only) and `tools/vendor/record`
regenerate the vendor tree and its checksums.

## Architecture

- `inst/include/zufast.h` and `inst/include/zufast/*.h` are the product, one
  header per area (layout in design §5). `detail/` holds internal helpers and
  `vendor_config.h`, which configures the vendored code. `vendor/` holds ffc.h,
  Ryu and xxHash.
- `src/` is a consumer of those headers like any other: `zufast_r.c` holds the
  `.Call` wrappers behind `R/`, and includes `<zufast.h>`. Because the package's
  own shared object contains every primitive, CRAN's sanitizer, valgrind, LTO
  and noLD flavours exercise the headers through it.
- `src/zufast_test.c` is an always-compiled test harness (`zufast_test_*`
  symbols) that drives each C function with caller-chosen buffer capacities.
  `zufast_test_wide.c` and `zufast_test_wide_fallback.c` compile one
  `wide.h` harness twice, natively and with `ZUF_NO_INT128`.
- `src/init.c` registers the entry points with `R_useDynamicSymbols(FALSE)`
  and `R_forceSymbols(TRUE)`. There is no `R_RegisterCCallable` and no static
  archive, by design (§4).
- `tools/zufasttest` is a fixture package in the exact shape of a consumer;
  `tools/zufastbench` is the benchmark package. `tools/abi/` holds the header
  probes, `tools/fuzz/` the fuzz targets and the pinned fast_float.
- The C layer never raises. R-side argument checks raise classed conditions
  through `zufast_abort()` in `R/conditions.R`, all inheriting
  `zufast_error`.

## Invariants that are easy to break

- **Every symbol a header emits is `static inline`.** A global symbol would
  let two consumers loaded together bind to each other's copy; plain `static`
  warns as unused under `-Werror` in a unit that does not call it (§4.5).
  `test-abi.R` asserts `zufast.so` exports only `R_init_zufast`.
- **Headers include only `<stddef.h>`, `<stdint.h>`, `<stdbool.h>` and
  `<string.h>`,** and compile standalone as C99 and C++11 under
  `-Wall -Wextra -Wpedantic -Werror`. Only the vendored Ryu units include
  more (§5).
- **No R header, `SEXP` or `Rf_` anywhere under `inst/include/`,** and no
  vendor identifier (`ffc_`, `XXH`, `ryu`) outside `vendor/` and
  `detail/vendor_config.h`. No vendor type in a public declaration.
- **Nothing R CMD check forbids in compiled code** (`printf`, `abort`,
  `exit`, `rand`, `stdout`, ...) and no writable data: every table is
  `const`. A violation is a NOTE in every consumer, not in zufast, which is
  why `tools/run-symbol-audit` exists (§21.3, #23).
- **Source compatibility, not ABI** (§4.4): a public function, type, macro or
  enumerator, once released, keeps its meaning, and enumerator values are
  permanent. The real gate is the reverse-dependency check of every consumer.
  `zuf_int_` helpers are visible but outside this promise.
- **Every function is pure:** no allocation, `errno`, locale, global state or
  `time.h`, so consumers may call it from any thread (§19.2).
- **`zuf_result.ptr` follows `std::from_chars`:** one past the last byte on
  success and on `ZUF_ERR_RANGE`, equal to `first` on `ZUF_ERR_INVALID`. A
  parser never requires the whole span to be consumed (§7).
- **Prefixes:** `zuf_`/`ZUF_` public C, `zuf_int_` internal, `zufast_` entry
  points and condition classes, `zufast_test_` test symbols, `fast_` R. The
  family's `zu_` is zukomp's and is never used here (§6).
- **Numbers are correctly rounded, not `as.numeric()`-exact** (§8.4). There
  is no R-exact mode; the differences from base R are documented, not bugs.

## Testing

- **External oracles:** strtod and `R_strtod()` differentials, the Ryu
  vectors (`fixtures/shortest-f64.tsv`), xxHash's published vectors
  (`fixtures/xxh3-vectors.tsv`, licence open in #25), Markus Kuhn's UTF-8
  stress cases, and R's own date functions. The parse-number-fxx corpus is
  too large for the suite and runs weekly in `parse-corpus.yaml`. The full
  plan is design §21.4.
- **Helpers:** one `helper-<area>.R` per test file, holding its `.Call`
  shims and constants so tests run shuffled. `helper-codec.R` holds R
  reference implementations of hex and Base64, so no Suggests dependency is
  needed. `helper-wide.R` calls both `wide.h` builds and requires identical
  results before returning any value. `helper-abi.R` reads the export table
  of the installed shared object with `nm`.
- **Budget:** the design requires the suite to finish in under a minute.

## Continuous integration

Shared r-actions workflows, pinned by commit: `R-CMD-check.yaml` (quick on a
pull request, full after merge or with `full-ci`; adds Windows R-devel,
NOSUGGESTS and NOLD, since a number parser must survive R without long
double), `native-checks.yml` (sanitizers, valgrind, LTO, gctorture, rchk,
analyzers, CRAN special flavours), `coverage.yaml`, `hardening.yaml` (the
libFuzzer targets; weekly for 30 minutes), `arch.yaml` (i386, musl,
big-endian s390x; weekly), `vendor.yaml` (`tools/vendor/verify`),
`vendor-upstream.yaml` (opens an issue when an upstream releases) and
`revdep.yaml` (every consumer, on header changes and weekly).

Package-owned workflows: `abi.yaml` runs `tools/check-headers` and the symbol
audit, `consumer.yaml` installs `tools/zufasttest`, proves it holds no global
zufast symbol and still loads with zufast uninstalled, `parse-corpus.yaml`
runs the number corpus weekly, and `zufastbench.yaml` runs the benchmark
package weekly. `pkgdown.yaml` builds the site.

## Vendored native code

Pins and licences are in `tools/vendor/manifest.tsv`; patches are in
`tools/patches/<source>/`, each named in the manifest, `inst/COPYRIGHTS` and
`cran-comments.md`, and each submitted upstream (#24 for ffc 0004).

- **ffc.h** (`kolemannix/ffc.h`, tag `v26.09.01`, MIT of its triple licence).
  Patches: 0001 adds the `FFC_LINKAGE` macros, 0002 drops the
  `-Wfloat-equal` pragmas, 0003 fixes the u64 overflow check at the maximum
  digit count, 0004 makes the lookup tables `const`.
- **Ryu** (`ulfjack/ryu`, tag `v2.0`, BSL-1.0 of its dual licence). Patch
  0001 turns `d2s.c` and `f2s.c` into `static inline` headers.
- **xxHash** (`Cyan4973/xxHash`, tag `v0.8.4`, BSD-2-Clause; `xxhash.h`
  only). No patches.

The defines that configure each library (`FFC_LINKAGE=static`,
`XXH_INLINE_ALL`, `XXH_NO_STDLIB`, `RYU_ASSERT`) live in
`inst/include/zufast/detail/vendor_config.h`, and `verify` checks them
against the manifest's `defines` column. Upstream licence texts are installed
under `inst/licenses/`.
