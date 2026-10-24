# Changelog

## zufast 0.1.1

- `zufast/datetime.h`: ISO 8601 week dates. `zuf_weekday_from_days()`,
  `zuf_iso_week_from_days()` (week-numbering year and week),
  `zuf_iso_weeks_in_year()` and the inverse `zuf_days_from_iso_week()`,
  exact over the whole `int32_t` day range. Week dates are computed, not
  parsed.
- `zufast/datetime.h`: `zuf_parse_date_opt()` and
  `zuf_parse_datetime_opt()` take flags. `ZUF_DT_EXPANDED_YEAR` accepts
  expanded years, a sign and exactly six digits as in Temporal
  (`+010000-01-01`, `-271821-04-20`), and rejects `-000000`.
  `zuf_parse_date()` and `zuf_parse_datetime()` keep the four-digit
  grammar.
- Breaking change for years outside 0000 to 9999: `zuf_format_date()`,
  `zuf_format_datetime()` and
  [`fast_format_datetime()`](https://pedrobtz.github.io/zufast/reference/fast_format_datetime.md)
  now pad expanded years to six digits (`+010000-01-01`,
  `-000001-01-01`), where 0.1.0 padded to five. The formatters now write
  the form that Temporal writes and that `ZUF_DT_EXPANDED_YEAR` reads.
  Years 0000 to 9999 are unchanged.
- New `zufast/wide.h`: 128-bit integers `zuf_i128` and `zuf_u128`, for
  Temporal’s epoch nanoseconds and durations in zutempo and for zuint.
  Both are two-word structs in every build. The header offers checked
  and wrapping add, subtract and multiply, the full 64 x 64 bit
  products, truncating and floor division by a 64- or 128-bit divisor,
  exact conversion to and from `double`, and decimal parsing and
  writing. The native `__int128` is used inside the functions where the
  compiler has it. Defining `ZUF_NO_INT128` selects the portable code,
  and the test suite checks that both give identical results.

## zufast 0.1.0

CRAN release: 2026-10-21

- Initial CRAN release: a header-only library of small, fast, portable
  C99 primitives, used by other packages through `LinkingTo: zufast`
  alone.
- `zufast/number.h`: correctly rounded parsing of `double`, `float` and
  32/64-bit integers in any base 2 to 36 (with a JSON mode), through the
  vendored ffc.h, a C port of fast_float; shortest round-trip formatting
  in ECMAScript notation through the vendored Ryu; exact fixed-notation
  formatting and integer writers.
- `zufast/datetime.h`: ISO 8601 / RFC 3339 date and timestamp parsing to
  fields, calendar arithmetic exact over the whole `int32_t` day range,
  and RFC 3339 formatting.
- `zufast/literal.h` (booleans with per-spelling flags, literal
  matching, ASCII whitespace), `zufast/uuid.h`, `zufast/hex.h` and
  `zufast/base64.h` (strict, standard and URL alphabets),
  `zufast/hash.h` (XXH3 64- and 128-bit, one-shot and streaming, through
  the vendored xxHash), `zufast/bits.h` (binary16, bfloat16, endian
  helpers) and `zufast/utf8.h` (UTF-8 validation and code-point
  counting).
- R functions that exercise the C layer:
  [`fast_parse_double()`](https://pedrobtz.github.io/zufast/reference/fast_parse_double.md),
  [`fast_parse_integer()`](https://pedrobtz.github.io/zufast/reference/fast_parse_double.md),
  [`fast_format_double()`](https://pedrobtz.github.io/zufast/reference/fast_format_double.md),
  [`fast_parse_date()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md),
  [`fast_parse_datetime()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md),
  [`fast_datetime_fields()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md),
  [`fast_format_datetime()`](https://pedrobtz.github.io/zufast/reference/fast_format_datetime.md),
  [`fast_hex_encode()`](https://pedrobtz.github.io/zufast/reference/fast_hex_encode.md),
  [`fast_hex_decode()`](https://pedrobtz.github.io/zufast/reference/fast_hex_encode.md),
  [`fast_base64_encode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md),
  [`fast_base64_decode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md),
  [`fast_hash()`](https://pedrobtz.github.io/zufast/reference/fast_hash.md),
  [`fast_utf8_valid()`](https://pedrobtz.github.io/zufast/reference/fast_utf8_valid.md)
  and
  [`fast_info()`](https://pedrobtz.github.io/zufast/reference/fast_info.md).
- [`vignette("linking")`](https://pedrobtz.github.io/zufast/articles/linking.md)
  gives the consumer recipe.
