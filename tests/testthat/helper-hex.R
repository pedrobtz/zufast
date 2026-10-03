# Helpers for test-hex.R, kept out of it so that the tests run shuffled.

hex_enc <- function(raw, cap, upper = FALSE) .Call(zufast_test_hex_encode, raw, cap, upper)

hex_dec <- function(s, cap) .Call(zufast_test_hex_decode, str_raw(s), cap)

SENT <- as.raw(0xA5)

