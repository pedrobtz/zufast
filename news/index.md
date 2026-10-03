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
