# Information about the compiled zufast headers

Reports, from compiled code, the version of the zufast headers the
package's own shared object was built with, the versions of the vendored
libraries compiled into it, and the compiler that built it. Consumers
that use `LinkingTo: zufast` carry their own copy of the code; this
describes zufast's copy only.

## Usage

``` r
fast_info()
```

## Value

A named list with elements `version` (a string such as `"0.1.0"`),
`version_major`, `version_minor`, `version_patch` (integers), `compiler`
(a string) and `vendored` (a named character vector of upstream
versions).

## Examples

``` r
fast_info()$version
#> [1] "0.1.0"
```
