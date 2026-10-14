test_that("every form of design 10.1 parses to the right fields", {
  expect_identical(pdt("2024-02-29")[1:3], list(OK, 10L, fields(2024, 2, 29)))
  expect_identical(pdt("2024-02-29T13:45")[1:3], list(OK, 16L, fields(2024, 2, 29, 13, 45, has_time = 1)))
  expect_identical(pdt("2024-02-29T13:45:59")[1:3],
                   list(OK, 19L, fields(2024, 2, 29, 13, 45, 59, has_time = 1)))
  expect_identical(pdt("2024-02-29T13:45:59.5")[[3]][7], 5e8)
  expect_identical(pdt("2024-02-29T13:45:59.123456789")[[3]][7], 123456789)
  expect_identical(pdt("2024-02-29T13:45:59.1234567891234")[1:2], list(OK, 33L))
  expect_identical(pdt("2024-02-29T13:45:59.1234567891234")[[3]][7], 123456789)
  expect_identical(pdt("2024-02-29T13:45:59Z")[[3]], fields(2024, 2, 29, 13, 45, 59, 0, 0, 1, 1))
  expect_identical(pdt("2024-02-29T13:45:59z")[[3]][10], 1)
  for (off in c("+01:30", "+0130")) {
    expect_identical(pdt(paste0("2024-01-01T00:00:00", off))[[3]][8], 5400)
  }
  expect_identical(pdt("2024-01-01T00:00:00+01")[[3]][8], 3600)
  expect_identical(pdt("2024-01-01T00:00:00-23:59")[[3]][8], -86340)
  # -00:00 is not told apart from +00:00 (documented at has_offset)
  expect_identical(pdt("2024-01-01T00:00:00-00:00")[[3]][c(8, 10)], c(0, 1))
  expect_identical(pdt("2024-01-01 10:00:00")[[3]][4], 10)
  expect_identical(pdt("2024-01-01t10:00:00")[[3]][4], 10)
  expect_identical(pdt("2024-01-01T10:00+02:00")[[3]][c(4, 8)], c(10, 7200))
  expect_identical(pdt("2016-12-31T23:59:60Z")[[3]][6], 60)
})

test_that("bare dates stop at bytes that cannot start a time", {
  expect_identical(pdt("2024-01-01,x")[1:2], list(OK, 10L))
  expect_identical(pdt("2024-01-01 ")[1:2], list(OK, 10L))
  expect_identical(pdt("2024-01-01 x")[1:2], list(OK, 10L))
  expect_identical(pdt("2024-01-01X10:00")[1:2], list(OK, 10L))
  expect_identical(pdt("2024-01-01T10:00:00 more")[1:2], list(OK, 19L))
  expect_identical(pdt("2024-01-01T10:00:00,5")[1:2], list(OK, 19L))   # no comma fractions
  expect_identical(pdt("2024-01-01T10:00:00", TRUE)[1:2], list(OK, 10L))
})

test_that("every invalid class is rejected", {
  bad <- c("2023-02-29", "1900-02-29", "2100-02-29", "2024-13-01", "2024-00-10",
           "2024-04-31", "2024-01-00", "2024-01-32", "24-01-01", "2024/01/01",
           "+2024-01-01", "20240101", "2024-W01-1", "2024-001",
           "2024-01-01T24:00:00", "2024-01-01T23:60", "2024-01-01T23:59:61",
           "2024-01-01T10", "2024-01-01T1:00",
           "2024-01-01T10:00:00.Z", "2024-01-01T10:00:00+24:00",
           "2024-01-01T10:00:00+01:60", "2024-01-01T10:00:00+1", "2024-01-01T10:00:00+01:0x",
           "2024-01-01Tx", "2024-1-01", " 2024-01-01", "")
  for (s in bad) {
    r <- pdt(s)
    expect_true(r[[1]] %in% c(INVALID, INCOMPLETE), label = s)
    if (r[[1]] == INVALID) expect_identical(r[[2]], 0L, label = s)
  }
  expect_identical(pdt("2024-01-01T10:00:00+1")[[1]], INCOMPLETE)
  expect_identical(pdt("2024-01-01T24:00:00")[[1]], INVALID)
  expect_identical(pdt("2024-01-0")[1:2], list(INCOMPLETE, 9L))
  expect_identical(pdt("2024-01-01T")[[1]], INCOMPLETE)
  expect_identical(pdt("2024-01-01T10:0")[[1]], INCOMPLETE)
  expect_identical(pdt("2024-01-01T10:00:00.")[[1]], INCOMPLETE)
  expect_identical(pdt("2024-01-01T10:00:00+01:")[[1]], INCOMPLETE)
  expect_identical(pdt("")[1:2], list(INVALID, 0L))   # an empty span is not inside a value
  expect_identical(pdt("", TRUE)[1:2], list(INVALID, 0L))
  expect_identical(pdt("x")[[1]], INVALID)
})

