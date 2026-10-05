# Format doubles with the shortest round-trip digits

Writes each value with the fewest significant digits that parse back to
exactly the same double, in the notation of ECMAScript's
`Number.prototype.toString()`: positional for decimal exponents from -6
to 20 (`0.000001`, `0.1`, `123.5`, `100`), `d.ddde+x` otherwise
(`1e+21`, `1e-7`).

## Usage

``` r
fast_format_double(x, scientific = FALSE, trailing_zero = FALSE)
```

## Arguments

- x:

  A double vector.

- scientific:

  Always use `d.ddde+x` notation.

- trailing_zero:

  Write integral values in positional notation with a trailing `.0`, as
  in `"1.0"`, so that they read back as non-integers in languages that
  distinguish the two.

## Value

A character vector; `NA` stays `NA`, `NaN`, `Inf` and `-Inf` are written
as R writes them.

## Details

This differs from
[`as.character()`](https://rdrr.io/r/base/character.html), which writes
15 significant digits: `as.character(0.1 + 0.2)` is `"0.3"`, while
`fast_format_double(0.1 + 0.2)` is `"0.30000000000000004"`, because
those are different doubles. `fast_parse_double(fast_format_double(x))`
is identical to `x` for every double, including `-0`, which is written
as `"-0"`.

## Examples

``` r
fast_format_double(c(1, 0.1, 0.1 + 0.2, 1e21, 1e-7, -0, NA, Inf))
#> [1] "1"                   "0.1"                 "0.30000000000000004"
#> [4] "1e+21"               "1e-7"                "-0"                 
#> [7] NA                    "Inf"                
fast_format_double(123.456, scientific = TRUE)
#> [1] "1.23456e+2"
fast_format_double(c(1, 2.5), trailing_zero = TRUE)
#> [1] "1.0" "2.5"
```
