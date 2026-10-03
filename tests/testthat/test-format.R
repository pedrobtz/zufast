SCI <- 1L; TRAIL <- 2L
fmt <- function(x, flags = 0L, kind = 0L, cap = 64) {
  r <- .Call(zufast_test_format_shortest, x, flags, kind, cap)
  if (r[[1]] <= cap) rawToChar(r[[2]][seq_len(r[[1]])]) else r
}

test_that("ECMAScript notation, design 9.1", {
  expect_identical(fast_format_double(c(1, 0.1, 0.1 + 0.2, 1e21, 1e-7, 1e-6, 1e20, 123.456)),
                   c("1", "0.1", "0.30000000000000004", "1e+21", "1e-7", "0.000001",
                     "100000000000000000000", "123.456"))
  expect_identical(fast_format_double(c(123456789012345680000, 1.5e300, 5e-324,
                                        .Machine$double.xmax, 2^53, 1 / 3)),
                   c("123456789012345680000", "1.5e+300", "5e-324",
                     "1.7976931348623157e+308", "9007199254740992", "0.3333333333333333"))
  expect_identical(fast_format_double(c(-0, 0, NaN, Inf, -Inf, NA, -1.5)),
                   c("-0", "0", "NaN", "Inf", "-Inf", NA, "-1.5"))
  expect_identical(fast_format_double(c(1, 0.1, 123.456, 1e21, -0), scientific = TRUE),
                   c("1e+0", "1e-1", "1.23456e+2", "1e+21", "-0e+0"))
  expect_identical(fast_format_double(c(1, 0.5, 1e20, 1e21, -0), trailing_zero = TRUE),
                   c("1.0", "0.5", "100000000000000000000.0", "1e+21", "-0.0"))
})

test_that("shortest digits agree with an independent algorithm (Python repr)", {
  fx <- utils::read.delim(test_path("fixtures", "shortest-f64.tsv"), colClasses = "character")
  got <- .Call(zufast_test_decimal_bits, fx$bits)
  expect_identical(got, paste(fx$negative, fx$mantissa, fx$exponent))
})

test_that("format then parse is the identity, bit for bit, over every exponent", {
  set.seed(11)
  bits_to_double <- function(n) {
    r <- matrix(as.raw(sample(0:255, 8 * n, replace = TRUE)), nrow = 8)
    readBin(as.vector(r), "double", n = n)
  }
  x <- bits_to_double(50000)
  x <- x[is.finite(x)]
  x <- c(x, 2^(-1074:1023), -2^(-1074:1023), (2^(-1074:1023)) * (1 + 2^-52),
         .Machine$double.xmax, .Machine$double.xmin, 0.1, 0.2, 0.3)
  for (flags in list(c(FALSE, FALSE), c(TRUE, FALSE), c(FALSE, TRUE))) {
    s <- fast_format_double(x, flags[1], flags[2])
    back <- fast_parse_double(s)
    expect_identical(back, x)
    expect_lte(max(nchar(s)), 25L)
  }
  # and the digits are the fewest: dropping one significant digit never
  # round-trips
  s <- fast_format_double(x[1:3000], scientific = TRUE)
  ndig <- nchar(sub("e.*", "", gsub("[-.]", "", s)))
  shorter <- sprintf("%.*e", pmax(ndig - 2L, 0L), x[1:3000])
  expect_false(any(fast_parse_double(shorter[ndig > 1]) == x[1:3000][ndig > 1]))
})

test_that("the longest outputs fit the MAX_CHARS capacities", {
  expect_identical(nchar(fmt(-0.000001234567890123456)), 24L)
  expect_identical(nchar(fmt(-1.2345678901234567e-6)), 25L)
  expect_identical(nchar(fmt(-1.7976931348623157e308)), 24L)
  expect_identical(nchar(fmt(-9.87654321e20, TRAIL)), 24L)
  expect_identical(nchar(fmt(-9.87654321e20, TRAIL, kind = 1L)), 24L)
  expect_identical(fmt(-1.17549435e-38, 0L, kind = 1L), "-1.1754944e-38")
})

test_that("float formatting is shortest for float and round-trips", {
  f32 <- function(x, flags = 0L) .Call(zufast_test_format_f32_vec, x, flags)
  expect_identical(f32(c(0.1, 1, 3.4028234663852886e38, 1.401298464324817e-45, -0, NaN, -Inf)),
                   c("0.1", "1", "3.4028235e+38", "1e-45", "-0", "NaN", "-Inf"))
  set.seed(12)
  x <- c(runif(3000, -1, 1) * 10^sample(-45:38, 3000, replace = TRUE))
  s <- f32(x)
  back <- vapply(s, function(z) .Call(zufast_test_parse_num, charToRaw(z), 1L, 0L, 10L, 0L)[[3]], 0)
  want <- .Call(zufast_test_to_f32, x)
  expect_identical(unname(back), want)
})

test_that("the formatter measures and writes nothing when short", {
  r <- .Call(zufast_test_format_shortest, 0.30000000000000004, 0L, 0L, 0)
  expect_identical(r[[1]], 19)
  for (cap in c(0, 5, 18)) {
    r <- .Call(zufast_test_format_shortest, 0.30000000000000004, 0L, 0L, cap)
    expect_true(all(r[[2]] == as.raw(0xA5)))
  }
  r <- .Call(zufast_test_format_shortest, 0.30000000000000004, 0L, 0L, 19)
  expect_identical(rawToChar(r[[2]][1:19]), "0.30000000000000004")
  expect_true(all(r[[2]][-(1:19)] == as.raw(0xA5)))
})

test_that("fast_format_double() checks its arguments", {
  expect_error(fast_format_double(1L), class = "zufast_invalid_argument")
  expect_error(fast_format_double(1, scientific = NA), class = "zufast_invalid_argument")
  expect_identical(fast_format_double(double()), character())
})
