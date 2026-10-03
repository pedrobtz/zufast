# Helpers for test-base64.R, kept out of it so that the tests run shuffled.

URL <- 1L

NO_PAD <- 2L

b64_enc <- function(raw, cap, flags = 0L) .Call(zufast_test_base64_encode, raw, cap, flags)

b64_dec <- function(s, cap = 100, flags = 0L) .Call(zufast_test_base64_decode, str_raw(s), cap, flags)

SENT <- as.raw(0xA5)

