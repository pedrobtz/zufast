#' Validate UTF-8
#'
#' Checks that bytes are well-formed UTF-8: no overlong forms, no surrogates
#' (U+D800 to U+DFFF), nothing above U+10FFFF. The result agrees with
#' [validUTF8()]; the declared encoding of a string is ignored, only its bytes
#' are examined.
#'
#' @param x A character vector, or a raw vector holding one byte sequence.
#' @return For a character vector, a logical vector of the same length, `NA`
#'   where `x` is `NA`. For a raw vector, a single `TRUE` or `FALSE`.
#' @export
#' @examples
#' fast_utf8_valid(c("plain", rawToChar(as.raw(c(0x63, 0x61, 0x66, 0xc3, 0xa9))), NA))
#' fast_utf8_valid(as.raw(c(0xc3, 0x28)))   # invalid continuation byte
fast_utf8_valid <- function(x) {
  if (!is.character(x) && !is.raw(x)) {
    invalid_argument("`x` must be a character or raw vector.")
  }
  .Call(zufast_utf8_valid, x)
}
