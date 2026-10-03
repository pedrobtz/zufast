# Helpers for test-bits.R, kept out of it so that the tests run shuffled.

f16_value <- function(bits) {
  sign <- ifelse(bitwAnd(bits, 0x8000L) != 0L, -1, 1)
  e <- bitwAnd(bitwShiftR(bits, 10L), 0x1FL)
  m <- bitwAnd(bits, 0x3FFL)
  v <- ifelse(e == 0L, m * 2^-24,
       ifelse(e == 31L, ifelse(m == 0L, Inf, NaN), (1 + m / 1024) * 2^(e - 15)))
  sign * v
}

bf16_value <- function(bits) {
  sign <- ifelse(bitwAnd(bits, 0x8000L) != 0L, -1, 1)
  e <- bitwAnd(bitwShiftR(bits, 7L), 0xFFL)
  m <- bitwAnd(bits, 0x7FL)
  v <- ifelse(e == 0L, m * 2^-133,
       ifelse(e == 255L, ifelse(m == 0L, Inf, NaN), (1 + m / 128) * 2^(e - 127)))
  sign * v
}

# Round a positive float-exact value to the nearest pattern of a finite,
# monotone table of non-negative values, ties to the even pattern. Values
# beyond the last entry map to `inf_bits`; callers fix up the band that
# still rounds down to the largest finite value.
round_to_table <- function(x, values, max_finite_bits, inf_bits) {
  i <- findInterval(x, values)          # values[i] <= x < values[i + 1]
  out <- integer(length(x))
  for (k in seq_along(x)) {
    j <- i[k]
    if (j >= length(values)) {
      # beyond the largest finite value: halfway to the next power is the cut
      out[k] <- inf_bits
      next
    }
    lo <- values[j]; hi <- values[j + 1]
    d_lo <- x[k] - lo; d_hi <- hi - x[k]
    b_lo <- j - 1L; b_hi <- j
    out[k] <- if (d_lo < d_hi) b_lo else if (d_hi < d_lo) b_hi else
      if (b_lo %% 2L == 0L) b_lo else b_hi
  }
  out
}

