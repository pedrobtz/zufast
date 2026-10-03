test_that("zuf_status_string() covers every enumerator and never returns NULL", {
  s <- vapply(0:4, function(i) .Call(zufast_test_status_string, i), "")
  expect_identical(s, c("ok", "invalid input", "value out of range",
                        "incomplete input", "output buffer too small"))
  expect_identical(.Call(zufast_test_status_string, 5L), "unknown status")
  expect_identical(.Call(zufast_test_status_string, -1L), "unknown status")
})

test_that("zuf_int_mul128() multiplies exactly", {
  halves <- function(a, b) .Call(zufast_test_mul128, a, b)
  expect_identical(halves(0, 12345), c(0, 0, 0, 0))
  expect_identical(halves(3, 5), c(15, 0, 0, 0))
  # 2^52 * 2^52 = 2^104: bit 8 of the top 32-bit word.
  expect_identical(halves(2^52, 2^52), c(0, 0, 0, 2^8))
  # (2^53 - 1)^2 = 2^106 - 2^54 + 1
  expect_identical(halves(2^53 - 1, 2^53 - 1), c(1, 2^32 - 2^22, 2^32 - 1, 2^10 - 1))
})
