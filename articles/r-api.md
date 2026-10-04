# The R functions, by example

zufast is a C library first: its R functions exist to test, benchmark
and demonstrate the headers, and they are a convenient way to see what
the C layer does. This article runs every exported function. All of them
are vectorised, keep `NA` as `NA`, and turn invalid input into `NA` (or
`NULL` in a list) without a warning. Arguments of the wrong type raise a
condition of class `zufast_invalid_argument`, which inherits from
`zufast_error`.

``` r

library(zufast)
```

## Package information

[`fast_info()`](https://pedrobtz.github.io/zufast/reference/fast_info.md)
reports, from compiled code, the version of the headers zufast’s own
shared object was built with, the vendored library versions and the
compiler.

``` r

str(fast_info())
#> List of 6
#>  $ version      : chr "0.1.0"
#>  $ version_major: int 0
#>  $ version_minor: int 1
#>  $ version_patch: int 0
#>  $ compiler     : chr "gcc 13.3.0"
#>  $ vendored     : Named chr [1:3] "26.09.01" "v2.0" "0.8.4"
#>   ..- attr(*, "names")= chr [1:3] "ffc" "ryu" "xxhash"
```

## Numbers

### `fast_parse_double()`

Decimal and scientific notation, `Inf`, `Infinity` and `NaN` in any
case, with a leading `-` or `+`, after trimming ASCII whitespace.
Anything else, including trailing characters and hexadecimal, is `NA`.

``` r

fast_parse_double(c("1.5", " -2e-3 ", "+inf", "nan", ".5", "1.", "1e400", "1e-400"))
#> [1]  1.500 -0.002    Inf    NaN  0.500  1.000    Inf  0.000
fast_parse_double(c("0x10", "1,5", "12abc", "", NA))
#> [1] NA NA NA NA NA
```

The result is correctly rounded: the nearest double, ties to even. R’s
[`as.numeric()`](https://rdrr.io/r/base/numeric.html) is not, and the
two can differ in the last bit:

``` r

x <- "0.799012"
fast_parse_double(x) == as.numeric(x)
#> [1] FALSE
sprintf("%a", c(fast_parse_double(x), as.numeric(x)))
#> [1] "0x1.991819d2391d5p-1" "0x1.991819d2391d6p-1"
```

### `fast_parse_integer()`

Decimal integers only, in R’s integer range. Unlike
[`as.integer()`](https://rdrr.io/r/base/integer.html), nothing goes
through a double, so a fraction, an exponent or an overflow is `NA`
rather than truncated.

``` r

fast_parse_integer(c("42", " +7 ", "-2147483647", "2147483648", "1.0", "1e3", "0x1A"))
#> [1]          42           7 -2147483647          NA          NA          NA
#> [7]          NA
as.integer(c("1.9", "1e3"))   # base R, for comparison
#> [1]    1 1000
```

### `fast_format_double()`

The fewest significant digits that read back to exactly the same double,
in ECMAScript `Number.prototype.toString()` notation.

``` r

x <- c(1, 0.1, 0.1 + 0.2, 1 / 3, 1e21, 1e-7, 123456.789, -0, NA, NaN, Inf)
fast_format_double(x)
#>  [1] "1"                   "0.1"                 "0.30000000000000004"
#>  [4] "0.3333333333333333"  "1e+21"               "1e-7"               
#>  [7] "123456.789"          "-0"                  NA                   
#> [10] "NaN"                 "Inf"
as.character(x)               # base R: fifteen significant digits
#>  [1] "1"                 "0.1"               "0.3"              
#>  [4] "0.333333333333333" "1e+21"             "1e-07"            
#>  [7] "123456.789"        "0"                 NA                 
#> [10] "NaN"               "Inf"
identical(fast_parse_double(fast_format_double(x)), x)
#> [1] TRUE
```

`scientific = TRUE` always writes `d.ddde+x`; `trailing_zero = TRUE`
keeps integral values recognisably floating point.

``` r

fast_format_double(c(1, 123.456, 1e-3), scientific = TRUE)
#> [1] "1e+0"       "1.23456e+2" "1e-3"
fast_format_double(c(1, 2.5, 1e20), trailing_zero = TRUE)
#> [1] "1.0"                     "2.5"                    
#> [3] "100000000000000000000.0"
```

## Dates and timestamps

### `fast_parse_date()`

`YYYY-MM-DD` with full validation in the proleptic Gregorian calendar.

``` r

fast_parse_date(c("2024-02-29", "2023-02-29", "1900-02-29", "2000-02-29",
                  "2024-13-01", "2024-1-5", "2024-01-01T00:00", NA))
#> [1] "2024-02-29" NA           NA           "2000-02-29" NA          
#> [6] NA           NA           NA
```

### `fast_parse_datetime()`

RFC 3339 and the common ISO 8601 forms: `T`, `t` or a space between date
and time, optional seconds, 1 to 9 fraction digits, and `Z` or a numeric
offset. The result is a `POSIXct` in UTC; an offset is applied, and a
timestamp without one is taken to be UTC. A bare date is midnight.

``` r

x <- c("2024-03-01T12:30:00Z",
       "2024-03-01 12:30:00.25+01:00",
       "2024-03-01t12:30-0530",
       "2016-12-31T23:59:60Z",       # leap second: the next minute
       "2024-03-01",
       "2024-03-01T24:00:00Z",       # rejected
       "2024-03-01T12:30:00,5Z")     # comma fractions are rejected
fast_parse_datetime(x)
#> [1] "2024-03-01 12:30:00 UTC" "2024-03-01 11:30:00 UTC"
#> [3] "2024-03-01 18:00:00 UTC" "2017-01-01 00:00:00 UTC"
#> [5] "2024-03-01 00:00:00 UTC" NA                       
#> [7] NA
```

### `fast_datetime_fields()`

The parsed fields themselves, for a caller that wants to apply a time
zone of its own or tell “no offset” apart from “UTC”.

``` r

fast_datetime_fields(c("2024-03-01T12:30:00.123456789-05:00",
                       "2024-03-01T12:30:00",
                       "2024-03-01",
                       "not a date"))
#>   year month day hour minute second nanosecond offset has_time has_offset
#> 1 2024     3   1   12     30      0  123456789 -18000     TRUE       TRUE
#> 2 2024     3   1   12     30      0          0     NA     TRUE      FALSE
#> 3 2024     3   1   NA     NA     NA         NA     NA    FALSE      FALSE
#> 4   NA    NA  NA   NA     NA     NA         NA     NA       NA         NA
```

### `fast_format_datetime()`

RFC 3339 in UTC for `POSIXct`, `YYYY-MM-DD` for `Date`.

``` r

t <- as.POSIXct(c("2024-03-01 12:30:00", "2024-03-01 12:30:00.25"), tz = "UTC")
fast_format_datetime(t)
#> [1] "2024-03-01T12:30:00Z"     "2024-03-01T12:30:00.250Z"
fast_format_datetime(t, digits = 0)
#> [1] "2024-03-01T12:30:00Z" "2024-03-01T12:30:00Z"
fast_format_datetime(t, digits = 6)
#> [1] "2024-03-01T12:30:00.000000Z" "2024-03-01T12:30:00.250000Z"
fast_format_datetime(as.Date(c("2024-03-01", "0001-01-01", NA)))
#> [1] "2024-03-01" "0001-01-01" NA
```

Formatting and parsing round-trip:

``` r

t <- as.POSIXct("1969-07-20 20:17:40", tz = "UTC")
identical(fast_parse_datetime(fast_format_datetime(t)), t)
#> [1] TRUE
```

## Bytes and text

### `fast_hex_encode()` and `fast_hex_decode()`

A raw vector encodes to one string; a character vector encodes each
element’s bytes. Decoding accepts either case and returns a list of raw
vectors, `NULL` where the input is `NA` or not hex.

``` r

fast_hex_encode(as.raw(c(0xde, 0xad, 0xbe, 0xef)))
#> [1] "deadbeef"
fast_hex_encode(c("zu", "fast", NA), upper = TRUE)
#> [1] "7A75"     "66617374" NA
fast_hex_decode(c("DEADbeef", "abc", "zz", NA))
#> [[1]]
#> [1] de ad be ef
#> 
#> [[2]]
#> NULL
#> 
#> [[3]]
#> NULL
#> 
#> [[4]]
#> NULL
```

### `fast_base64_encode()` and `fast_base64_decode()`

Standard or URL-safe alphabet, with or without padding.

``` r

fast_base64_encode(c("f", "fo", "foo", "foob"))
#> [1] "Zg=="     "Zm8="     "Zm9v"     "Zm9vYg=="
fast_base64_encode(as.raw(c(0xfb, 0xff, 0xbf)), url = TRUE)
#> [1] "-_-_"
fast_base64_encode("hello", url = TRUE, pad = FALSE)
#> [1] "aGVsbG8"
```

Decoding is strict, so that every accepted input has exactly one
encoding: whitespace, the other alphabet, partial padding and non-zero
unused bits are all rejected. Padding itself is optional.

``` r

decoded <- fast_base64_decode(c("aGVsbG8=", "aGVsbG8", "aGVsbG9=", "aGVs bG8=", NA))
lapply(decoded, function(r) if (is.null(r)) NULL else rawToChar(r))
#> [[1]]
#> [1] "hello"
#> 
#> [[2]]
#> [1] "hello"
#> 
#> [[3]]
#> NULL
#> 
#> [[4]]
#> NULL
#> 
#> [[5]]
#> NULL
fast_base64_decode("-_-_", url = TRUE)
#> [[1]]
#> [1] fb ff bf
```

### `fast_hash()`

XXH3, 64 or 128 bits, as lower-case hex in xxHash’s canonical form. The
values match the reference implementation. **XXH3 is not a cryptographic
hash**: use it for caches, deduplication and change detection, never
against an adversary.

``` r

fast_hash(charToRaw("hello"))
#> [1] "9555e8555c62dcfd"
fast_hash(c("hello", "world", NA))
#> [1] "9555e8555c62dcfd" "d6476c25083d69be" NA
fast_hash("hello", bits = 128)
#> [1] "b5e9c1ad071b3e7fc779cfaa5e523818"
fast_hash("hello", seed = 42)
#> [1] "bafa072f07db7937"
```

A string and its bytes hash alike:

``` r

fast_hash("hello") == fast_hash(charToRaw("hello"))
#> [1] TRUE
```

### `fast_utf8_valid()`

Well-formed UTF-8: no overlong forms, no surrogates, nothing above
U+10FFFF. The declared encoding is ignored; only the bytes are examined.

``` r

cafe <- rawToChar(as.raw(c(0x63, 0x61, 0x66, 0xc3, 0xa9)))   # "cafe" with an acute e
latin1 <- rawToChar(as.raw(c(0x63, 0x61, 0x66, 0xe9)))       # the same in Latin-1
fast_utf8_valid(c("plain", cafe, latin1, NA))
#> [1]  TRUE  TRUE FALSE    NA
fast_utf8_valid(as.raw(c(0xc0, 0xaf)))         # overlong "/"
#> [1] FALSE
fast_utf8_valid(as.raw(c(0xed, 0xa0, 0x80)))   # a surrogate
#> [1] FALSE
```

## Errors

Argument problems are classed conditions, so they can be caught
precisely:

``` r

tryCatch(fast_parse_double(1.5), zufast_invalid_argument = function(e) conditionMessage(e))
#> [1] "`x` must be a character vector."
tryCatch(fast_hash("x", bits = 32), zufast_error = function(e) class(e))
#> [1] "zufast_invalid_argument" "zufast_error"           
#> [3] "error"                   "condition"
```
