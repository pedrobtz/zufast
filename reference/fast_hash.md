# XXH3 hashes

Computes the 64- or 128-bit XXH3 hash of raw bytes or of the bytes of
each string, as lower-case hexadecimal in xxHash's canonical
(big-endian) form. The values are identical to those of the reference
implementation, so they can be compared with hashes computed elsewhere.

## Usage

``` r
fast_hash(x, bits = 64, seed = 0)
```

## Arguments

- x:

  A raw vector (hashed as one input) or a character vector (each element
  hashed separately; the encoding is not converted, the bytes are hashed
  as stored).

- bits:

  `64` or `128`.

- seed:

  A whole number from 0 to 2^53.

## Value

A character vector of 16 (64-bit) or 32 (128-bit) hex digits, `NA` where
`x` is `NA`.

## Details

**XXH3 is not a cryptographic hash.** Use it for caches, deduplication,
fingerprints and change detection, never where an adversary could choose
the input to produce a collision; that needs a cryptographic digest.

## Examples

``` r
fast_hash(charToRaw("hello"))
#> [1] "9555e8555c62dcfd"
fast_hash(c("a", "b", NA), bits = 128)
#> [1] "a96faf705af16834e6c632b61e964e1f" "4b2212e31ac97fd4575a0b1c44d8843f"
#> [3] NA                                
fast_hash("hello", seed = 42)
#> [1] "bafa072f07db7937"
```