test_that("the calendar is exact against itself over four centuries and the int32 range", {
  walk <- function(lo, hi, step) .Call(zufast_test_calendar_walk, lo, hi, step)
  expect_identical(walk(-146097 * 2, 146097 * 2, 1), 0)
  expect_identical(walk(-2^31, 2^31 - 1, 9973), 0)
  expect_identical(walk(-2^31, -2^31 + 1000, 1), 0)
  expect_identical(walk(2^31 - 1001, 2^31 - 1, 1), 0)
  ext <- .Call(zufast_test_civil_from_days, c(-2^31, 2^31 - 1, 0, -1, -719468))
  expect_identical(ext[3, ], c(1970, 1, 1))
  expect_identical(ext[4, ], c(1969, 12, 31))
  expect_identical(ext[5, ], c(0, 3, 1))
  expect_identical(ext[2, ], c(5881580, 7, 11))
  expect_identical(ext[1, ], c(-5877641, 6, 23))
})

test_that("the calendar agrees with R over every day of four centuries", {
  days <- seq(-146097, 146097 - 1)            # 1570-03-01 .. 2369-...
  civil <- .Call(zufast_test_civil_from_days, as.double(days))
  ref <- as.POSIXlt(as.Date(days, origin = "1970-01-01"))
  expect_identical(civil[, 1], ref$year + 1900)
  expect_identical(civil[, 2], ref$mon + 1)
  expect_identical(civil[, 3], as.numeric(ref$mday))
  back <- .Call(zufast_test_days_from_civil, civil[, 1], civil[, 2], civil[, 3])
  expect_identical(back, as.double(days))
})

test_that("leap years and month lengths", {
  info <- function(y) .Call(zufast_test_year_info, y)
  expect_identical(info(2024), c(1L, 0L, 31L, 29L, 31L, 30L, 31L, 30L, 31L, 31L, 30L, 31L, 30L, 31L, 0L))
  expect_identical(info(2023)[c(1, 4)], c(0L, 28L))
  expect_identical(info(1900)[1], 0L)
  expect_identical(info(2000)[1], 1L)
  expect_identical(info(0)[1], 1L)
  expect_identical(info(-4)[1], 1L)
  expect_identical(info(-100)[1], 0L)
  expect_identical(info(-400)[1], 1L)
  expect_identical(info(-1)[1], 0L)
})

test_that("fast_parse_date() and fast_parse_datetime() agree with R on random input", {
  set.seed(20261003)
  days <- sample(seq(as.numeric(as.Date("0001-01-01")), as.numeric(as.Date("9999-12-31"))), 3000)
  d <- as.Date(days, origin = "1970-01-01")
  s <- iso_date(d)
  expect_identical(fast_parse_date(s), d)

  secs <- days * 86400 + sample(0:86399, length(days), replace = TRUE)
  t <- .POSIXct(secs, tz = "UTC")
  txt <- iso_time(t)
  expect_identical(fast_parse_datetime(txt), t)
  expect_identical(fast_parse_datetime(paste0(txt, "Z")), t)

  offs <- sample(seq(-23 * 60 - 59, 23 * 60 + 59), length(days), replace = TRUE)
  sign <- ifelse(offs < 0, "-", "+")
  a <- abs(offs)
  forms <- list(sprintf("%s%02d:%02d", sign, a %/% 60, a %% 60),
                sprintf("%s%02d%02d", sign, a %/% 60, a %% 60))
  for (f in forms) {
    expect_identical(fast_parse_datetime(paste0(txt, f)), .POSIXct(secs - offs * 60, tz = "UTC"))
  }
  whole <- sign(offs) * (abs(offs) %/% 60) * 60
  expect_identical(
    fast_parse_datetime(paste0(txt, sprintf("%s%02d", ifelse(whole < 0, "-", "+"), abs(whole) %/% 60))),
    .POSIXct(secs - whole * 60, tz = "UTC"))
})

