## Submission

First submission of zufast, a header-only library of C primitives used
through `LinkingTo`.

## Vendored code

zufast carries three upstream libraries under `inst/include/zufast/vendor/`;
`inst/COPYRIGHTS` and `tools/vendor/manifest.tsv` record their authors,
licences, pinned commits and checksums. The following patches are applied:

* ffc.h: `0001-add-linkage-macros.patch` gives every function internal
  linkage in a header-only build; `0002-drop-float-equal-pragmas.patch`
  removes pragmas that silenced `-Wfloat-equal`.
* Ryu: `0001-header-only.patch` turns `d2s.c` and `f2s.c` into headers with
  `static inline` entry points and routes `assert()` through a macro that
  zufast defines away.

## R CMD check results

0 errors | 0 warnings | 0 notes
