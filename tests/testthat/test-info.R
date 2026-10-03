test_that("fast_info() reports the header version from compiled code", {
  info <- fast_info()
  expect_type(info, "list")
  expect_named(info, c("version", "version_major", "version_minor",
                       "version_patch", "compiler", "vendored"))
  expect_identical(
    info$version,
    paste(info$version_major, info$version_minor, info$version_patch, sep = ".")
  )
  expect_type(info$compiler, "character")
})

test_that("the header version matches DESCRIPTION", {
  desc <- unlist(utils::packageVersion("zufast"))
  info <- fast_info()
  expect_identical(
    c(info$version_major, info$version_minor, info$version_patch),
    as.integer(desc[1:3])
  )
})

test_that("fast_info() reports the pinned vendor versions from compiled code", {
  # tools/ is not in the built package; this runs from a source checkout.
  path <- test_path("..", "..", "tools", "vendor", "manifest.tsv")
  skip_if_not(file.exists(path), "no source checkout")
  manifest <- utils::read.delim(path, colClasses = "character")
  info <- fast_info()$vendored
  for (i in seq_len(nrow(manifest))) {
    expect_identical(info[[manifest$source[i]]], manifest$version_string[i])
  }
})
