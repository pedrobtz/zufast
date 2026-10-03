test_that("both forms parse to network-order bytes", {
  expect_identical(parse_uuid(U), list(0L, 36L, U_BYTES))
  expect_identical(parse_uuid(toupper(U)), list(0L, 36L, U_BYTES))
  expect_identical(parse_uuid(gsub("-", "", U)), list(0L, 32L, U_BYTES))
  expect_identical(parse_uuid(paste0(U, "xyz"))[1:2], list(0L, 36L))
})

test_that("malformed UUIDs are rejected", {
  bad <- c("", "123e4567", sub("-", "", U), paste0("{", U, "}"),
           paste0("urn:uuid:", U), sub("a456", "a45g", U),
           "123e4567-e89b-12d3-a456_426614174000",
           "123e4567e89b-12d3-a456-4266141740000",
           strrep("g", 32), substr(gsub("-", "", U), 1, 31))
  for (s in bad) {
    expect_identical(parse_uuid(s)[1:2], list(1L, 0L), label = s)
  }
})

test_that("formatting round-trips, measures, and writes nothing when short", {
  res <- format_uuid(U_BYTES)
  expect_identical(res[[1]], 36)
  expect_identical(rawToChar(res[[2]][1:36]), U)
  expect_true(all(res[[2]][-(1:36)] == SENT))
  expect_identical(rawToChar(format_uuid(U_BYTES, upper = TRUE)[[2]][1:36]), toupper(U))
  for (cap in c(0, 1, 35)) {
    res <- format_uuid(U_BYTES, cap)
    expect_identical(res[[1]], 36)
    expect_true(all(res[[2]] == SENT))
  }
  set.seed(4)
  for (i in 1:200) {
    b <- as.raw(sample(0:255, 16, replace = TRUE))
    s <- rawToChar(format_uuid(b)[[2]][1:36])
    expect_identical(parse_uuid(s)[[3]], b)
  }
})
