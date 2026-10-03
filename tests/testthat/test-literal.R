BOOL <- c(LOWER = 1L, UPPER = 2L, TITLE = 4L, LETTER = 8L, DIGIT = 16L, YESNO = 32L)
BOOL_R <- BOOL[["LOWER"]] + BOOL[["UPPER"]] + BOOL[["TITLE"]] + BOOL[["LETTER"]]
BOOL_ALL <- sum(BOOL)

parse_bool <- function(x, accept) .Call(zufast_test_parse_bool, x, accept)

test_that("zuf_parse_bool() accepts exactly the selected spellings", {
  spellings <- list(
    LOWER = c(true = TRUE, false = FALSE),
    UPPER = c(`TRUE` = TRUE, `FALSE` = FALSE),
    TITLE = c(True = TRUE, False = FALSE),
    LETTER = c(`T` = TRUE, `F` = FALSE, t = TRUE, f = FALSE),
    DIGIT = c("1" = TRUE, "0" = FALSE),
    YESNO = c(yes = TRUE, no = FALSE, Yes = TRUE, No = FALSE, YES = TRUE,
              NO = FALSE, y = TRUE, n = FALSE, Y = TRUE, N = FALSE,
              on = TRUE, off = FALSE, On = TRUE, Off = FALSE, ON = TRUE,
              OFF = FALSE)
  )
  for (family in names(spellings)) {
    for (word in names(spellings[[family]])) {
      r <- parse_bool(word, BOOL[[family]])
      expect_identical(r, c(0L, nchar(word), as.integer(spellings[[family]][[word]])),
                       label = paste(family, word))
      # No other family alone accepts the whole word, unless the same
      # spelling is also theirs (a prefix such as "T" of "TRUE" may match).
      for (other in setdiff(names(BOOL), family)) {
        if (word %in% names(spellings[[other]])) next
        r <- parse_bool(word, BOOL[[other]])
        expect_false(r[1] == 0L && r[2] == nchar(word),
                     label = paste(other, "accepts", word))
      }
    }
  }
})

test_that("ZUF_BOOL_R matches as.logical() on its spellings", {
  words <- c("TRUE", "true", "True", "T", "FALSE", "false", "False", "F",
             "t", "f", "yes", "1", "0", "tRUE", "")
  for (w in words) {
    r <- parse_bool(w, BOOL_R)
    ours <- if (r[1] == 0L && r[2] == nchar(w)) as.logical(r[3]) else NA
    # as.logical() does not accept lower-case t and f
    if (w %in% c("t", "f")) next
    expect_identical(ours, as.logical(w), label = w)
  }
})

test_that("zuf_parse_bool() takes the longest match and reports where it stopped", {
  expect_identical(parse_bool("TRUE", BOOL_R), c(0L, 4L, 1L))
  expect_identical(parse_bool("Trueish", BOOL_R), c(0L, 4L, 1L))
  expect_identical(parse_bool("Tx", BOOL_R), c(0L, 1L, 1L))
  expect_identical(parse_bool("off", BOOL_ALL), c(0L, 3L, 0L))
  expect_identical(parse_bool("no", BOOL_ALL), c(0L, 2L, 0L))
  expect_identical(parse_bool("", BOOL_ALL), c(1L, 0L, NA))
  expect_identical(parse_bool(" true", BOOL_ALL), c(1L, 0L, NA))
  expect_identical(parse_bool("true", 0L), c(1L, 0L, NA))
})

test_that("zuf_equals() and zuf_equals_ci() compare exactly and ASCII-insensitively", {
  eq <- function(x, lit, ci = FALSE) .Call(zufast_test_equals, x, lit, ci)
  expect_true(eq("NA", "NA"))
  expect_false(eq("NA", "na"))
  expect_false(eq("NA ", "NA"))
  expect_true(eq("", ""))
  expect_true(eq("NuLl", "null", TRUE))
  expect_false(eq("nul", "null", TRUE))
  expect_false(eq("[", "{", TRUE))   # 0x5B vs 0x7B differ by the case bit
  e_acute <- rawToChar(as.raw(c(0xc3, 0xa9)))     # U+00E9
  E_acute <- rawToChar(as.raw(c(0xc3, 0x89)))     # U+00C9
  expect_true(eq(paste0("caf", E_acute), paste0("CAF", E_acute), TRUE))
  expect_false(eq(paste0("caf", e_acute), paste0("caf", E_acute), TRUE))   # non-ASCII is exact
})

test_that("zuf_skip_space() and zuf_trim_space() handle every ASCII space", {
  trim <- function(x) .Call(zufast_test_trim, x)
  expect_identical(trim(" \t\n\v\f\rab c \r\n"), c(6L, 6L, 10L))
  expect_identical(trim("abc"), c(0L, 0L, 3L))
  expect_identical(trim("   "), c(3L, 3L, 3L))
  expect_identical(trim(""), c(0L, 0L, 0L))
  expect_identical(trim(rawToChar(as.raw(c(0xc2, 0xa0, 0x78)))), c(0L, 0L, 3L))   # NBSP is not ASCII space
})
