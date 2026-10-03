# Helpers for test-uuid.R, kept out of it so that the tests run shuffled.

parse_uuid <- function(s) .Call(zufast_test_parse_uuid, charToRaw(s))

format_uuid <- function(bytes, cap = 36, upper = FALSE) .Call(zufast_test_format_uuid, bytes, cap, upper)

SENT <- as.raw(0xA5)

U <- "123e4567-e89b-12d3-a456-426614174000"

U_BYTES <- as.raw(c(0x12, 0x3e, 0x45, 0x67, 0xe8, 0x9b, 0x12, 0xd3, 0xa4, 0x56,
                    0x42, 0x66, 0x14, 0x17, 0x40, 0x00))

