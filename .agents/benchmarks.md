# zufast benchmarks

Measurements from `tools/run-benchmarks` (design §22). Not a CI gate: shared
runners are too noisy to gate on. The script builds zufast and
tools/zufastbench into tarballs, installs them into a fresh library, and
refuses to run unless every compiled unit is optimised and built with
`NDEBUG`, so a debug build cannot be measured by accident.

`tools/benchmarks.R` gives the median of `bench::mark()` over 100,000 inputs
(or one call, for hashing and Base64, where "rows/sec" is calls per second).
Those numbers include R's own overhead (CHARSXP access, allocating the
result); a C consumer calling the headers pays none of it.
`tools/zufastbench` runs RcppFastFloat's benchmark with zufast added: one
million doubles, 100 runs, each parser in the same C++ harness.

Still to measure: Apple ARM64 (design §22 asks for x86-64 Linux and Apple
ARM64 at minimum), and the canada and mesh corpora for number parsing.

An earlier version of this file was replaced: its run installed zufast with
a plain `R CMD INSTALL .` after `devtools::test()`, which can reuse objects
compiled with `-O0 -UNDEBUG`, so it may have measured a debug build.

## Summary, x86-64 Linux, release build

Repeated runs on this machine vary by up to about 50% for some rows (the
date parsers most of all); read differences under 1.5x as noise.

- **Parse doubles**: zufast and fast_float are the same algorithm and
  produce identical doubles; in the same C++ harness they are level (zufast
  83 ms, fast_float 97 ms per million with a `std::string` copy; 55 and
  57 ms parsing in place). Both are 2.7x faster than `strtod`, `atof` and
  `std::stod`, 4.6x faster than `sscanf`. Parsing in place instead of
  through `std::string` saves a third. From R, `fast_parse_double()` is
  1.4-5.9x faster than `as.numeric()` (5.9x on subnormals) and level with
  `RcppFastFloat::as.double2()`, ahead of it on long mantissas.
- **Format doubles**: 2.8-4.3x faster than `sprintf("%.17g")` and 3-5.5x
  faster than `as.character()`, while writing the shortest round-trip
  digits.
- **Dates**: 44x faster than `as.Date()`, 7x faster than `clock`.
  Timestamps: 27x faster than `as.POSIXct(format =)`, 5-7x faster than
  `clock`, and 15-25% behind `fasttime` in this run (ahead of it in an
  earlier one); fasttime validates less.
- **XXH3**: 3x faster than `digest(algo = "xxhash64")` for small inputs,
  where call overhead dominates; 8% faster at 1 MiB.
- **Base64 encode**: slower than base64enc below a few KiB (R wrapper
  overhead per call), level at 1 MiB, as design §14 expected for a scalar
  codec.

## Raw results

Machine: cloud container, Intel Xeon @ 2.80GHz, 4 vCPUs.

Taken at commit `1dd42cf` (stage branch `bench-release-build`, C sources identical to main `e016cb6`).


2026-10-03, R R version 4.6.1 (2026-06-24), x86_64 Linux, zufast 0.0.0, compiler: gcc 13.3.0

### Parse doubles: small integers

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| small integers | `fast_parse_double(x)` | 3.828 ms | 237 | 2.61e+07 |
| small integers | `as.numeric(x)` | 6.703 ms | 149 | 1.49e+07 |
| small integers | `RcppFastFloat::as.double2(x)` | 4.177 ms | 214 | 2.39e+07 |

### Parse doubles: ordinary decimals

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| ordinary decimals | `fast_parse_double(x)` | 4.999 ms | 188 | 2e+07 |
| ordinary decimals | `as.numeric(x)` | 7.982 ms | 124 | 1.25e+07 |
| ordinary decimals | `RcppFastFloat::as.double2(x)` | 4.615 ms | 199 | 2.17e+07 |

### Parse doubles: scientific

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| scientific | `fast_parse_double(x)` | 6.246 ms | 142 | 1.6e+07 |
| scientific | `as.numeric(x)` | 11.838 ms | 83 | 8.45e+06 |
| scientific | `RcppFastFloat::as.double2(x)` | 6.790 ms | 146 | 1.47e+07 |

### Parse doubles: long mantissas

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| long mantissas | `fast_parse_double(x)` | 8.240 ms | 121 | 1.21e+07 |
| long mantissas | `as.numeric(x)` | 11.693 ms | 83 | 8.55e+06 |
| long mantissas | `RcppFastFloat::as.double2(x)` | 10.887 ms | 84 | 9.19e+06 |

### Parse doubles: subnormals

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| subnormals | `fast_parse_double(x)` | 5.623 ms | 178 | 1.78e+07 |
| subnormals | `as.numeric(x)` | 33.124 ms | 30 | 3.02e+06 |
| subnormals | `RcppFastFloat::as.double2(x)` | 5.538 ms | 180 | 1.81e+07 |

### Format doubles: random

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| random | `nchar(fast_format_double(x))` | 44.444 ms | 22 | 2.25e+06 |
| random | `nchar(sprintf("%.17g", x))` | 124.059 ms | 8 | 8.06e+05 |
| random | `nchar(as.character(x))` | 132.133 ms | 7 | 7.57e+05 |

### Format doubles: integral

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| integral | `nchar(fast_format_double(x))` | 20.835 ms | 48 | 4.8e+06 |
| integral | `nchar(sprintf("%.17g", x))` | 88.994 ms | 11 | 1.12e+06 |
| integral | `nchar(as.character(x))` | 115.608 ms | 9 | 8.65e+05 |

