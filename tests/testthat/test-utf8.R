test_that("the DFA agrees with a reference validator exhaustively", {
  r <- .Call(zufast_test_utf8_exhaustive)
  expect_identical(r[1], 0)
  expect_gt(r[2], 3e7)
})


test_that("Markus Kuhn's stress-test classes are classified correctly", {
  valid <- list(
    bytes(0x7f), bytes(0xc2, 0x80), bytes(0xdf, 0xbf), bytes(0xe0, 0xa0, 0x80),
    bytes(0xef, 0xbf, 0xbf), bytes(0xf0, 0x90, 0x80, 0x80),
    bytes(0xf4, 0x8f, 0xbf, 0xbf), bytes(0xed, 0x9f, 0xbf),
    bytes(0xee, 0x80, 0x80), bytes(0xef, 0xbf, 0xbe),   # noncharacter U+FFFE is valid
    raw(0)
  )
  invalid <- list(
    bytes(0x80), bytes(0xbf), bytes(0x80, 0xbf, 0x80),      # lone continuations
    bytes(0xc0, 0x20), bytes(0xe0, 0x80, 0x20),             # truncated + space
    bytes(0xc2), bytes(0xe0, 0xa0), bytes(0xf0, 0x90, 0x80), # incomplete at end
    bytes(0xfe), bytes(0xff), bytes(0xfe, 0xfe, 0xff, 0xff),
    bytes(0xc0, 0xaf), bytes(0xe0, 0x80, 0xaf), bytes(0xf0, 0x80, 0x80, 0xaf), # overlong /
    bytes(0xc1, 0xbf), bytes(0xe0, 0x9f, 0xbf), bytes(0xf0, 0x8f, 0xbf, 0xbf), # max overlong
    bytes(0xc0, 0x80), bytes(0xe0, 0x80, 0x80),             # overlong NUL
    bytes(0xed, 0xa0, 0x80), bytes(0xed, 0xbf, 0xbf),       # surrogates
    bytes(0xed, 0xa0, 0x80, 0xed, 0xb0, 0x80),              # surrogate pair
    bytes(0xf4, 0x90, 0x80, 0x80), bytes(0xf7, 0xbf, 0xbf, 0xbf), # > U+10FFFF
    bytes(0xf8, 0x88, 0x80, 0x80, 0x80), bytes(0xfc, 0x84, 0x80, 0x80, 0x80, 0x80)
  )
  for (v in valid) expect_true(fast_utf8_valid(v), label = paste(v, collapse = " "))
  for (v in invalid) expect_false(fast_utf8_valid(v), label = paste(v, collapse = " "))
})

test_that("fast_utf8_valid() agrees with validUTF8() on random byte strings", {
  set.seed(20261003)
  pool <- as.raw(c(0x00:0x7f, 0x80, 0x8f, 0x90, 0x9f, 0xa0, 0xbf, 0xc0:0xc3,
                   0xdf, 0xe0, 0xe1, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf4, 0xf5,
                   0xf8, 0xfe, 0xff))
  strs <- vapply(1:4000, function(i) {
    r <- sample(pool, sample(0:12, 1), replace = TRUE)
    r <- r[r != as.raw(0)]
    rawToChar(r)
  }, "")
  expect_identical(fast_utf8_valid(strs), validUTF8(strs))
})

test_that("fast_utf8_valid() handles NA, empty input, and long ASCII runs", {
  expect_identical(fast_utf8_valid(c(NA, "", "abc")), c(NA, TRUE, TRUE))
  long <- strrep("abcdefgh", 1000)
  expect_true(fast_utf8_valid(long))
  expect_false(fast_utf8_valid(paste0(long, "\xff")))
  expect_false(fast_utf8_valid(paste0("\xff", long)))
  expect_identical(fast_utf8_valid(character()), logical())
})

test_that("fast_utf8_valid() rejects other types with a classed condition", {
  expect_error(fast_utf8_valid(1), class = "zufast_invalid_argument")
  expect_error(fast_utf8_valid(NULL), class = "zufast_error")
})

test_that("zuf_utf8_count() counts code points up to the first error", {
  cnt <- function(r) .Call(zufast_test_utf8_count, r)
  expect_identical(cnt(bytes(0x63, 0x61, 0x66, 0xc3, 0xa9, 0x20, 0xe2, 0x82, 0xac,
                             0xf0, 0x9f, 0x98, 0x80)), c(1, 7))
  expect_identical(cnt(raw(0)), c(1, 0))
  expect_identical(cnt(bytes(0x61, 0x62, 0xff, 0x63)), c(0, 2))
  expect_identical(cnt(bytes(0x61, 0xe2, 0x82)), c(0, 1))
})