test_that("the R parsers require the whole string and keep NA", {
  expect_identical(fast_parse_date(c("2024-01-01", "2024-01-01T00:00", NA, "x")),
                   as.Date(c("2024-01-01", NA, NA, NA)))
  expect_identical(fast_parse_datetime(c("2024-01-01", "2024-01-01 junk", NA)),
                   .POSIXct(c(19723 * 86400, NA, NA), tz = "UTC"))
  expect_equal(as.numeric(fast_parse_datetime("1970-01-01T00:00:00.25Z")), 0.25)
  f <- fast_datetime_fields(c("2024-03-01T12:30:00.5-05:00", "2024-03-01", "bad"))
  expect_s3_class(f, "data.frame")
  expect_identical(f$year, c(2024L, 2024L, NA))
  expect_identical(f$hour, c(12L, NA, NA))
  expect_identical(f$nanosecond, c(500000000L, NA, NA))
  expect_identical(f$offset, c(-18000L, NA, NA))
  expect_identical(f$has_time, c(TRUE, FALSE, NA))
  expect_identical(f$has_offset, c(TRUE, FALSE, NA))
  expect_error(fast_parse_date(1), class = "zufast_invalid_argument")
  expect_identical(nrow(fast_datetime_fields(character())), 0L)
})

test_that("zuf_format_datetime() writes RFC 3339 and round-trips", {
  fmt <- function(f, cap = 64) {
    r <- .Call(zufast_test_format_datetime, f, cap)
    if (r[[1]] <= cap) rawToChar(r[[2]][seq_len(r[[1]])]) else r
  }
  expect_identical(fmt(fields(2024, 2, 29)), "2024-02-29")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, has_time = 1)), "2024-02-29T01:02:03")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, 5e8, 0, 1, 1)), "2024-02-29T01:02:03.500Z")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, 5e5, 0, 1, 1)), "2024-02-29T01:02:03.000500Z")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, 5, 0, 1, 1)), "2024-02-29T01:02:03.000000005Z")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, 0, -5400, 1, 1)), "2024-02-29T01:02:03-01:30")
  expect_identical(fmt(fields(2024, 2, 29, 1, 2, 3, 0, 86340, 1, 1)), "2024-02-29T01:02:03+23:59")
  expect_identical(fmt(fields(9999, 12, 31, 23, 59, 60, 999999999, -86340, 1, 1)),
                   "9999-12-31T23:59:60.999999999-23:59")
  expect_identical(nchar(fmt(fields(9999, 12, 31, 23, 59, 60, 999999999, -86340, 1, 1))), 35L)
  # expanded years have at least six digits, as Temporal writes them
  expect_identical(fmt(fields(10000, 1, 1)), "+010000-01-01")
  expect_identical(fmt(fields(-1, 1, 1)), "-000001-01-01")
  expect_identical(fmt(fields(0, 1, 1)), "0000-01-01")
  expect_identical(fmt(fields(123456, 1, 1)), "+123456-01-01")
  expect_identical(fmt(fields(-99999, 1, 1)), "-099999-01-01")
  expect_identical(fmt(fields(275760, 9, 13)), "+275760-09-13")
  expect_identical(fmt(fields(-271821, 4, 20)), "-271821-04-20")
  expect_identical(fmt(fields(-100000, 1, 1)), "-100000-01-01")
  # ZUF_DATETIME_MAX_CHARS (42) covers any int32_t year
  widest <- fields(-2147483648, 12, 31, 23, 59, 60, 999999999, -86340, 1, 1)
  expect_identical(fmt(widest), "-2147483648-12-31T23:59:60.999999999-23:59")
  expect_identical(.Call(zufast_test_format_datetime, widest, 0)[[1]], 42)
  expect_identical(fmt(fields(2147483647, 1, 1)), "+2147483647-01-01")
  # out-of-range fields never write out of bounds
  expect_identical(fmt(fields(2024, 250, 250, 250, 250, 250, 4e9, 0, 1, 1)),
                   "2024-50-50T50:50:50Z")

  set.seed(5)
  for (i in 1:300) {
    f <- fields(sample(0:9999, 1), sample(1:12, 1), sample(1:28, 1), sample(0:23, 1),
                sample(0:59, 1), sample(0:60, 1),
                sample(c(0, 1e6 * sample(1:999, 1), 1e3 * sample(1:999999, 1), sample(1:999999999, 1)), 1),
                60 * sample(-1439:1439, 1), 1, sample(0:1, 1))
    if (f[10] == 0) f[8] <- 0
    s <- fmt(f)
    expect_identical(pdt(s)[[3]], f, label = s)
  }
})

