check_flag <- function(x, name) {
  if (!is.logical(x) || length(x) != 1L || is.na(x)) {
    invalid_argument(sprintf("`%s` must be TRUE or FALSE.", name), call = sys.call(-1))
  }
}

check_encode_input <- function(x) {
  if (!is.raw(x) && !is.character(x)) {
    invalid_argument("`x` must be a raw or character vector.", call = sys.call(-1))
  }
}

check_decode_input <- function(x) {
  if (!is.character(x)) {
    invalid_argument("`x` must be a character vector.", call = sys.call(-1))
  }
}

#' Hexadecimal encoding and decoding
#'
#' `fast_hex_encode()` writes two hex digits per byte. `fast_hex_decode()`
#' accepts digits in either case and rejects an odd length or any other byte.
#'
#' @param x For encoding, a raw vector (encoded as one string) or a character
#'   vector (the bytes of each element encoded separately, `NA` kept). For
#'   decoding, a character vector.
#' @param upper Use upper-case digits.
#' @return `fast_hex_encode()` returns a character vector. `fast_hex_decode()`
#'   returns a list of raw vectors, one per element of `x`, with `NULL` where
#'   the element is `NA` or not valid hex.
#' @export
#' @examples
#' fast_hex_encode(as.raw(c(0xde, 0xad, 0xbe, 0xef)))
#' fast_hex_decode(c("DEADbeef", "abc", NA))
fast_hex_encode <- function(x, upper = FALSE) {
  check_encode_input(x)
  check_flag(upper, "upper")
  .Call(zufast_encode, x, 0L, as.integer(upper))
}

#' @rdname fast_hex_encode
#' @export
fast_hex_decode <- function(x) {
  check_decode_input(x)
  .Call(zufast_decode, x, 0L, 0L)
}

#' Base64 encoding and decoding
#'
#' Standard (RFC 4648 section 4) or URL-safe (section 5) Base64. Decoding is
#' strict: no whitespace, nothing outside the selected alphabet, padding
#' either complete or absent, and the unused trailing bits zero, so that
#' every accepted input has exactly one encoding.
#'
#' @inheritParams fast_hex_encode
#' @param url Use the URL-safe alphabet (`-` and `_` instead of `+` and `/`).
#' @param pad Write `=` padding.
#' @return `fast_base64_encode()` returns a character vector.
#'   `fast_base64_decode()` returns a list of raw vectors, one per element of
#'   `x`, with `NULL` where the element is `NA` or not valid Base64.
#' @export
#' @examples
#' fast_base64_encode(charToRaw("hello"))
#' fast_base64_encode("hello", url = TRUE, pad = FALSE)
#' rawToChar(fast_base64_decode("aGVsbG8=")[[1]])
fast_base64_encode <- function(x, url = FALSE, pad = TRUE) {
  check_encode_input(x)
  check_flag(url, "url")
  check_flag(pad, "pad")
  .Call(zufast_encode, x, 1L, b64_flags(url, !pad))
}

#' @rdname fast_base64_encode
#' @export
fast_base64_decode <- function(x, url = FALSE) {
  check_decode_input(x)
  check_flag(url, "url")
  .Call(zufast_decode, x, 1L, b64_flags(url, TRUE))
}

b64_flags <- function(url, no_pad) {
  as.integer(url) + 2L * as.integer(no_pad)
}
