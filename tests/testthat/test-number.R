test_that("the default float grammar and from_chars ptr semantics", {
  expect_identical(pn("1.5"), list(OK, 3L, 1.5))
  expect_identical(pn("-2.5e-3xyz"), list(OK, 7L, -2.5e-3))
  expect_identical(pn("1."), list(OK, 2L, 1))
  expect_identical(pn(".5"), list(OK, 2L, 0.5))
  expect_identical(pn("1,5"), list(OK, 1L, 1))
  expect_identical(pn("0x10"), list(OK, 1L, 0))
  expect_identical(pn("inf"), list(OK, 3L, Inf))
  expect_identical(pn("-INFINITY"), list(OK, 9L, -Inf))
  expect_true(is.nan(pn("NaN")[[3]]))
  expect_identical(1 / pn("-0")[[3]], -Inf)
  for (s in c("+1", " 1", "", "-", ".", "e5", "abc", "--1")) {
    expect_identical(pn(s)[1:2], list(INVALID, 0L), label = s)
  }
})

test_that("out-of-range floats report RANGE and still write the value", {
  expect_identical(pn("1e400"), list(RANGE, 5L, Inf))
  expect_identical(pn("-1e400"), list(RANGE, 6L, -Inf))
  expect_identical(pn("1e-400"), list(RANGE, 6L, 0))
  expect_identical(1 / pn("-1e-400")[[3]], -Inf)
  expect_identical(pn("0e9999"), list(OK, 6L, 0))
  expect_identical(pn("3.5e38", 1L), list(RANGE, 6L, Inf))
})

test_that("options: JSON, leading plus, whitespace, decimal point", {
  expect_identical(pn("+1", flags = PLUS), list(OK, 2L, 1))
  expect_identical(pn(" \t1", flags = SPACE), list(OK, 3L, 1))
  expect_identical(pn("1,5", dp = utf8ToInt(",")), list(OK, 3L, 1.5))
  for (s in c("01", "1.", ".5", "+1", "inf", "NaN", "-")) {
    expect_identical(pn(s, flags = JSON)[[1]], INVALID, label = s)
  }
  expect_identical(pn("-0.5e+10", flags = JSON), list(OK, 8L, -0.5e10))
})

test_that("integers: every type, bases, saturation on overflow", {
  expect_identical(pn("9223372036854775807", 2L), list(OK, 19L, "9223372036854775807"))
  expect_identical(pn("9223372036854775808", 2L), list(RANGE, 19L, "9223372036854775807"))
  expect_identical(pn("-9223372036854775808", 2L), list(OK, 20L, "-9223372036854775808"))
  expect_identical(pn("-9223372036854775809", 2L), list(RANGE, 20L, "-9223372036854775808"))
  expect_identical(pn("18446744073709551615", 3L), list(OK, 20L, "18446744073709551615"))
  expect_identical(pn("18446744073709551616", 3L), list(RANGE, 20L, "18446744073709551615"))
  expect_identical(pn("-1", 3L)[1:2], list(INVALID, 0L))
  expect_identical(pn("2147483648", 4L), list(RANGE, 10L, "2147483647"))
  expect_identical(pn("-2147483649", 4L), list(RANGE, 11L, "-2147483648"))
  expect_identical(pn("4294967295", 5L), list(OK, 10L, "4294967295"))
  expect_identical(pn("4294967296", 5L), list(RANGE, 10L, "4294967295"))
  expect_identical(pn("12.5", 2L), list(OK, 2L, "12"))
  expect_identical(pn("+5", 2L)[[1]], INVALID)
  expect_identical(pn("+5", 2L, PLUS), list(OK, 2L, "5"))
  expect_identical(pn("  -5", 2L, SPACE), list(OK, 4L, "-5"))
  expect_identical(pn("  -99999999999999999999", 2L, SPACE)[c(1, 3)], list(RANGE, "-9223372036854775808"))
  # bases
  expect_identical(pn("ff", 2L, base = 16L)[[3]], "255")
  expect_identical(pn("FF", 3L, base = 16L)[[3]], "255")
  expect_identical(pn("777", 4L, base = 8L)[[3]], "511")
  expect_identical(pn("-zz", 2L, base = 36L)[[3]], "-1295")
  expect_identical(pn("102", 2L, base = 2L), list(OK, 2L, "2"))
  expect_identical(pn("12", 2L, base = 0L)[[3]], "12")
  for (b in c(1L, 37L, -1L)) expect_identical(pn("1", 2L, base = b)[[1]], INVALID)
  set.seed(7)
  for (b in 2:36) {
    v <- sample(.Machine$integer.max, 50)
    digits <- c(0:9, letters)[1:b]
    s <- vapply(v, function(n) {
      out <- character()
      while (n > 0) { out <- c(digits[n %% b + 1], out); n <- n %/% b }
      paste(out, collapse = "")
    }, "")
    got <- vapply(s, function(x) pn(x, 4L, base = b)[[3]], "", USE.NAMES = FALSE)
    expect_identical(got, as.character(v))
    expect_identical(strtoi(s, base = b), v)
  }
})

test_that("integer writers are exact at every type boundary", {
  expect_identical(.Call(zufast_test_write_ints), 0)
  x <- c(0L, 1L, -1L, 9L, 10L, 99L, 100L, .Machine$integer.max, -.Machine$integer.max,
         sample(-1e9:1e9, 1000))
  expect_identical(.Call(zufast_test_write_i32, x), as.character(x))
})

