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
- `zufast/number.h`: correctly rounded parsing of `double`, `float` and
  32/64-bit integers in any base 2 to 36 through the vendored `ffc.h`
  (v26.09.01), with JSON, leading-plus, whitespace and decimal-point
  options; exact fixed-notation formatting; integer writers.
  [`fast_parse_double()`](https://pedrobtz.github.io/zufast/reference/fast_parse_double.md)
  and
  [`fast_parse_integer()`](https://pedrobtz.github.io/zufast/reference/fast_parse_double.md).
- Vendoring tooling: `tools/vendor/{fetch,record,verify}`, the manifest
  and checksums, and `tools/run-parse-corpus`.
- Shortest round-trip formatting of `double` and `float` through the
  vendored Ryu (v2.0) in ECMAScript `Number::toString` notation:
  `zuf_format_f64()`, `zuf_format_f32()`, the `_opt` variants with
  `ZUF_FMT_SCIENTIFIC` and `ZUF_FMT_TRAILING_ZERO`, `zuf_decimal_f64()`,
  `zuf_decimal_f32()`;
  [`fast_format_double()`](https://pedrobtz.github.io/zufast/reference/fast_format_double.md).
- `zufast/hash.h`: XXH3 64- and 128-bit hashing through the vendored
  `xxhash.h` (v0.8.4), one-shot and streaming with a stack-allocatable
  `zuf_hasher`;
  [`fast_hash()`](https://pedrobtz.github.io/zufast/reference/fast_hash.md).
  XXH3 is not a cryptographic hash.
- The consumer fixture `tools/zufasttest` and the `consumer` workflow: a
  package using zufast through `LinkingTo` alone builds, checks without
  a compiled-code NOTE, exports nothing but its init function, and keeps
  working with zufast uninstalled.
- `tools/run-symbol-audit`, libFuzzer targets for every parser
  (`tools/fuzz`, `tools/run-fuzz`, the `hardening` workflow) and the
  `native-checks` workflow.
