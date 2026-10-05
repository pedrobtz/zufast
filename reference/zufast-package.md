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

**Maintainer**: Pedro Baltazar <pedrobtz@gmail.com> \[copyright holder\]

Authors:

- Pedro Baltazar <pedrobtz@gmail.com> \[copyright holder\]

Other contributors:

- Bjoern Hoehrmann (UTF-8 decoder state table in
  inst/include/zufast/utf8.h) \[copyright holder\]

- Daniel Lemire (fast_float, ported to C as ffc.h) \[copyright holder\]

- João Paulo Magalhaes (fast_float, ported to C as ffc.h) \[copyright
  holder\]

- The fast_float authors (fast_float, ported to C as ffc.h) \[copyright
  holder\]

- Koleman Nix (ffc.h, the C port of fast_float) \[copyright holder\]

- Ulf Adams (Ryu) \[copyright holder\]

- Yann Collet (xxHash) \[copyright holder\]
