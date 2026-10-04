# Reference conversions written in R (design 21.4).




test_that("binary16 decodes exactly for all 65536 patterns", {
  bits <- 0:65535
  got <- .Call(zufast_test_half_decode, bits, 0L)
  want <- f16_value(bits)
  expect_identical(is.nan(got), is.nan(want))
  ok <- !is.nan(want)
  expect_identical(got[ok], want[ok])
  # signed zero
  expect_identical(1 / got[0x8001L], -Inf)
})

test_that("bfloat16 decodes exactly for all 65536 patterns", {
  bits <- 0:65535
  got <- .Call(zufast_test_half_decode, bits, 1L)
  want <- bf16_value(bits)
  expect_identical(is.nan(got), is.nan(want))
  ok <- !is.nan(want)
  expect_identical(got[ok], want[ok])
})

test_that("binary16 encoding round-trips every pattern and rounds ties to even", {
  bits <- 0:65535
  finite <- bits[bitwAnd(bitwShiftR(bits, 10L), 0x1FL) != 31L]
  vals <- f16_value(finite)
  expect_identical(.Call(zufast_test_half_encode, vals, 0L), finite)

  pos <- f16_value(0:0x7BFF)            # every finite non-negative half, ascending
  mids <- (pos[-1] + pos[-length(pos)]) / 2   # exact in binary32
  # perturb by a quarter of the gap, which stays float-exact
  gaps <- diff(pos)
  probes <- c(mids, mids - gaps / 4, mids + gaps / 4, 65504 + 8, 65504 + 15.99,
              65520, 1e6, 2^-25, 2^-26, 2^-25 * 1.5)
  want <- round_to_table(probes, pos, 0x7BFFL, 0x7C00L)
  want[probes > 65504 & probes < 65520] <- 0x7BFFL
  want[probes >= 65520] <- 0x7C00L
  expect_identical(.Call(zufast_test_half_encode, probes, 0L), want)
  expect_identical(.Call(zufast_test_half_encode, -probes, 0L), want + 0x8000L)

  specials <- c(Inf, -Inf, 0, -0)
  expect_identical(.Call(zufast_test_half_encode, specials, 0L),
                   c(0x7C00L, 0xFC00L, 0L, 0x8000L))
  nan <- .Call(zufast_test_half_encode, NaN, 0L)
  expect_identical(bitwAnd(nan, 0x7E00L), 0x7E00L)
})

test_that("binary16 encoding agrees with a float-level reference at the float ulp", {
  # Values one binary32 ulp either side of each midpoint, given as float bits.
  pos <- f16_value(0:0x7BFF)
  mids <- (pos[-1] + pos[-length(pos)]) / 2
  normal <- mids >= 2^-14
  e <- floor(log2(mids[normal]))
  ulp <- 2^(e - 23)
  probes <- c(mids[normal] - ulp, mids[normal] + ulp)
  want <- round_to_table(probes, pos, 0x7BFFL, 0x7C00L)
  expect_identical(.Call(zufast_test_half_encode, probes, 0L), want)
})

test_that("bfloat16 encoding rounds to nearest even", {
  bits <- 0:65535
  finite <- bits[bitwAnd(bitwShiftR(bits, 7L), 0xFFL) != 255L]
  expect_identical(.Call(zufast_test_half_encode, bf16_value(finite), 1L), finite)

  pos <- bf16_value(0:0x7F7F)
  mids <- (pos[-1] + pos[-length(pos)]) / 2
  sel <- seq(1, length(mids), by = 37)
  probes <- mids[sel]
  want <- round_to_table(probes, pos, 0x7F7FL, 0x7F80L)
  expect_identical(.Call(zufast_test_half_encode, probes, 1L), want)
  # beyond FLT_MAX's bfloat16 neighbourhood rounds to infinity
  expect_identical(.Call(zufast_test_half_encode_bits, 0x7F7FFFFF, 1L), 0x7F80L)
  nan <- .Call(zufast_test_half_encode_bits, 0x7F800001, 1L)
  expect_true(bitwAnd(nan, 0x7F80L) == 0x7F80L && bitwAnd(nan, 0x7FL) != 0L)
})

test_that("the fits predicates are exact round trips", {
  fits <- function(x, w) .Call(zufast_test_fits, x, w)
  expect_identical(fits(c(1, 0.5, 65504, -0, Inf, NaN, 2^-24), 16L), rep(TRUE, 7))
  expect_identical(fits(c(0.1, 65505, 1 + 2^-11, 2^-25, 1e10), 16L), rep(FALSE, 5))
  expect_identical(fits(c(1, 0.5, Inf, -Inf, -0, 2^-149, NaN), 32L), rep(TRUE, 7))
  expect_identical(fits(c(0.1, 1e39, -1e300, 2^-150, 1 + 2^-30), 32L), rep(FALSE, 5))
  # NaN payloads: the default NaN survives, NA_real_ (payload 1954) does not
  expect_identical(fits(c(NaN, NA_real_), 32L), c(TRUE, FALSE))
})

test_that("loads, stores and swaps agree for every width, order and offset", {
  raw <- as.raw(c(0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xff, 0x00))
  r0 <- .Call(zufast_test_endian, raw, 0L)
  expect_identical(r0, c(
    "0000000000002301", "0000000000000123",
    "0000000067452301", "0000000001234567",
    "efcdab8967452301", "0123456789abcdef", "ok"))
  for (off in 0:2) {
    expect_identical(.Call(zufast_test_endian, raw, off)[7], "ok")
  }
})
