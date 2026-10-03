# Format dates and timestamps as RFC 3339

Writes a `POSIXct` as `YYYY-MM-DDTHH:MM:SS[.fff]Z` in UTC, or a `Date`
as `YYYY-MM-DD`. Years outside 0000 to 9999 are written in the ISO 8601
expanded form with a sign.

## Usage

``` r
fast_format_datetime(x, digits = NULL)
```

## Arguments

- x:

  A `POSIXct` or `Date` vector.

- digits:

  For `POSIXct`, the number of fraction digits, 0 to 9. `NULL` writes
  the shortest of 0, 3 or 6 digits that represents the time at
  microsecond resolution, which is about the precision a `POSIXct`
  double holds for present-day times.

## Value

A character vector, `NA` where `x` is `NA` or not finite.

## Examples

``` r
fast_format_datetime(as.POSIXct("2024-03-01 12:30:00.25", tz = "UTC"))
#> [1] "2024-03-01T12:30:00.250Z"
fast_format_datetime(Sys.time(), digits = 0)
#> [1] "2026-10-03T21:20:53Z"
fast_format_datetime(as.Date("2024-03-01"))
#> [1] "2024-03-01"
```
