# zufast benchmarks

Measurements from `tools/run-benchmarks` (design §22). Not a CI gate: shared
runners are too noisy to gate on. Each section gives the median of
`bench::mark()` over 100,000 inputs (or one call, for hashing and Base64,
where "rows/sec" is calls per second) on the machine and commit stated.

The R-level numbers include R's own overhead (CHARSXP access, allocation of
the result); a C consumer calling the headers directly pays none of it.

Still to measure: Apple ARM64 (design §22 asks for x86-64 Linux and Apple
ARM64 at minimum), and the canada and mesh corpora for number parsing.

## Summary, x86-64 Linux

- **Parse doubles**: 1.4–6x faster than `as.numeric()` (6x on subnormals,
  where R's `long double` path is slow), on a par with RcppFastFloat, which
  runs the same algorithm; faster than it on long mantissas.
- **Format doubles**: 2.3–5.7x faster than `sprintf("%.17g")` and 3–7x faster
  than `as.character()`, while writing the shortest round-trip digits.
- **Dates**: 70x faster than `as.Date()`, 11x faster than `clock`;
  timestamps 35x faster than `as.POSIXct(format =)`, on a par with
  `fasttime` (15% behind it without a fraction, 25% ahead with one; it
  validates less), 8–15x faster than `clock`.
- **XXH3**: 3x faster than `digest(algo = "xxhash64")` for small inputs (call
  overhead dominates), 15% faster at 1 MiB.
- **Base64 encode**: slower than base64enc below a few KiB (R wrapper
  overhead per call), level at 1 MiB, as design §14 expected for a scalar
  codec; no reader in the family is bound by it.

## Raw results

Machine: GitHub-hosted-like cloud container, Intel(R) Xeon(R) Processor @ 2.80GHz, 4 vCPUs.
Taken at commit `0c2bcaa` (the C sources are those of the stage-9 branch).


2026-10-03, R R version 4.6.1 (2026-06-24), x86_64 Linux, zufast 0.0.0, compiler: gcc 13.3.0

### Parse doubles: small integers

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| small integers | `fast_parse_double(x)` | 4.307 ms | 207 | 2.32e+07 |
| small integers | `as.numeric(x)` | 7.406 ms | 126 | 1.35e+07 |
| small integers | `RcppFastFloat::as.double2(x)` | 5.079 ms | 172 | 1.97e+07 |

### Parse doubles: ordinary decimals

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| ordinary decimals | `fast_parse_double(x)` | 5.628 ms | 171 | 1.78e+07 |
| ordinary decimals | `as.numeric(x)` | 8.093 ms | 121 | 1.24e+07 |
| ordinary decimals | `RcppFastFloat::as.double2(x)` | 5.461 ms | 177 | 1.83e+07 |

### Parse doubles: scientific

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| scientific | `fast_parse_double(x)` | 6.228 ms | 152 | 1.61e+07 |
| scientific | `as.numeric(x)` | 11.945 ms | 82 | 8.37e+06 |
| scientific | `RcppFastFloat::as.double2(x)` | 6.716 ms | 137 | 1.49e+07 |

### Parse doubles: long mantissas

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| long mantissas | `fast_parse_double(x)` | 8.014 ms | 126 | 1.25e+07 |
| long mantissas | `as.numeric(x)` | 11.007 ms | 92 | 9.09e+06 |
| long mantissas | `RcppFastFloat::as.double2(x)` | 10.719 ms | 92 | 9.33e+06 |

### Parse doubles: subnormals

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| subnormals | `fast_parse_double(x)` | 5.295 ms | 189 | 1.89e+07 |
| subnormals | `as.numeric(x)` | 33.078 ms | 30 | 3.02e+06 |
| subnormals | `RcppFastFloat::as.double2(x)` | 6.068 ms | 162 | 1.65e+07 |

### Format doubles: random

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| random | `nchar(fast_format_double(x))` | 46.098 ms | 21 | 2.17e+06 |
| random | `nchar(sprintf("%.17g", x))` | 120.950 ms | 8 | 8.27e+05 |
| random | `nchar(as.character(x))` | 143.744 ms | 7 | 6.96e+05 |

### Format doubles: integral

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| integral | `nchar(fast_format_double(x))` | 22.762 ms | 39 | 4.39e+06 |
| integral | `nchar(sprintf("%.17g", x))` | 98.402 ms | 10 | 1.02e+06 |
| integral | `nchar(as.character(x))` | 115.640 ms | 9 | 8.65e+05 |

### Format doubles: tiny

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| tiny | `nchar(fast_format_double(x))` | 52.176 ms | 19 | 1.92e+06 |
| tiny | `nchar(sprintf("%.17g", x))` | 117.997 ms | 8 | 8.47e+05 |
| tiny | `nchar(as.character(x))` | 190.995 ms | 5 | 5.24e+05 |

### Format doubles: huge

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| huge | `nchar(fast_format_double(x))` | 32.493 ms | 29 | 3.08e+06 |
| huge | `nchar(sprintf("%.17g", x))` | 185.495 ms | 5 | 5.39e+05 |
| huge | `nchar(as.character(x))` | 233.115 ms | 4 | 4.29e+05 |

### Dates: YYYY-MM-DD

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| date | `fast_parse_date(d)` | 5.851 ms | 157 | 1.71e+07 |
| date | `as.Date(d)` | 416.053 ms | 2 | 2.4e+05 |
| date | `clock::date_parse(d)` | 66.207 ms | 14 | 1.51e+06 |

### Timestamps: ...THH:MM:SSZ

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp | `fast_parse_datetime(dt)` | 10.108 ms | 98 | 9.89e+06 |
| timestamp | `as.POSIXct(dt, format = "%Y-%m-%dT%H:%M:%SZ", tz = "UTC")` | 369.578 ms | 3 | 2.71e+05 |
| timestamp | `fasttime::fastPOSIXct(dt, tz = "UTC")` | 8.761 ms | 114 | 1.14e+07 |
| timestamp | `clock::sys_time_parse_RFC_3339(dt)` | 80.376 ms | 12 | 1.24e+06 |

### Timestamps with fraction

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp.ffffff | `fast_parse_datetime(dtf)` | 8.223 ms | 118 | 1.22e+07 |
| timestamp.ffffff | `fasttime::fastPOSIXct(dtf, tz = "UTC")` | 10.908 ms | 82 | 9.17e+06 |
| timestamp.ffffff | `clock::sys_time_parse_RFC_3339(dtf, precision = "microsecond")` | 120.054 ms | 8 | 8.33e+05 |

### Timestamps with offset

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp+hh:mm | `fast_parse_datetime(dto)` | 7.274 ms | 132 | 1.37e+07 |
| timestamp+hh:mm | `clock::sys_time_parse_RFC_3339(dto, offset = "%Ez")` | 85.919 ms | 11 | 1.16e+06 |

### XXH3, 8 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 8 B | `fast_hash(x)` | 0.003 ms | 297873 | 3.42e+05 |
| 8 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 84393 | 9.83e+04 |

### XXH3, 16 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 16 B | `fast_hash(x)` | 0.003 ms | 248853 | 3.33e+05 |
| 16 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 85079 | 9.8e+04 |

### XXH3, 32 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 32 B | `fast_hash(x)` | 0.003 ms | 232649 | 2.94e+05 |
| 32 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 80473 | 9.66e+04 |

### XXH3, 64 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 64 B | `fast_hash(x)` | 0.003 ms | 279524 | 3.46e+05 |
| 64 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 79656 | 9.71e+04 |

### XXH3, 1,024 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1024 B | `fast_hash(x)` | 0.003 ms | 244213 | 3.06e+05 |
| 1024 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 86349 | 9.67e+04 |

### XXH3, 1,048,576 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1048576 B | `fast_hash(x)` | 0.089 ms | 9117 | 1.12e+04 |
| 1048576 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.104 ms | 8564 | 9.61e+03 |

### Base64 encode, 16 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 16 B | `fast_base64_encode(x)` | 0.004 ms | 192464 | 2.43e+05 |
| 16 B | `base64enc::base64encode(x)` | 0.002 ms | 454210 | 5.21e+05 |
| 16 B | `jsonlite::base64_enc(x)` | 0.004 ms | 239728 | 2.71e+05 |

### Base64 encode, 1,024 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1024 B | `fast_base64_encode(x)` | 0.007 ms | 122413 | 1.43e+05 |
| 1024 B | `base64enc::base64encode(x)` | 0.005 ms | 200335 | 2.17e+05 |
| 1024 B | `jsonlite::base64_enc(x)` | 0.007 ms | 138229 | 1.51e+05 |

### Base64 encode, 1,048,576 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1048576 B | `fast_base64_encode(x)` | 3.330 ms | 269 | 300 |
| 1048576 B | `base64enc::base64encode(x)` | 3.209 ms | 293 | 312 |
| 1048576 B | `jsonlite::base64_enc(x)` | 3.326 ms | 274 | 301 |
