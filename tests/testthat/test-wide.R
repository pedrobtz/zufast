test_that("the two harness units are the native and the fallback build", {
  expect_identical(.Call(zufast_test_wide_fallback_native), 0L)
  # the native unit uses __int128 wherever the compiler defines it
  expect_true(.Call(zufast_test_wide_native) %in% c(0L, 1L))
})

test_that("decimal text parses to the right bits and writes back", {
  ref <- c(
    "0" = "00000000000000000000000000000000",
    "1" = "00000000000000000000000000000001",
    "-1" = "ffffffffffffffffffffffffffffffff",
    "170141183460469231731687303715884105727" = "7fffffffffffffffffffffffffffffff",
    "170141183460469231731687303715884105726" = "7ffffffffffffffffffffffffffffffe",
    "-170141183460469231731687303715884105728" = "80000000000000000000000000000000",
    "-170141183460469231731687303715884105727" = "80000000000000000000000000000001",
    "9223372036854775807" = "00000000000000007fffffffffffffff",
    "9223372036854775808" = "00000000000000008000000000000000",
    "9223372036854775809" = "00000000000000008000000000000001",
    "18446744073709551615" = "0000000000000000ffffffffffffffff",
    "18446744073709551616" = "00000000000000010000000000000000",
    "18446744073709551617" = "00000000000000010000000000000001",
    "9007199254740991" = "0000000000000000001fffffffffffff",
    "9007199254740992" = "00000000000000000020000000000000",
    "9007199254740993" = "00000000000000000020000000000001",
    "8640000000000000000000" = "00000000000001d460162f516f000000",
    "-8640000000000000000000" = "fffffffffffffe2b9fe9d0ae91000000",
    "9007199254740992000000000" = "00000000000773594000000000000000"
  )
  s <- names(ref)
  expect_identical(wide("parse", "i", s), sprintf("0 %d %s", nchar(s), ref))
  expect_identical(dec(unname(ref)), s)
  pos <- !startsWith(s, "-")
  expect_identical(wide("parse", "u", s[pos]), sprintf("0 %d %s", nchar(s[pos]), ref[pos]))
  expect_identical(dec(unname(ref[pos]), "u"), s[pos])

  expect_identical(wide("parse", "u", U128_MAX), paste("0 39", strrep("f", 32)))
  expect_identical(dec(strrep("f", 32), "u"), U128_MAX)
  expect_identical(nchar(U128_MAX), 39L)   # ZUF_U128_MAX_CHARS
  expect_identical(nchar(I128_MIN), 40L)   # ZUF_I128_MAX_CHARS
})

test_that("the parsers follow zuf_parse_i64(): RANGE saturates, INVALID consumes nothing", {
  # one past each bound is RANGE, with the saturated value and ptr past the digits
  expect_identical(wide("parse", "i", "170141183460469231731687303715884105728"),
                   paste("2 39", "7fffffffffffffffffffffffffffffff"))
  expect_identical(wide("parse", "i", "-170141183460469231731687303715884105729"),
                   paste("2 40", "80000000000000000000000000000000"))
  expect_identical(wide("parse", "u", "340282366920938463463374607431768211456"),
                   paste("2 39", strrep("f", 32)))
  expect_identical(wide("parse", "u", paste0(strrep("9", 60), "x")), paste("2 60", strrep("f", 32)))
  # leading zeros do not count toward overflow
  expect_identical(wide("parse", "u", paste0(strrep("0", 50), "1")),
                   paste("0 51", "00000000000000000000000000000001"))
  # as zuf_parse_i64(): "-000" is zero and "-0" too; the digits end the value
  expect_identical(wide("parse", "i", c("-000", "-0", "12a")),
                   c("0 4 00000000000000000000000000000000", "0 2 00000000000000000000000000000000",
                     "0 2 0000000000000000000000000000000c"))
  # no sign on the unsigned type, no '+', no whitespace; *out is untouched
  expect_identical(wide("parse", "i", c("--1", "+1", "-", "", " 1", "x")), rep("1 0 -", 6))
  expect_identical(wide("parse", "u", c("-1", "-0", "+1", "", " 1")), rep("1 0 -", 5))
})

