# Helpers for test-format.R, kept out of it so that the tests run shuffled.

SCI <- 1L; TRAIL <- 2L

fmt <- function(x, flags = 0L, kind = 0L, cap = 64) {
  r <- .Call(zufast_test_format_shortest, x, flags, kind, cap)
  if (r[[1]] <= cap) rawToChar(r[[2]][seq_len(r[[1]])]) else r
}

