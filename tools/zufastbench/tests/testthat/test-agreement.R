run <- function(f, x) {
  out <- double(length(x))
  f(x, out)
  out
}

test_that("every parser reads the benchmark input back", {
  set.seed(42)
  invec <- sample(sqrt(seq_len(1e4)))
  input <- as.character(invec)
  for (f in list(zufast, fastfloat, strtod, atof, sscanf, stod, zufast_spans, fastfloat_spans)) {
    expect_equal(run(f, input), invec)
  }
})

test_that("zufast and fast_float give bit-identical doubles on hard decimals", {
  set.seed(7)
  n <- 20000
  digits <- vapply(sample(1:40, n, TRUE),
                   function(k) paste(sample(0:9, k, TRUE), collapse = ""), "")
  x <- c(paste0(digits, "e", sample(-340:340, n, TRUE)),
         "9007199254740993", "2.2250738585072011e-308", "4.9406564584124654e-324",
         "2.4703282292062328e-324", "1.7976931348623157e308", "1e23", "0.1",
         "7.038531e-26", "1.00000005960464477539062500", paste0("0.", strrep("1", 800)),
         "-0", "inf", "-infinity", "nan")
  expect_identical(run(zufast_spans, x), run(fastfloat_spans, x))
  expect_identical(run(zufast, x), run(fastfloat, x))
  # and both agree with the C library, which is exact on glibc
  skip_if_not(Sys.info()[["sysname"]] == "Linux")
  finite <- is.finite(run(strtod, x))
  expect_identical(run(zufast_spans, x)[finite], run(strtod, x)[finite])
})
