# Parse numbers

`fast_parse_double()` parses decimal and scientific notation, `Inf`,
`-Inf`, `Infinity` and `NaN` in any case, with a leading `-` or `+`,
after trimming ASCII whitespace. The result is correctly rounded: the
nearest double to the decimal value, ties to even, on every platform.

## Usage

``` r
fast_parse_double(x)

fast_parse_integer(x)
```

## Arguments

- x:

  A character vector.

## Value

A double vector (`fast_parse_double()`) or an integer vector
(`fast_parse_integer()`) of the same length as `x`.

## Details

This is not exactly
[`as.numeric()`](https://rdrr.io/r/base/numeric.html): R accumulates
digits in `long double` and is not correctly rounded, so the two differ
in the last bit for roughly one ordinary decimal in five thousand
(`"0.799012"` is one), and R's result depends on whether it was built
with `long double`. Hexadecimal input, which
[`as.numeric()`](https://rdrr.io/r/base/numeric.html) accepts, is not
accepted.

`fast_parse_integer()` parses a decimal integer and returns `NA` for
anything that is not one or does not fit an R integer, rather than going
through a double as
[`as.integer()`](https://rdrr.io/r/base/integer.html) does: `"1.5"`,
`"1e3"` and `"3000000000"` are all `NA`.

Invalid input is `NA` with no warning.

## Examples

``` r
fast_parse_double(c("1.5", " -2e-3 ", "+inf", "0x10", "abc", NA))
#> [1]  1.500 -0.002    Inf     NA     NA     NA
fast_parse_integer(c("42", "+7", "2147483647", "2147483648", "1.0"))
#> [1]         42          7 2147483647         NA         NA
```
