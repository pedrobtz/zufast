# Changelog

## zufast (development version)

- Header-only foundation: `<zufast.h>`, `zufast/version.h` and
  `zufast/status.h`, consumed through `LinkingTo: zufast` alone.
- [`fast_info()`](https://pedrobtz.github.io/zufast/reference/fast_info.md)
  reports the compiled header version.
- `zufast/literal.h`: `zuf_parse_bool()` with per-spelling flags,
  `zuf_equals()`, `zuf_equals_ci()`, `zuf_skip_space()`,
  `zuf_trim_space()`.
- `zufast/bits.h`: binary16 and bfloat16 conversion, the `fits`
  predicates, endian loads, stores and byte swaps.
- `zufast/utf8.h` and
  [`fast_utf8_valid()`](https://pedrobtz.github.io/zufast/reference/fast_utf8_valid.md):
  UTF-8 validation and code-point counting with Hoehrmann’s DFA.
- `zufast/hex.h`, `zufast/base64.h` and `zufast/uuid.h`: hex and strict
  Base64 (standard and URL alphabets, optional padding) codecs and UUID
  parsing and formatting;
  [`fast_hex_encode()`](https://pedrobtz.github.io/zufast/reference/fast_hex_encode.md),
  [`fast_hex_decode()`](https://pedrobtz.github.io/zufast/reference/fast_hex_encode.md),
  [`fast_base64_encode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md)
  and
  [`fast_base64_decode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md).
- `zufast/datetime.h`: ISO 8601 / RFC 3339 date and timestamp parsing to
  fields, Neri-Schneider calendar arithmetic exact over the whole
  `int32_t` day range, and RFC 3339 formatting;
  [`fast_parse_date()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md),
  [`fast_parse_datetime()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md),
  [`fast_datetime_fields()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md)
  and
  [`fast_format_datetime()`](https://pedrobtz.github.io/zufast/reference/fast_format_datetime.md).
