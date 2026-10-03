# Helpers for test-literal.R, kept out of it so that the tests run shuffled.

BOOL <- c(LOWER = 1L, UPPER = 2L, TITLE = 4L, LETTER = 8L, DIGIT = 16L, YESNO = 32L)

BOOL_R <- BOOL[["LOWER"]] + BOOL[["UPPER"]] + BOOL[["TITLE"]] + BOOL[["LETTER"]]

BOOL_ALL <- sum(BOOL)

parse_bool <- function(x, accept) .Call(zufast_test_parse_bool, x, accept)

