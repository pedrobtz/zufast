# zufast: Fast Portable C Primitives for Parsing and Formatting Data

A header-only library of small, fast, portable C99 primitives for
converting external text and bytes into native values and back:
correctly rounded number parsing, shortest round-trip number formatting,
ISO 8601 dates and timestamps, booleans, UUIDs, hexadecimal and Base64,
XXH3 hashing, half-precision floats, endian helpers and UTF-8
validation. Other packages use it through 'LinkingTo' alone; the R
functions exist to test, benchmark and demonstrate the C layer.

## See also

Useful links:

- <https://pedrobtz.github.io/zufast/>

- <https://github.com/pedrobtz/zufast>

- Report bugs at <https://github.com/pedrobtz/zufast/issues>

## Author

**Maintainer**: Pedro Z <pedrobtz@gmail.com>

Authors:

- Pedro Z <pedrobtz@gmail.com>

Other contributors:

- Bjoern Hoehrmann (UTF-8 decoder state table in
  inst/include/zufast/utf8.h) \[copyright holder\]
