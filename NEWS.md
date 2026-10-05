# zufast 0.1.0

* Initial CRAN release: a header-only library of small, fast, portable C99
  primitives, used by other packages through `LinkingTo: zufast` alone.
* `zufast/number.h`: correctly rounded parsing of `double`, `float` and
  32/64-bit integers in any base 2 to 36 (with a JSON mode), through the
  vendored ffc.h, a C port of fast_float; shortest round-trip formatting in
  ECMAScript notation through the vendored Ryu; exact fixed-notation
  formatting and integer writers.
* `zufast/datetime.h`: ISO 8601 / RFC 3339 date and timestamp parsing to
  fields, calendar arithmetic exact over the whole `int32_t` day range, and
  RFC 3339 formatting.
* `zufast/literal.h` (booleans with per-spelling flags, literal matching,
  ASCII whitespace), `zufast/uuid.h`, `zufast/hex.h` and `zufast/base64.h`
  (strict, standard and URL alphabets), `zufast/hash.h` (XXH3 64- and
  128-bit, one-shot and streaming, through the vendored xxHash),
  `zufast/bits.h` (binary16, bfloat16, endian helpers) and `zufast/utf8.h`
  (UTF-8 validation and code-point counting).
* R functions that exercise the C layer: `fast_parse_double()`,
  `fast_parse_integer()`, `fast_format_double()`, `fast_parse_date()`,
  `fast_parse_datetime()`, `fast_datetime_fields()`,
  `fast_format_datetime()`, `fast_hex_encode()`, `fast_hex_decode()`,
  `fast_base64_encode()`, `fast_base64_decode()`, `fast_hash()`,
  `fast_utf8_valid()` and `fast_info()`.
* `vignette("linking")` gives the consumer recipe and the
  source-compatibility contract.