### Format doubles: tiny

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| tiny | `nchar(fast_format_double(x))` | 35.374 ms | 28 | 2.83e+06 |
| tiny | `nchar(sprintf("%.17g", x))` | 98.318 ms | 10 | 1.02e+06 |
| tiny | `nchar(as.character(x))` | 142.463 ms | 7 | 7.02e+05 |

### Format doubles: huge

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| huge | `nchar(fast_format_double(x))` | 48.214 ms | 19 | 2.07e+06 |
| huge | `nchar(sprintf("%.17g", x))` | 201.412 ms | 5 | 4.96e+05 |
| huge | `nchar(as.character(x))` | 225.880 ms | 4 | 4.43e+05 |

### Dates: YYYY-MM-DD

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| date | `fast_parse_date(d)` | 9.436 ms | 102 | 1.06e+07 |
| date | `as.Date(d)` | 419.790 ms | 2 | 2.38e+05 |
| date | `clock::date_parse(d)` | 69.853 ms | 14 | 1.43e+06 |

### Timestamps: ...THH:MM:SSZ

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp | `fast_parse_datetime(dt)` | 13.811 ms | 71 | 7.24e+06 |
| timestamp | `as.POSIXct(dt, format = "%Y-%m-%dT%H:%M:%SZ", tz = "UTC")` | 370.989 ms | 3 | 2.7e+05 |
| timestamp | `fasttime::fastPOSIXct(dt, tz = "UTC")` | 10.853 ms | 83 | 9.21e+06 |
| timestamp | `clock::sys_time_parse_RFC_3339(dt)` | 93.860 ms | 10 | 1.07e+06 |

### Timestamps with fraction

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp.ffffff | `fast_parse_datetime(dtf)` | 19.128 ms | 48 | 5.23e+06 |
| timestamp.ffffff | `fasttime::fastPOSIXct(dtf, tz = "UTC")` | 16.826 ms | 59 | 5.94e+06 |
| timestamp.ffffff | `clock::sys_time_parse_RFC_3339(dtf, precision = "microsecond")` | 129.007 ms | 8 | 7.75e+05 |

### Timestamps with offset

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| timestamp+hh:mm | `fast_parse_datetime(dto)` | 23.980 ms | 38 | 4.17e+06 |
| timestamp+hh:mm | `clock::sys_time_parse_RFC_3339(dto, offset = "%Ez")` | 110.829 ms | 9 | 9.02e+05 |

### XXH3, 8 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 8 B | `fast_hash(x)` | 0.005 ms | 165736 | 1.94e+05 |
| 8 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 77296 | 9.73e+04 |

### XXH3, 16 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 16 B | `fast_hash(x)` | 0.003 ms | 312847 | 3.68e+05 |
| 16 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 87482 | 1.02e+05 |

### XXH3, 32 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 32 B | `fast_hash(x)` | 0.003 ms | 284135 | 3.34e+05 |
| 32 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 87674 | 1.02e+05 |

### XXH3, 64 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 64 B | `fast_hash(x)` | 0.003 ms | 286537 | 3.56e+05 |
| 64 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 93453 | 1.03e+05 |

### XXH3, 1,024 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1024 B | `fast_hash(x)` | 0.003 ms | 282648 | 3.22e+05 |
| 1024 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.010 ms | 83654 | 9.95e+04 |

### XXH3, 1,048,576 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1048576 B | `fast_hash(x)` | 0.094 ms | 8319 | 1.07e+04 |
| 1048576 B | `digest::digest(x, algo = "xxhash64", serialize = FALSE)` | 0.102 ms | 9274 | 9.84e+03 |

### Base64 encode, 16 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 16 B | `fast_base64_encode(x)` | 0.004 ms | 204950 | 2.43e+05 |
| 16 B | `base64enc::base64encode(x)` | 0.002 ms | 496794 | 5.86e+05 |
| 16 B | `jsonlite::base64_enc(x)` | 0.004 ms | 213818 | 2.46e+05 |

### Base64 encode, 1,024 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1024 B | `fast_base64_encode(x)` | 0.007 ms | 112937 | 1.36e+05 |
| 1024 B | `base64enc::base64encode(x)` | 0.005 ms | 162212 | 1.98e+05 |
| 1024 B | `jsonlite::base64_enc(x)` | 0.006 ms | 136795 | 1.54e+05 |

### Base64 encode, 1,048,576 bytes

| input | expression | median | itr/sec | rows/sec |
|---|---|---:|---:|---:|
| 1048576 B | `fast_base64_encode(x)` | 3.195 ms | 284 | 313 |
| 1048576 B | `base64enc::base64encode(x)` | 3.192 ms | 297 | 313 |
| 1048576 B | `jsonlite::base64_enc(x)` | 3.304 ms | 290 | 303 |

### RcppFastFloat's benchmark, with zufast added

```
Unit: milliseconds
            expr       min        lq      mean    median        uq        max
           scanf 349.05170 365.39489 385.99828 380.53518 399.07448  492.06317
            atof 208.35467 218.37709 235.05273 228.87209 244.24779  357.39736
          strtod 209.54751 217.77033 257.58916 225.54850 242.27054 2453.05392
            stod 212.94081 222.64420 237.12937 229.17917 244.67953  325.34850
       fastfloat  87.06003  93.59608 100.39360  96.86121 103.22285  150.05145
          zufast  75.77735  79.69126  86.47897  83.17745  90.89615  121.67439
 fastfloat_spans  51.30429  53.36338  59.51796  56.87494  61.92551   90.27005
    zufast_spans  49.16262  52.37847  57.07562  54.60259  59.09980  102.81626
 neval
   100
   100
   100
   100
   100
   100
   100
   100
```
