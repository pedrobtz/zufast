URL <- 1L
NO_PAD <- 2L
b64_enc <- function(raw, cap, flags = 0L) .Call(zufast_test_base64_encode, raw, cap, flags)
b64_dec <- function(s, cap = 100, flags = 0L) .Call(zufast_test_base64_decode, str_raw(s), cap, flags)
SENT <- as.raw(0xA5)

test_that("RFC 4648 test vectors", {
  v <- c("", "f", "fo", "foo", "foob", "fooba", "foobar")
  e <- c("", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy")
  expect_identical(fast_base64_encode(v), e)
  expect_identical(fast_base64_encode(v, pad = FALSE), sub("=+$", "", e))
  expect_identical(lapply(fast_base64_decode(e), rawToChar), as.list(v))
})

test_that("encoding matches the reference for every length up to 6", {
  set.seed(2)
  for (n in 0:6) {
    for (k in 1:40) {
      r <- as.raw(sample(0:255, n, replace = TRUE))
      for (url in c(FALSE, TRUE)) for (pad in c(TRUE, FALSE)) {
        expect_identical(fast_base64_encode(r, url = url, pad = pad),
                         ref_base64(r, url = url, pad = pad))
      }
    }
  }
  one <- vapply(0:255, function(b) fast_base64_encode(as.raw(b)), "")
  expect_identical(one, vapply(0:255, function(b) ref_base64(as.raw(b)), ""))
})

test_that("decoding round-trips every length up to 6, padded or not", {
  set.seed(3)
  for (n in 0:6) {
    for (k in 1:40) {
      r <- as.raw(sample(0:255, n, replace = TRUE))
      for (url in c(FALSE, TRUE)) {
        expect_identical(fast_base64_decode(ref_base64(r, url), url = url)[[1]], r)
        expect_identical(fast_base64_decode(ref_base64(r, url, FALSE), url = url)[[1]], r)
      }
    }
  }
})

test_that("the encoder measures with cap 0 and writes nothing when short", {
  for (n in 0:7) {
    r <- as.raw(seq_len(n))
    for (flags in c(0L, NO_PAD)) {
      want <- ref_base64(r, pad = flags == 0L)
      len <- nchar(want)
      for (cap in 0:(len + 2)) {
        res <- b64_enc(r, cap, flags)
        expect_identical(res[[1]], as.numeric(len))
        if (cap < len) {
          expect_true(all(res[[2]] == SENT))
        } else {
          expect_identical(rawToChar(res[[2]][seq_len(len)]), want)
          expect_true(all(res[[2]][-seq_len(len)] == SENT))
        }
      }
    }
  }
})

test_that("the bounds hold", {
  for (n in 0:20) {
    b <- .Call(zufast_test_base64_bounds, n)
    expect_identical(b[1], 4 * ceiling(n / 3))
    expect_identical(b[2], floor(n / 4) * 3 + c(0, 0, 1, 2)[n %% 4 + 1])
  }
})

test_that("the decoder rejects every malformed class with the right status", {
  INVALID <- 1L; INCOMPLETE <- 3L; NO_SPACE <- 4L
  st <- function(s, flags = 0L) b64_dec(s, flags = flags)[[1]]
  # partial or misplaced padding
  expect_identical(st("Zg="), INVALID)
  expect_identical(st("Zg"), INVALID)                 # padding required
  expect_identical(st("Zg", NO_PAD), 0L)
  expect_identical(st("Zg==", NO_PAD), 0L)            # complete padding still fine
  expect_identical(st("Zg=", NO_PAD), INVALID)
  expect_identical(st("Z==="), INVALID)
  expect_identical(st("===="), INVALID)
  expect_identical(st("=="), INVALID)
  expect_identical(st("Zm=v"), INVALID)
  expect_identical(st("Zg==Zg=="), INVALID)
  # final quantum of one character
  expect_identical(st("Z"), INCOMPLETE)
  expect_identical(st("Zm9vY"), INCOMPLETE)
  expect_identical(st("Zm9vY", NO_PAD), INCOMPLETE)
  # non-zero trailing bits
  expect_identical(st("Zh=="), INVALID)
  expect_identical(st("Zm9="), INVALID)
  expect_identical(st("Zh", NO_PAD), INVALID)
  # whitespace and foreign bytes
  expect_identical(st("Zm9v\n"), INVALID)
  expect_identical(st(" Zm9v"), INVALID)
  expect_identical(st("Zm 9v"), INVALID)
  # alphabet mixing
  expect_identical(st("-_-_"), INVALID)
  expect_identical(st("+/+/", URL), INVALID)
  expect_identical(st("-_-_", URL), 0L)
  for (b in 0:255) {
    ch <- rawToChar(as.raw(if (b == 0) 1 else b))
    ok_std <- ch %in% c(B64_STD, "=")   # "AAA=" is valid padding
    s <- paste0("AAA", ch)
    expect_identical(st(s) == 0L, ok_std, label = sprintf("byte %d", b))
  }
  # NO_SPACE before anything is written
  res <- b64_dec("Zm9vYmFy", cap = 5)
  expect_identical(res[[1]], NO_SPACE)
  expect_identical(res[[2]], 0)
  expect_true(all(res[[3]] == SENT))
  res <- b64_dec("Zm9vYmE=", cap = 5)
  expect_identical(res[[1]], 0L)
  expect_identical(rawToChar(res[[3]][1:5]), "fooba")
  expect_true(all(res[[3]][-(1:5)] == SENT))
})

test_that("the R API is vectorised and checks its arguments", {
  expect_identical(fast_base64_encode(c("a", NA)), c("YQ==", NA))
  expect_identical(fast_base64_decode(c("YQ==", NA, "Y")), list(charToRaw("a"), NULL, NULL))
  expect_identical(fast_base64_encode(as.raw(c(0xfb, 0xff)), url = TRUE), "-_8=")
  expect_error(fast_base64_encode(1), class = "zufast_invalid_argument")
  expect_error(fast_base64_encode("a", pad = "yes"), class = "zufast_invalid_argument")
  expect_error(fast_base64_decode(raw()), class = "zufast_error")
})
