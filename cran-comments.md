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
  removes pragmas that silenced `-Wfloat-equal`.
* Ryu: `0001-header-only.patch` turns `d2s.c` and `f2s.c` into headers with
  `static inline` entry points and routes `assert()` through a macro that
  zufast defines away.
* xxHash: not patched.

The vendored headers are compiled with `-Wpedantic`, `-Wunused-function` and
`-Wmissing-field-initializers` relaxed through `_Pragma` in
`inst/include/zufast/detail/vendor_config.h`, scoped to the vendored files
only; no diagnostic CRAN treats as important is suppressed.

## R CMD check results

0 errors | 0 warnings | 0 notes
