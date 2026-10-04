test_that("zuf_status_string() covers every enumerator and never returns NULL", {
  s <- vapply(0:4, function(i) .Call(zufast_test_status_string, i), "")
  expect_identical(s, c("ok", "invalid input", "value out of range",
                        "incomplete input", "output buffer too small"))
  expect_identical(.Call(zufast_test_status_string, 5L), "unknown status")
  expect_identical(.Call(zufast_test_status_string, -1L), "unknown status")
})