test_that("checked arithmetic fails exactly on overflow; wrapping wraps", {
  expect_identical(wop("i_add", I128_MAX, "1"), "F")
  expect_identical(wop("i_add_wrap", I128_MAX, "1"), I128_MIN)
  expect_identical(wop("i_add", I128_MAX, "-1"), "170141183460469231731687303715884105726")
  expect_identical(wop("i_sub", I128_MIN, "1"), "F")
  expect_identical(wop("i_sub_wrap", I128_MIN, "1"), I128_MAX)
  expect_identical(wop("i_sub", "-1", I128_MAX), I128_MIN)
  expect_identical(wop("i_neg", I128_MIN), "F")
  expect_identical(wop("i_neg", I128_MAX), "-170141183460469231731687303715884105727")
  expect_identical(wop("i_mul", I128_MIN, "-1"), "F")
  expect_identical(wop("i_mul", I128_MIN, "1"), I128_MIN)
  expect_identical(wop("i_mul", "-85070591730234615865843651857942052864", "2"), I128_MIN)
  expect_identical(wop("i_mul", "85070591730234615865843651857942052864", "2"), "F")
  expect_identical(wop("i_mul_wrap", I128_MAX, I128_MAX), "1")
  expect_identical(wop("u_add", U128_MAX, "1"), "F")
  expect_identical(wop("u_add_wrap", U128_MAX, "1"), "0")
  expect_identical(wop("u_sub", "0", "1"), "F")
  expect_identical(wop("u_sub_wrap", "0", "1"), U128_MAX)
  expect_identical(wop("u_mul", "18446744073709551616", "18446744073709551616"), "F")
  expect_identical(wop("u_mul", "18446744073709551616", "18446744073709551615"),
                   "340282366920938463444927863358058659840")
  # Temporal: 8.64e21 ns, 2^53 s times 10^9, and the product of two 64-bit values
  expect_identical(wop("i_mul", "86400000000000", "-100000000"), "-8640000000000000000000")
  expect_identical(wop("i_mul", "9007199254740992", "1000000000"), "9007199254740992000000000")
  expect_identical(wop("u_mul_u64_u64", "18446744073709551615", "18446744073709551615"),
                   "340282366920938463426481119284349108225")
  expect_identical(wop("i_mul_i64_i64", "-9223372036854775808", "-9223372036854775808"),
                   "85070591730234615865843651857942052864")
  expect_identical(wop("i_mul_i64_i64", "-9223372036854775808", "9223372036854775807"),
                   "-85070591730234615856620279821087277056")

  expect_identical(wop("i_cmp", c(I128_MIN, "-1", "0", I128_MAX), c("0", "0", "0", "0")),
                   c("-1", "-1", "0", "1"))
  expect_identical(wop("u_cmp", c("0", U128_MAX), c(U128_MAX, "18446744073709551616")), c("-1", "1"))
  expect_identical(wop("i_is_neg", c(I128_MIN, "-1", "0", I128_MAX)), c("1", "1", "0", "0"))
  expect_identical(wop("i_to_i64", c("9223372036854775807", "9223372036854775808",
                                     "-9223372036854775808", "-9223372036854775809")),
                   c("9223372036854775807", "F", "-9223372036854775808", "F"))
  expect_identical(wop("u_to_u64", c("18446744073709551615", "18446744073709551616")),
                   c("18446744073709551615", "F"))
})

test_that("division truncates or floors, and refuses 0 and MIN / -1", {
  a <- c("-7", "7", "-7", "7", "6", "-6")
  b <- c("2", "-2", "-2", "2", "-3", "3")
  expect_identical(wop("i_divmod_trunc", a, b), c("-3 -1", "-3 1", "3 -1", "3 1", "-2 0", "-2 0"))
  expect_identical(wop("i_divmod_floor", a, b), c("-4 1", "-4 -1", "3 -1", "3 1", "-2 0", "-2 0"))
  expect_identical(wop("i_divmod_trunc_i64", a, b), wop("i_divmod_trunc", a, b))
  expect_identical(wop("i_divmod_floor_i64", a, b), wop("i_divmod_floor", a, b))
  for (op in c("i_divmod_trunc", "i_divmod_floor", "i_divmod_trunc_i64", "i_divmod_floor_i64")) {
    expect_identical(wop(op, c(I128_MIN, "5"), c("-1", "0")), c("F", "F"), label = op)
  }
  expect_identical(wop("i_divmod_trunc", I128_MIN, "1"), paste(I128_MIN, "0"))
  expect_identical(wop("i_divmod_floor", I128_MIN, I128_MAX), "-2 170141183460469231731687303715884105726")
  expect_identical(wop("u_divmod", c(U128_MAX, "5"), c("10000000000000000000", "0")),
                   c("34028236692093846346 3374607431768211455", "F"))
  expect_identical(wop("u_divmod_u64", U128_MAX, "18446744073709551615"),
                   "18446744073709551617 0")
  # nanoseconds to whole days, rounding toward negative infinity
  expect_identical(wop("i_divmod_floor_i64", "-8640000000000000000001", "86400000000000"),
                   "-100000001 86399999999999")
})

test_that("double conversion is exact or fails", {
  to <- function(kind, x) wide("to_f64", kind, if (kind == "u") u128(x) else i128(x))
  from <- function(kind, x) dec(wide("from_f64", kind, x), kind)
  expect_identical(to("i", c("0", "-1", "9007199254740992", "9007199254740993", "-9007199254740993",
                             "8640000000000000000000", "-8640000000000000000000",
                             "9007199254740992000000000", I128_MAX, I128_MIN)),
                   c(0, -1, 2^53, NA, NA, 8.64e21, -8.64e21, 2^53 * 1e9, NA, -2^127))
  expect_identical(to("u", c("340282366920938463463374607431768211455", "170141183460469231731687303715884105728")),
                   c(NA, 2^127))
  expect_identical(from("i", c(0, -0, -1, 2^53, 2^53 + 2, 8.64e21, -8.64e21, 2^53 * 1e9, -2^127)),
                   c("0", "0", "-1", "9007199254740992", "9007199254740994", "8640000000000000000000",
                     "-8640000000000000000000", "9007199254740992000000000", I128_MIN))
  expect_identical(from("i", c(2^127, 0.5, -1.5, 2^51 + 0.5, NaN, Inf, -Inf, 1e300, 5e-324)), rep("F", 9))
  expect_identical(from("u", c(0, -0, 2^127, 2^128 - 2^75)),
                   c("0", "0", "170141183460469231731687303715884105728",
                     "340282366920938425684442744474606501888"))
  expect_identical(from("u", c(-1, 2^128, 0.25, NA)), rep("F", 4))
})

test_that("native and fallback agree on random pairs, and every property holds", {
  r <- wide("random", 20000)
  expect_identical(r[[2]], 0)
})
