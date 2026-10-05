# Changelog

## zufast (development version)

- `tools/run-symbol-audit` scans with R’s own
  `tools:::check_so_symbols()` instead of a hand-written list, which
  missed glibc’s `__printf_chk` (what `printf()` becomes under R’s
  default `-D_FORTIFY_SOURCE`) and `sprintf`. A planted `printf()`
  canary must fail the audit before the real run.
- `zuf_base64_encode()` with `ZUF_B64_NO_PAD` returns `SIZE_MAX` when
  the encoded length does not fit a `size_t`, as the padded form does;
  before, the length wrapped and a measuring call could write through
  `NULL` ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- `ZUF_NUM_JSON` now applies to the integer parsers: a leading zero, a
  leading `+` (even with `ZUF_NUM_LEADING_PLUS`) and any base other than
  10 are `ZUF_ERR_INVALID`, as for floating point
  ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- The floating-point parsers no longer consume a `nan(...)` payload,
  which the documented grammar excludes: the number ends after `nan`, so
  `fast_parse_double("nan(1)")` is `NA`
  ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- `zuf_hasher` is a union holding a real XXH3 state object, so xxHash’s
  typed accesses no longer rely on reading a character array as another
  type. Its size (640 bytes) and alignment (64) are unchanged
  ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- [`fast_hex_encode()`](https://pedrobtz.github.io/zufast/reference/fast_hex_encode.md)
  and
  [`fast_base64_encode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md)
  raise an error when the result would exceed R’s string length limit,
  before allocating
  ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- The header gate’s no-int128 check builds the vendored portable
  multiplies (by undefining `__SIZEOF_INT128__`) and requires them to
  match the native ones; the old `-DZUF_NO_INT128` build selected
  nothing ([\#21](https://github.com/pedrobtz/zufast/issues/21)).
- `zuf_parse_bool()` dispatches on the first byte instead of scanning
  every spelling (about 6x faster on mixed cells); the longest-match
  rule is unchanged.
- `zuf_base64_decode()` reads its input once (about 2x faster). It and
  `zuf_hex_decode()` now share one error contract: a byte outside the
  alphabet takes precedence over every other error (so
  `zuf_hex_decode()` reports `ZUF_ERR_INVALID`, not `ZUF_ERR_NO_SPACE`,
  for a short buffer and bad input), `ZUF_ERR_NO_SPACE` writes nothing,
  and on `ZUF_ERR_INVALID` the output may have been partly written.
- Date parsing validates and decodes `YYYY-MM-` with SWAR on one 8-byte
  load, and the time and offset fields are checked and decoded in one
  pass (dates about 1.5x, timestamps about 2x faster).
- `ZUF_DATETIME_MAX_CHARS` is now 42 and covers every `int32_t` year, as
  the other `ZUF_*_MAX_CHARS` capacities do.
- The formatters return before `memcpy()` when `cap == 0`, so measuring
  with a `NULL` buffer no longer trips `-Wnonnull` under
  `_FORTIFY_SOURCE`.
- `zuf_hasher`’s 64-byte alignment is checked by a static assertion, and
  the public `ZUF_NUM_*` and `ZUF_FMT_*` flags are tied to their
  internal copies the same way.
- [`fast_base64_decode()`](https://pedrobtz.github.io/zufast/reference/fast_base64_encode.md)
  allocates each result once, and
  [`fast_datetime_fields()`](https://pedrobtz.github.io/zufast/reference/fast_parse_date.md)
  no longer looks up its columns per element.
- [`fast_info()`](https://pedrobtz.github.io/zufast/reference/fast_info.md)
  reports build flags in a new `build` element.
- Removed the unused internal `zuf_int_mul128()`.

## zufast 0.1.0

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
- `README.md` gives the consumer recipe in full;
  [`vignette("linking")`](https://pedrobtz.github.io/zufast/articles/linking.md)
  states the source-compatibility contract and the two deliberate
  differences from base R; `LICENSE.note`, `cran-comments.md`;
  `tools/benchmarks.R`, `tools/run-benchmarks` and first measurements in
  `.agents/benchmarks.md`.
