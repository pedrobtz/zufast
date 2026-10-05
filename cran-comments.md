## Submission

First submission of zufast, a header-only library of C primitives that other
packages use through `LinkingTo`. The R functions exist to test and
demonstrate the C layer.

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

## Test environments

The pre-submission check is the R CMD check matrix of the
pedrobtz/r-actions `r-cmd-check.yml` workflow, run with `--as-cran` on
every merge:

* GitHub Actions: macOS (R release), Windows (R release), Ubuntu (R release
  and oldrel-1).
* CRAN-like containers: r-devel with GCC 16, clang 23, ubuntu-clang; the
  NOSUGGESTS and NOLD flavors.

Alongside it, on every merge:

* r-devel under ASan and UBSan (GCC and clang), valgrind, LTO, rchk,
  `-fanalyzer`, and CRAN's rcnst, rlibro and vnu checks; the test suite
  under `gctorture2(step = 100)`.
* Linux i386 (32-bit), musl (Alpine) and s390x (big-endian), weekly.

## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new submission.
