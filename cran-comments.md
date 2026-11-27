## Submission

Update of zufast, a header-only library of C primitives that other packages
use through `LinkingTo`, from 0.1.0 (published 2026-10-21) to 0.1.1. It adds
128-bit integers (`zufast/wide.h`) and ISO 8601 week dates, which two
packages being prepared for CRAN need.

One behaviour change: the date formatters now pad years outside 0000 to 9999
to six digits instead of five, the form that the new expanded-year parser
reads. Years 0000 to 9999 are formatted as before.

## Vendored code

zufast carries three upstream libraries under `inst/include/zufast/vendor/`.
`inst/COPYRIGHTS`, `LICENSE.note` and `Authors@R` record their authors and
licences; the licence texts are installed under `licenses/`. The following
patches are applied:

* ffc.h: `0001-add-linkage-macros.patch` gives every function internal
  linkage in a header-only build; `0002-drop-float-equal-pragmas.patch`
  removes pragmas that silenced `-Wfloat-equal`;
  `0003-u64-overflow-at-max-digits.patch` fixes the overflow check for
  20-digit unsigned integers, as fast_float 8.3 does;
  `0004-const-tables.patch` declares its lookup tables `const`.
* Ryu: `0001-header-only.patch` turns `d2s.c` and `f2s.c` into headers with
  `static inline` entry points and routes `assert()` through a macro that
  zufast defines away.
* xxHash: not patched.

The vendored headers are compiled with `-Wpedantic`, `-Wunused-function` and
`-Wmissing-field-initializers` relaxed through `_Pragma` in
`inst/include/zufast/detail/vendor_config.h`, scoped to the vendored files
only; no diagnostic CRAN treats as important is suppressed.

## Method references

The number conversions come from published work: Ryu (Adams 2018,
<doi:10.1145/3192366.3192369>) and fast_float (Lemire 2021,
<doi:10.1002/spe.2984>). They are credited in `Authors@R` and
`inst/COPYRIGHTS`; we can add them to Description if preferred.

## Test environments

The pre-submission check is the R CMD check matrix of the
pedrobtz/r-actions `r-cmd-check.yml` workflow, run with `--as-cran` in full
on the release commit:

* GitHub Actions: macOS (R release), Windows (R release and R-devel),
  Ubuntu (R release and oldrel-1).
* CRAN-like containers: r-devel with GCC 16, clang 23, ubuntu-clang; the
  NOSUGGESTS and NOLD flavors.

Alongside it, on the same commit:

* r-devel under ASan and UBSan (GCC and clang), valgrind, LTO, rchk,
  `-fanalyzer`, and CRAN's rcnst, rlibro and vnu checks; the test suite
  under `gctorture2(step = 100)`.
* musl (Alpine) and s390x (big-endian).

## R CMD check results

0 errors | 0 warnings | 0 notes

## Reverse dependencies

We checked the three CRAN reverse dependencies (zubin, zutoml and zuyaml)
against this version, and none of them changed status.
