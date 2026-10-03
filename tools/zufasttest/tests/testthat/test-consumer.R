# Every public function of zufast is called from the fixture's two
# translation units; these are their results.

test_that("text: numbers, dates and literals", {
  r <- .Call(zt_text, "  0.1  ")
  expect_identical(r[1], "value out of range")
  expect_match(r[2], "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  expect_identical(r[3], "0")                         # untrimmed: " " is not a number
  r <- .Call(zt_text, "0.1")
  expect_identical(r[3:7], c("0.1", "0.1", "1e-1", "0.1", "0.100"))
  expect_identical(r[8], "1e-1/1+")
  expect_identical(r[9], "12345 12345 12345 12345")
  expect_identical(r[10], "74565 74565 74565 74565")   # base 16
  expect_identical(r[11], "2024-02-29T12:30:45.500+01:00")
  expect_identical(r[12], "1709206245.500000000")
  expect_identical(r[13], "2024-02-29")
  expect_identical(r[14], "2024 3 1 29 L")
  r <- .Call(zt_text, " yes ")
  expect_identical(r[15:18], c("yes", "space", "true", "other"))
  expect_identical(.Call(zt_text, "NULL")[18], "null")
  expect_identical(.Call(zt_text, "NA")[18], "NA")
})

test_that("binary: hex, Base64, UUID, hashing, bits, UTF-8", {
  r <- .Call(zt_binary, charToRaw("hello"))
  expect_identical(r[1:2], c("68656c6c6f", "hex-ok"))
  expect_identical(r[3:4], c("aGVsbG8", "b64-ok"))
  expect_identical(r[5:6], c("68656C6C-6F00-0000-0000-000000000000", "uuid-ok"))
  expect_identical(r[7], "9555e8555c62dcfd")   # XXH3_64bits("hello")
  expect_identical(nchar(r[8]), 32L)
  expect_identical(r[9:11], c("stream-ok", "bits-ok", "utf8"))
  expect_identical(.Call(zt_binary, as.raw(0xff))[11], "not-utf8")
})
