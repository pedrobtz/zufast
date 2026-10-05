#' Parse numbers
#'
#' `fast_parse_double()` parses decimal and scientific notation, `Inf`,
#' `-Inf`, `Infinity` and `NaN` in any case, with a leading `-` or `+`, after
#' trimming ASCII whitespace. The result is correctly rounded: the nearest
#' double to the decimal value, ties to even, on every platform.
#'
#' This is not exactly [as.numeric()]: R accumulates digits in `long double`
#' and is not correctly rounded, so the two differ in the last bit for
#' roughly one ordinary decimal in five thousand (`"0.799012"` is one), and
#' R's result depends on whether it was built with `long double`. Hexadecimal
#' input, which `as.numeric()` accepts, is not accepted.
#'
#' `fast_parse_integer()` parses a decimal integer and returns `NA` for
#' anything that is not one or does not fit an R integer, rather than going
#' through a double as `as.integer()` does: `"1.5"`, `"1e3"` and
#' `"3000000000"` are all `NA`.
#'
#' Invalid input is `NA` with no warning.
#'
#' @param x A character vector.
#' @return A double vector (`fast_parse_double()`) or an integer vector
#'   (`fast_parse_integer()`) of the same length as `x`.
#' @export
#' @examples
#' fast_parse_double(c("1.5", " -2e-3 ", "+inf", "0x10", "abc", NA))
#' fast_parse_integer(c("42", "+7", "2147483647", "2147483648", "1.0"))
fast_parse_double <- function(x) {
  check_character(x)
  .Call(zufast_parse_double, x)
}

#' @rdname fast_parse_double
#' @export
fast_parse_integer <- function(x) {
  check_character(x)
  .Call(zufast_parse_integer, x)
}

#' Format doubles with the shortest round-trip digits
#'
#' Writes each value with the fewest significant digits that parse back to
#' exactly the same double, in the notation of ECMAScript's
#' `Number.prototype.toString()`: positional for decimal exponents from -6 to
#' 20 (`0.000001`, `0.1`, `123.5`, `100`), `d.ddde+x` otherwise (`1e+21`,
#' `1e-7`).
#'
#' This differs from [as.character()], which writes 15 significant digits:
#' `as.character(0.1 + 0.2)` is `"0.3"`, while `fast_format_double(0.1 + 0.2)`
#' is `"0.30000000000000004"`, because those are different doubles.
#' `fast_parse_double(fast_format_double(x))` is identical to `x` for every
#' double, including `-0`, which is written as `"-0"`.
#'
#' @param x A double vector.
#' @param scientific Always use `d.ddde+x` notation.
#' @param trailing_zero Write integral values in positional notation with a
#'   trailing `.0`, as in `"1.0"`, so that they read back as non-integers in
#'   languages that distinguish the two.
#' @return A character vector; `NA` stays `NA`, `NaN`, `Inf` and `-Inf` are
#'   written as R writes them.
#' @export
#' @examples
#' fast_format_double(c(1, 0.1, 0.1 + 0.2, 1e21, 1e-7, -0, NA, Inf))
#' fast_format_double(123.456, scientific = TRUE)
#' fast_format_double(c(1, 2.5), trailing_zero = TRUE)
fast_format_double <- function(x, scientific = FALSE, trailing_zero = FALSE) {
  if (!is.double(x)) {
    invalid_argument("`x` must be a double vector.")
  }
  check_flag(scientific, "scientific")
  check_flag(trailing_zero, "trailing_zero")
  .Call(zufast_format_double, x, as.integer(scientific) + 2L * as.integer(trailing_zero))
}