test_that("the formatters measure and write nothing when short", {
  f <- fields(2024, 2, 29, 1, 2, 3, 5e8, 3600, 1, 1)
  r <- .Call(zufast_test_format_datetime, f, 0)
  expect_identical(r[[1]], 29)
  for (cap in c(0, 10, 28)) {
    r <- .Call(zufast_test_format_datetime, f, cap)
    expect_identical(r[[1]], 29)
    expect_true(all(r[[2]] == SENT))
  }
  r <- .Call(zufast_test_format_datetime, f, 29)
  expect_identical(rawToChar(r[[2]][1:29]), "2024-02-29T01:02:03.500+01:00")
  expect_true(all(r[[2]][-(1:29)] == SENT))
  r <- .Call(zufast_test_format_date, 0, 9)
  expect_identical(r[[1]], 10)
  expect_true(all(r[[2]] == SENT))
  r <- .Call(zufast_test_format_date, 0, 10)
  expect_identical(rawToChar(r[[2]][1:10]), "1970-01-01")
})

test_that("fast_format_datetime() writes UTC RFC 3339", {
  t <- .POSIXct(c(0, 0.25, 1709296200.5, -1, NA, Inf, 1.5e-6), tz = "UTC")
  expect_identical(fast_format_datetime(t),
                   c("1970-01-01T00:00:00Z", "1970-01-01T00:00:00.250Z",
                     "2024-03-01T12:30:00.500Z", "1969-12-31T23:59:59Z", NA, NA,
                     "1970-01-01T00:00:00.000002Z"))
  expect_identical(fast_format_datetime(t[1:4], digits = 0),   # rounded to nearest
                   c("1970-01-01T00:00:00Z", "1970-01-01T00:00:00Z",
                     "2024-03-01T12:30:01Z", "1969-12-31T23:59:59Z"))
  expect_identical(fast_format_datetime(.POSIXct(0.9996, tz = "UTC"), digits = 3),
                   "1970-01-01T00:00:01.000Z")
  expect_identical(fast_format_datetime(.POSIXct(0.25, tz = "UTC"), digits = 9),
                   "1970-01-01T00:00:00.250000000Z")
  expect_identical(fast_format_datetime(as.Date(c("2024-03-01", NA))), c("2024-03-01", NA))
  # finite but beyond the int32_t day range of the calendar: NA, as documented
  expect_identical(fast_format_datetime(.POSIXct(c(1.8e14, 1.9e14), tz = "UTC")),
                   c("+5705942-12-07T08:00:00Z", NA))
  # origin: as.Date.numeric() has no default origin before R 4.3, and DESCRIPTION
  # declares R >= 4.1.
  expect_identical(fast_format_datetime(as.Date(c(2^31 - 1, 2^31), origin = "1970-01-01")),
                   c("+5881580-07-11", NA))

  set.seed(6)
  secs <- runif(500, -6e10, 2.5e11)
  t <- .POSIXct(round(secs), tz = "UTC")
  expect_identical(fast_parse_datetime(fast_format_datetime(t)), t)
  expect_identical(fast_format_datetime(t, digits = 0), paste0(iso_time(t), "Z"))

  expect_error(fast_format_datetime("2024"), class = "zufast_invalid_argument")
  expect_error(fast_format_datetime(Sys.time(), digits = 10), class = "zufast_invalid_argument")
  expect_error(fast_format_datetime(Sys.time(), digits = 1.5), class = "zufast_invalid_argument")
})

test_that("ISO week dates agree with strftime() on every day from 1900 to 2100", {
  skip_if(format(as.Date("2021-01-03"), "%G-W%V-%u") != "2020-W53-7",
          "this platform's strftime() lacks %G, %V or %u")
  d <- seq(as.Date("1900-01-01"), as.Date("2100-12-31"), by = "day")
  days <- as.double(unclass(d))
  wk <- .Call(zufast_test_iso_week, days)
  expect_identical(sprintf("%04d-W%02d-%d", wk[, 1], wk[, 2], wk[, 3]),
                   format(d, "%G-W%V-%u"))
  expect_identical(.Call(zufast_test_days_from_iso_week, wk[, 1], wk[, 2], wk[, 3]), days)

  # weeks per week year, from the reference; 1900 and 2100 are whole week
  # years inside the span only by luck, so the comparison leaves them out
  ref <- tapply(as.integer(format(d, "%V")), format(d, "%G"), max)
  years <- 1901:2099
  expect_identical(.Call(zufast_test_iso_weeks_in_year, as.double(years)),
                   as.double(ref[as.character(years)]), ignore_attr = TRUE)
})

