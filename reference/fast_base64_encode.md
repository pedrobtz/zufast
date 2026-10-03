# Base64 encoding and decoding

Standard (RFC 4648 section 4) or URL-safe (section 5) Base64. Decoding
is strict: no whitespace, nothing outside the selected alphabet, padding
either complete or absent, and the unused trailing bits zero, so that
every accepted input has exactly one encoding.

## Usage

``` r
fast_base64_encode(x, url = FALSE, pad = TRUE)

fast_base64_decode(x, url = FALSE)
```

## Arguments

- x:

  For encoding, a raw vector (encoded as one string) or a character
  vector (the bytes of each element encoded separately, `NA` kept). For
  decoding, a character vector.

- url:

  Use the URL-safe alphabet (`-` and `_` instead of `+` and `/`).

- pad:

  Write `=` padding.

## Value

`fast_base64_encode()` returns a character vector.
`fast_base64_decode()` returns a list of raw vectors, one per element of
`x`, with `NULL` where the element is `NA` or not valid Base64.

## Examples

``` r
fast_base64_encode(charToRaw("hello"))
#> [1] "aGVsbG8="
fast_base64_encode("hello", url = TRUE, pad = FALSE)
#> [1] "aGVsbG8"
rawToChar(fast_base64_decode("aGVsbG8=")[[1]])
#> [1] "hello"
```
