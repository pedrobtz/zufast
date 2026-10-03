test_that("XXH3 equals xxHash's published sanity vectors", {
  v <- utils::read.delim(test_path("fixtures", "xxh3-vectors.tsv"),
                         colClasses = c("integer", "integer", "character", "character", "character"))
  v64 <- v[v$kind == 64, ]
  v128 <- v[v$kind == 128, ]
  expect_gt(nrow(v64), 500)
  expect_gt(nrow(v128), 500)
  expect_identical(.Call(zufast_test_xxh3_vectors, v64$len, v64$seed, 64L), v64$hash_low)
  expect_identical(.Call(zufast_test_xxh3_vectors, v128$len, v128$seed, 128L),
                   paste(v128$hash_low, v128$hash_high))
})

test_that("one-shot equals XXH3 directly and streaming equals one-shot at every split", {
  expect_identical(.Call(zufast_test_xxh3_streaming, 1024L), 0)
})

test_that("fast_hash() writes the canonical hex form", {
  # XXH3_64bits("") and XXH3_128bits("") from the sanity vectors
  expect_identical(fast_hash(raw()), "2d06800538d394c2")
  expect_identical(fast_hash(raw(), bits = 128), "99aa06d3014798d86001c324468d497f")
  expect_identical(fast_hash(""), "2d06800538d394c2")
  expect_identical(fast_hash(c("abc", NA)), c(fast_hash(charToRaw("abc")), NA))
  expect_false(fast_hash("abc", seed = 1) == fast_hash("abc"))
  expect_identical(nchar(fast_hash(letters, bits = 128)), rep(32L, 26))
  expect_identical(fast_hash(character()), character())
})

test_that("fast_hash() checks its arguments", {
  expect_error(fast_hash(1), class = "zufast_invalid_argument")
  expect_error(fast_hash("a", bits = 32), class = "zufast_invalid_argument")
  expect_error(fast_hash("a", seed = -1), class = "zufast_invalid_argument")
  expect_error(fast_hash("a", seed = 1.5), class = "zufast_invalid_argument")
  expect_error(fast_hash("a", seed = NA), class = "zufast_error")
})
