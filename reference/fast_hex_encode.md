# Hexadecimal encoding and decoding

`fast_hex_encode()` writes two hex digits per byte. `fast_hex_decode()`
accepts digits in either case and rejects an odd length or any other
byte.

## Usage

``` r
fast_hex_encode(x, upper = FALSE)

fast_hex_decode(x)
```

## Arguments

- x:

  For encoding, a raw vector (encoded as one string) or a character
  vector (the bytes of each element encoded separately, `NA` kept). For
  decoding, a character vector.

- upper:

  Use upper-case digits.

## Value

`fast_hex_encode()` returns a character vector. `fast_hex_decode()`
returns a list of raw vectors, one per element of `x`, with `NULL` where
the element is `NA` or not valid hex.

## Examples

``` r
fast_hex_encode(as.raw(c(0xde, 0xad, 0xbe, 0xef)))
#> [1] "deadbeef"
fast_hex_decode(c("DEADbeef", "abc", NA))
#> [[1]]
#> [1] de ad be ef
#> 
#> [[2]]
#> NULL
#> 
#> [[3]]
#> NULL
#> 
```
