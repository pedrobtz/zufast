# Reference implementations written in R (design 21.4), so that no Suggests
# dependency is needed.

ref_hex <- function(raw, upper = FALSE) {
  h <- sprintf(if (upper) "%02X" else "%02x", as.integer(raw))
  paste(h, collapse = "")
}

B64_STD <- c(LETTERS, letters, 0:9, "+", "/")
B64_URL <- c(LETTERS, letters, 0:9, "-", "_")

ref_base64 <- function(raw, url = FALSE, pad = TRUE) {
  alpha <- if (url) B64_URL else B64_STD
  n <- length(raw)
  if (n == 0L) return("")
  bits <- as.integer(rawToBits(rev(raw)))   # little-endian bit order of reversed bytes
  bits <- rev(bits)                          # most significant first, byte order kept
  extra <- (6L - length(bits) %% 6L) %% 6L
  bits <- c(bits, integer(extra))
  groups <- matrix(bits, nrow = 6L)
  idx <- colSums(groups * c(32L, 16L, 8L, 4L, 2L, 1L))
  out <- paste(alpha[idx + 1L], collapse = "")
  if (pad) out <- paste0(out, strrep("=", (3L - n %% 3L) %% 3L))
  out
}

str_raw <- function(s) charToRaw(s)