test_that("float parsing agrees with strtod and strtof (correct rounding)", {
  set.seed(8)
  rand_decimal <- function(n) {
    nd <- sample(1:40, n, replace = TRUE)
    mant <- vapply(nd, function(k) paste(sample(0:9, k, replace = TRUE), collapse = ""), "")
    point <- vapply(seq_len(n), function(i) sample(0:nd[i], 1), 0L)
    mant <- ifelse(point > 0 & point < nd,
                   paste0(substr(mant, 1, point), ".", substring(mant, point + 1)), mant)
    paste0(ifelse(runif(n) < 0.5, "-", ""), mant, "e", sample(-340:340, n, replace = TRUE))
  }
  x <- c(rand_decimal(20000),
         # halfway and near-halfway cases, subnormals, extremes
         "9007199254740993", "9007199254740992.5", "2.2250738585072011e-308",
         "2.2250738585072012e-308", "4.9406564584124654e-324", "2.4703282292062327e-324",
         "2.4703282292062328e-324", "1.7976931348623157e308", "1.7976931348623158e308",
         "1.7976931348623159e308", "0.1", "0.2", "0.3", "1e23", "8.98846567431158e307",
         "7.038531e-26", "1.00000005960464477539062499", "1.00000005960464477539062500",
         "1.00000005960464477539062501", "3.4028235677973366e38", "1.4e-45", "7e-46",
         paste0("0.", strrep("1", 800)), paste0(strrep("9", 800), "e-800"),
         paste0("1", strrep("0", 400), "e-400"), "0e9999", "-0")
  m <- .Call(zufast_test_parse_vs_strtod, x)
  ok <- !is.na(m[, 2])
  expect_true(all(ok))
  expect_identical(m[, 1], m[, 2])
  expect_identical(m[, 3], m[, 4])
})

test_that("disagreements with as.numeric() are classified, not hidden", {
  set.seed(9)
  x <- sprintf("%.*f", sample(1:8, 20000, replace = TRUE), runif(20000, 0, 1000))
  ours <- fast_parse_double(x)
  base <- as.numeric(x)
  exact <- .Call(zufast_test_parse_vs_strtod, x)[, 2]
  expect_identical(ours, exact)
  differ <- ours != base
  # every disagreement is R's: one unit in the last place
  ulp <- 2^(floor(log2(abs(ours[differ]))) - 52)
  expect_true(all(abs(ours[differ] - base[differ]) <= ulp))
  expect_lt(mean(differ), 0.01)
  # zucsv's example: R's parser (also for the literal) is one ulp off
  expect_identical(fast_parse_double("0.799012"),
                   .Call(zufast_test_parse_vs_strtod, "0.799012")[, 2])
})

test_that("fast_parse_double() and fast_parse_integer() follow the R-facing grammar", {
  expect_identical(fast_parse_double(c("1.5", " -2e-3 ", "+inf", "0x10", "abc", NA, "", "1e999", "NaN")),
                   c(1.5, -2e-3, Inf, NA, NA, NA, NA, Inf, NaN))
  expect_identical(fast_parse_integer(c("42", " +7 ", "2147483647", "-2147483647", "2147483648",
                                        "-2147483648", "1.0", "1e3", "", NA, "0x1")),
                   c(42L, 7L, .Machine$integer.max, -.Machine$integer.max, NA, NA, NA, NA, NA, NA, NA))
  expect_error(fast_parse_double(1), class = "zufast_invalid_argument")
  expect_error(fast_parse_integer(list()), class = "zufast_invalid_argument")
})

test_that("fixed formatting matches sprintf() on glibc and follows the formatter convention", {
  fx <- function(x, p) .Call(zufast_test_format_fixed_vec, x, p)
  expect_identical(fx(c(0.125, 0.375, 2.5, -0.0, 1e22, 5e-324), 2L),
                   c("0.12", "0.38", "2.50", "-0.00", "10000000000000000000000.00", "0.00"))
  expect_identical(fx(c(NaN, Inf, -Inf, NA), 3L), c("NaN", "Inf", "-Inf", "NaN"))
  expect_identical(fx(0.1, -1L), "0.100000")
  expect_identical(fx(.Machine$double.xmax, 0L), sprintf("%.0f", .Machine$double.xmax))
  expect_identical(nchar(fx(-.Machine$double.xmax, 2L)), 311L + 2L)   # the 311 + places bound
  r <- .Call(zufast_test_format_fixed, 3.25, 1L, 0)
  expect_identical(r[[1]], 3)
  expect_true(all(r[[2]] == as.raw(0xA5)))
  r <- .Call(zufast_test_format_fixed, 3.25, 1L, 3)
  expect_identical(rawToChar(r[[2]][1:3]), "3.2")
  expect_true(all(r[[2]][-(1:3)] == as.raw(0xA5)))

  skip_if_not(Sys.info()[["sysname"]] == "Linux", "only glibc prints exact binary values")
  set.seed(10)
  x <- c(runif(2000, -1e6, 1e6), 10^runif(500, -320, 308), -10^runif(500, -30, 30),
         (0:2000) / 8, 2^(-1074:-1000))
  for (p in c(0L, 1L, 3L, 6L, 17L, 30L)) {
    expect_identical(fx(x, p), sprintf("%.*f", p, x), label = paste("places", p))
  }
})
