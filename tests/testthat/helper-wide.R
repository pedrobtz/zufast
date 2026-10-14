# Helpers for test-wide.R. Every call goes to both builds of the wide.h
# harness, the native one and the ZUF_NO_INT128 one, and requires identical
# results before returning them.

wide <- function(fn, ...) {
  native <- .Call(get(paste0("zufast_test_wide_", fn)), ...)
  fallback <- .Call(get(paste0("zufast_test_wide_fallback_", fn)), ...)
  testthat::expect_identical(fallback, native, label = paste0("ZUF_NO_INT128 ", fn, "()"))
  native
}

# Decimal text to the hex the harness takes, through zuf_parse_*128().
i128 <- function(x) sub("^0 [0-9]+ ", "", wide("parse", "i", x))
u128 <- function(x) sub("^0 [0-9]+ ", "", wide("parse", "u", x))

# Hex results back to decimal through zuf_write_*128(); "F" stays "F".
dec <- function(x, kind = "i") {
  out <- x
  ok <- x != "F"
  if (any(ok)) out[ok] <- wide("op", paste0(kind, "_write"), x[ok], x[ok])
  out
}

# op on decimal operands with a decimal result; divisions give "q r".
wop <- function(op, a, b = a, kind = substr(op, 1, 1)) {
  conv <- if (kind == "u") u128 else i128
  r <- wide("op", op, conv(a), conv(b))
  vapply(r, function(s) {
    if (s == "F" || !grepl("^[0-9a-f]{32}", s)) return(s)
    paste(dec(strsplit(s, " ")[[1]], kind), collapse = " ")
  }, "", USE.NAMES = FALSE)
}

I128_MAX <- "170141183460469231731687303715884105727"
I128_MIN <- "-170141183460469231731687303715884105728"
U128_MAX <- "340282366920938463463374607431768211455"
