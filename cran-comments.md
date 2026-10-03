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

## R CMD check results

0 errors | 0 warnings | 0 notes
