# zufast (development version)

* Header-only foundation: `<zufast.h>`, `zufast/version.h` and
  `zufast/status.h`, consumed through `LinkingTo: zufast` alone.
* `fast_info()` reports the compiled header version.
* `zufast/literal.h`: `zuf_parse_bool()` with per-spelling flags,
  `zuf_equals()`, `zuf_equals_ci()`, `zuf_skip_space()`, `zuf_trim_space()`.
* `zufast/bits.h`: binary16 and bfloat16 conversion, the `fits` predicates,
  endian loads, stores and byte swaps.
* `zufast/utf8.h` and `fast_utf8_valid()`: UTF-8 validation and code-point
  counting with Hoehrmann's DFA.
* `zufast/hex.h`, `zufast/base64.h` and `zufast/uuid.h`: hex and strict
  Base64 (standard and URL alphabets, optional padding) codecs and UUID
  parsing and formatting; `fast_hex_encode()`, `fast_hex_decode()`,
  `fast_base64_encode()` and `fast_base64_decode()`.
