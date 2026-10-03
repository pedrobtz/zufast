# Information about the compiled zufast headers

Reports, from compiled code, the version of the zufast headers the
package's own shared object was built with and the compiler that built
it. Consumers that use `LinkingTo: zufast` carry their own copy of the
code; this describes zufast's copy only.

## Usage

``` r
fast_info()
```

## Value

A named list with elements `version` (a string such as `"0.1.0"`),
`version_major`, `version_minor`, `version_patch` (integers) and
`compiler` (a string).

## Examples

``` r
fast_info()$version
#> [1] "0.0.0"
```
