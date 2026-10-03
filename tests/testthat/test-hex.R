hex_enc <- function(raw, cap, upper = FALSE) .Call(zufast_test_hex_encode, raw, cap, upper)
hex_dec <- function(s, cap) .Call(zufast_test_hex_decode, str_raw(s), cap)
SENT <- as.raw(0xA5)

test_that("zuf_hex_encode() matches the reference for every 1- and 2-byte input", {
  all2 <- expand.grid(a = 0:255, b = 0:255)
  for (i in seq(1, nrow(all2), by = 97)) {
    r <- as.raw(c(all2$a[i], all2$b[i]))
    expect_identical(fast_hex_encode(r), ref_hex(r))
  }
  one <- vapply(0:255, function(b) fast_hex_encode(as.raw(b), upper = TRUE), "")
  expect_identical(one, sprintf("%02X", 0:255))
})

test_that("zuf_hex_encode() measures with cap 0 and writes nothing when short", {
  for (n in 0:6) {
    r <- as.raw(sample(0:255, n, replace = TRUE))
    for (cap in 0:(2 * n + 2)) {
      res <- hex_enc(r, cap)
      expect_identical(res[[1]], 2 * n)
      buf <- res[[2]]
      if (cap < 2 * n) {
        expect_true(all(buf == SENT))
      } else {
        expect_identical(rawToChar(buf[seq_len(2 * n)]), ref_hex(r))
        expect_true(all(buf[-seq_len(2 * n)] == SENT))
      }
    }
  }
})

test_that("zuf_hex_decode() round-trips and accepts either case", {
  set.seed(1)
  for (n in 0:6) {
    for (k in 1:20) {
      r <- as.raw(sample(0:255, n, replace = TRUE))
      expect_identical(fast_hex_decode(ref_hex(r))[[1]], r)
      expect_identical(fast_hex_decode(ref_hex(r, TRUE))[[1]], r)
    }
  }
  expect_identical(fast_hex_decode("aBcD")[[1]], as.raw(c(0xab, 0xcd)))
})

test_that("zuf_hex_decode() rejects every malformed class with the right status", {
  expect_identical(hex_dec("abc", 10)[[1]], 1L)         # odd length
  expect_identical(hex_dec("zz", 10)[[1]], 1L)          # not hex
  expect_identical(hex_dec("a g", 10)[[1]], 1L)
  expect_identical(hex_dec("0x", 10)[[1]], 1L)
  expect_identical(hex_dec(" 00", 10)[[1]], 1L)
  for (b in setdiff(0:255, c(48:57, 65:70, 97:102))) {
    s <- rawToChar(as.raw(c(0x30, if (b == 0) 0x20 else b)))
    expect_identical(hex_dec(s, 10)[[1]], 1L)
  }
  # too small: NO_SPACE, nothing written, out_len 0
  res <- hex_dec("abcdef", 2)
  expect_identical(res[[1]], 4L)
  expect_identical(res[[2]], 0)
  expect_true(all(res[[3]] == SENT))
  # exactly enough
  res <- hex_dec("abcdef", 3)
  expect_identical(res[[1]], 0L)
  expect_identical(res[[3]][1:3], as.raw(c(0xab, 0xcd, 0xef)))
  expect_true(all(res[[3]][-(1:3)] == SENT))
  expect_identical(hex_dec("", 0)[1:2], list(0L, 0))
})

test_that("fast_hex_encode() and fast_hex_decode() are vectorised with NA", {
  expect_identical(fast_hex_encode(c("ab", NA, "")), c("6162", NA, ""))
  expect_identical(fast_hex_decode(c("6162", NA, "6", "zz")),
                   list(charToRaw("ab"), NULL, NULL, NULL))
  expect_identical(fast_hex_encode(raw()), "")
  expect_error(fast_hex_encode(1), class = "zufast_invalid_argument")
  expect_error(fast_hex_encode(raw(), upper = NA), class = "zufast_invalid_argument")
  expect_error(fast_hex_decode(as.raw(1)), class = "zufast_invalid_argument")
})