test_that("ISO weeks are consistent over four centuries and at the int32 ends", {
  walk <- function(lo, hi) .Call(zufast_test_iso_week_walk, lo, hi)
  expect_identical(walk(-146097, 146097), 0)
  expect_identical(walk(-2^31, -2^31 + 6), 0)
  expect_identical(walk(2^31 - 7, 2^31 - 1), 0)
  wk <- .Call(zufast_test_iso_week, c(0, 1, -1, -2^31, 2^31 - 1))
  expect_identical(wk[1, ], c(1970, 1, 4))   # 1970-01-01, a Thursday
  expect_identical(wk[2, 3], 5)
  expect_identical(wk[3, 3], 3)
  expect_identical(wk[4, 1], -5877641)
  expect_identical(wk[5, 1], 5881580)

  in_year <- function(y) .Call(zufast_test_iso_weeks_in_year, as.double(y))
  expect_identical(in_year(c(2004, 2009, 2015, 2020, 2026)), rep(53, 5))
  expect_identical(in_year(c(2021, 2024, 2025)), rep(52, 3))
  expect_true(all(in_year(c(-5877641, 0, 5881580)) %in% c(52, 53)))
  # 2020-W53-7 is 2021-01-03; 2026-W01-1 is 2025-12-29
  expect_identical(.Call(zufast_test_days_from_iso_week, c(2020, 2026), c(53, 1), c(7, 1)),
                   as.double(unclass(as.Date(c("2021-01-03", "2025-12-29")))))
})

test_that("ZUF_DT_EXPANDED_YEAR reads a sign and exactly six year digits", {
  expect_identical(pdt("+010000-01-01", flags = EXPANDED)[1:3], list(OK, 13L, fields(10000, 1, 1)))
  expect_identical(pdt("-000001-01-01", flags = EXPANDED)[1:3], list(OK, 13L, fields(-1, 1, 1)))
  expect_identical(pdt("+002024-02-29", flags = EXPANDED)[1:3], list(OK, 13L, fields(2024, 2, 29)))
  expect_identical(pdt("+275760-09-13T00:00:00Z", flags = EXPANDED)[1:3],
                   list(OK, 23L, fields(275760, 9, 13, 0, 0, 0, 0, 0, 1, 1)))
  expect_identical(pdt("-271821-04-20T00:00", flags = EXPANDED)[[3]][1:3], c(-271821, 4, 20))
  expect_identical(pdt("-271821-04-20", TRUE, EXPANDED)[1:2], list(OK, 13L))
  expect_identical(pdt("+000000-01-01", flags = EXPANDED)[[3]][1], 0)
  # the four-digit form still parses with the flag
  expect_identical(pdt("2024-02-29", flags = EXPANDED)[1:3], list(OK, 10L, fields(2024, 2, 29)))
  # -100000-02-29 is a leap day: -100000 is divisible by 400
  expect_identical(pdt("-100000-02-29", flags = EXPANDED)[[1]], OK)
  expect_identical(pdt("-099999-02-29", flags = EXPANDED)[[1]], INVALID)

  invalid <- c("-000000-01-01", "+2024-01-01", "+02024-01-01", "+0002024-01-01",
               "+1000000-01-01", "+00202x-01-01", "+010000-13-01", "+010000-02-30",
               "+010000/01/01", "++10000-01-01", "+-10000-01-01")
  for (s in invalid) expect_identical(pdt(s, flags = EXPANDED)[1:2], list(INVALID, 0L), label = s)
  for (s in c("+", "-", "+01000", "+010000-01", "+010000-01-0")) {
    expect_identical(pdt(s, flags = EXPANDED)[1:2], list(INCOMPLETE, nchar(s)), label = s)
  }
  # without the flag every one of these, and a valid expanded date, is invalid
  for (s in c(invalid, "+01000", "+010000-01", "+010000-01-01", "-000001-01-01")) {
    expect_identical(pdt(s)[1:2], list(INVALID, 0L), label = s)
    expect_identical(pdt(s, TRUE)[1:2], list(INVALID, 0L), label = s)
  }
  # the default grammar's rejections stand with the flag on
  bad <- c("2023-02-29", "2024-13-01", "24-01-01", "2024/01/01", "20240101", "2024-W01-1",
           "2024-001", "2024-01-01T24:00:00", "2024-01-01T23:60", "2024-01-01T10",
           "2024-01-01T10:00:00.Z", "2024-01-01T10:00:00+24:00", " 2024-01-01", "")
  for (s in bad) {
    r <- pdt(s, flags = EXPANDED)
    expect_true(r[[1]] %in% c(INVALID, INCOMPLETE), label = s)
    if (r[[1]] == INVALID) expect_identical(r[[2]], 0L, label = s)
  }
})

test_that("expanded years round-trip through the formatters over Temporal's range", {
  expect_identical(.Call(zufast_test_expanded_roundtrip, -271821, 275760, 200000), 0)
  # and over the whole six-digit range
  expect_identical(.Call(zufast_test_expanded_roundtrip, -999999, 999999, 0), 0)
})
