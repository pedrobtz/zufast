# Extracted from test-consumer.R:27

# setup ------------------------------------------------------------------------
library(testthat)
test_env <- simulate_test_env(package = "zufasttest", path = "..")
attach(test_env, warn.conflicts = FALSE)

# test -------------------------------------------------------------------------
r <- .Call(zt_binary, charToRaw("hello"))
expect_identical(r[1:2], c("68656c6c6f", "hex-ok"))
expect_identical(r[3:4], c("aGVsbG8", "b64-ok"))
