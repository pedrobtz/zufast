test_that("fast_info() reports the header version from compiled code", {
  info <- fast_info()
  expect_type(info, "list")
  expect_named(info, c("version", "version_major", "version_minor",
                       "version_patch", "compiler"))
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
