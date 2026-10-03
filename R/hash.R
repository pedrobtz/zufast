#' XXH3 hashes
#'
#' Computes the 64- or 128-bit XXH3 hash of raw bytes or of the bytes of each
#' string, as lower-case hexadecimal in xxHash's canonical (big-endian) form.
#' The values are identical to those of the reference implementation, so
#' they can be compared with hashes computed elsewhere.
#'
#' **XXH3 is not a cryptographic hash.** Use it for caches, deduplication,
#' fingerprints and change detection, never where an adversary could choose
#' the input to produce a collision; that needs a cryptographic digest.
#'
#' @param x A raw vector (hashed as one input) or a character vector (each
#'   element hashed separately; the encoding is not converted, the bytes are
#'   hashed as stored).
#' @param bits `64` or `128`.
#' @param seed A whole number from 0 to 2^53.
#' @return A character vector of 16 (64-bit) or 32 (128-bit) hex digits, `NA`
#'   where `x` is `NA`.
#' @export
#' @examples
#' fast_hash(charToRaw("hello"))
#' fast_hash(c("a", "b", NA), bits = 128)
#' fast_hash("hello", seed = 42)
fast_hash <- function(x, bits = 64, seed = 0) {
  if (!is.raw(x) && !is.character(x)) {
    invalid_argument("`x` must be a raw or character vector.")
  }
  if (!is.numeric(bits) || length(bits) != 1L || !(bits %in% c(64, 128))) {
    invalid_argument("`bits` must be 64 or 128.")
  }
  if (!is.numeric(seed) || length(seed) != 1L || is.na(seed) || seed < 0 ||
      seed > 2^53 || seed != floor(seed)) {
    invalid_argument("`seed` must be a whole number from 0 to 2^53.")
  }
  .Call(zufast_hash, x, as.integer(bits), as.double(seed))
}
