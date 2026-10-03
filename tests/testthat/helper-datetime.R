# Helpers for test-datetime.R, kept out of it so that the tests run shuffled.

# format(x, "%Y") does not zero-pad years below 1000, so the references are
# built from POSIXlt fields.
iso_date <- function(x) {
  lt <- as.POSIXlt(x, tz = "UTC")
  sprintf("%04d-%02d-%02d", lt$year + 1900L, lt$mon + 1L, lt$mday)
}

iso_time <- function(x) {
  lt <- as.POSIXlt(x, tz = "UTC")
  sprintf("%sT%02d:%02d:%02d", iso_date(x), lt$hour, lt$min, as.integer(lt$sec))
}

pdt <- function(s, date_only = FALSE) .Call(zufast_test_parse_datetime, charToRaw(s), date_only)

OK <- 0L; INVALID <- 1L; INCOMPLETE <- 3L

SENT <- as.raw(0xA5)

# fields: year month day hour minute second nanosecond offset has_time has_offset
fields <- function(y, mo, d, h = 0, mi = 0, s = 0, ns = 0, off = 0, has_time = 0, has_offset = 0) {
  c(y, mo, d, h, mi, s, ns, off, has_time, has_offset)
}

