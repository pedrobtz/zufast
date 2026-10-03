# Helpers for test-number.R, kept out of it so that the tests run shuffled.

OK <- 0L; INVALID <- 1L; RANGE <- 2L

JSON <- 1L; PLUS <- 2L; SPACE <- 4L

pn <- function(s, kind = 0L, flags = 0L, base = 10L, dp = 0L) {
  .Call(zufast_test_parse_num, charToRaw(s), kind, flags, base, dp)
}

