# Parse ISO 8601 / RFC 3339 dates and timestamps

`fast_parse_date()` parses `YYYY-MM-DD`. `fast_parse_datetime()` parses
a date, optionally followed by `T`, `t` or a space and `HH:MM`,
`HH:MM:SS` or `HH:MM:SS.fff` (1 to 9 fraction digits; further digits are
discarded), optionally followed by `Z`, `z`, `+HH:MM`, `+HHMM` or `+HH`
(or `-`). `fast_datetime_fields()` returns the parsed fields themselves,
for callers that apply a time zone of their own.

## Usage

``` r
fast_parse_date(x)

fast_parse_datetime(x)

fast_datetime_fields(x)
```

## Arguments

- x:

  A character vector.

## Value

`fast_parse_date()` returns a `Date` vector. `fast_parse_datetime()`
returns a `POSIXct` vector in UTC: an offset is applied, and a timestamp
without one is taken to be UTC. `fast_datetime_fields()` returns a data
frame with integer columns `year`, `month`, `day`, `hour`, `minute`,
`second`, `nanosecond`, `offset` (seconds east of UTC, `NA` when none
was given) and logical columns `has_time` and `has_offset`.

## Details

Validation is complete: year 0000 to 9999, month 1 to 12, a day that
exists in the proleptic Gregorian calendar, hour 0 to 23, minute 0 to
59, second 0 to 60 (a leap second counts as the first second of the next
minute), an offset within 23:59. Anything else, including a string with
trailing bytes, is `NA` without a warning.

## Examples

``` r
fast_parse_date(c("2024-02-29", "2023-02-29", NA))
#> [1] "2024-02-29" NA           NA          
fast_parse_datetime(c("2024-03-01T12:30:00Z", "2024-03-01 12:30:00.25+01:00"))
#> [1] "2024-03-01 12:30:00 UTC" "2024-03-01 11:30:00 UTC"
fast_datetime_fields("2024-03-01T12:30:00-05:00")
#>   year month day hour minute second nanosecond offset has_time has_offset
#> 1 2024     3   1   12     30      0          0 -18000     TRUE       TRUE
```
