#' Parse ISO 8601 / RFC 3339 dates and timestamps
#'
#' `fast_parse_date()` parses `YYYY-MM-DD`. `fast_parse_datetime()` parses a
#' date, optionally followed by `T`, `t` or a space and `HH:MM`, `HH:MM:SS`
#' or `HH:MM:SS.fff` (1 to 9 fraction digits; further digits are discarded),
#' optionally followed by `Z`, `z`, `+HH:MM`, `+HHMM` or `+HH` (or `-`).
#' `fast_datetime_fields()` returns the parsed fields themselves, for callers
#' that apply a time zone of their own.
#'
#' Validation is complete: year 0000 to 9999, month 1 to 12, a day that exists
#' in the proleptic Gregorian calendar, hour 0 to 23, minute 0 to 59, second
#' 0 to 60 (a leap second counts as the first second of the next minute), an
#' offset within 23:59. Anything else, including a string with trailing
#' bytes, is `NA` without a warning.
#'
#' @param x A character vector.
#' @return `fast_parse_date()` returns a `Date` vector. `fast_parse_datetime()`
#'   returns a `POSIXct` vector in UTC: an offset is applied, and a timestamp
#'   without one is taken to be UTC. `fast_datetime_fields()` returns a data
#'   frame with integer columns `year`, `month`, `day`, `hour`, `minute`,
#'   `second`, `nanosecond`, `offset` (seconds east of UTC, `NA` when none
#'   was given) and logical columns `has_time` and `has_offset`.
#' @export
#' @examples
#' fast_parse_date(c("2024-02-29", "2023-02-29", NA))
#' fast_parse_datetime(c("2024-03-01T12:30:00Z", "2024-03-01 12:30:00.25+01:00"))
#' fast_datetime_fields("2024-03-01T12:30:00-05:00")
fast_parse_date <- function(x) {
  check_character(x)
  structure(.Call(zufast_parse_date, x), class = "Date")
}

#' @rdname fast_parse_date
#' @export
fast_parse_datetime <- function(x) {
  check_character(x)
  .POSIXct(.Call(zufast_parse_datetime, x), tz = "UTC")
}

#' @rdname fast_parse_date
#' @export
fast_datetime_fields <- function(x) {
  check_character(x)
  cols <- .Call(zufast_datetime_fields, x)
  structure(cols, class = "data.frame", row.names = .set_row_names(length(x)))
}

#' Format dates and timestamps as RFC 3339
#'
#' Writes a `POSIXct` as `YYYY-MM-DDTHH:MM:SS[.fff]Z` in UTC, or a `Date` as
#' `YYYY-MM-DD`. Years outside 0000 to 9999 are written in the ISO 8601
#' expanded form with a sign.
#'
#' @param x A `POSIXct` or `Date` vector.
#' @param digits For `POSIXct`, the number of fraction digits, 0 to 9. `NULL`
#'   writes the shortest of 0, 3 or 6 digits that represents the time at
#'   microsecond resolution, which is about the precision a `POSIXct` double
#'   holds for present-day times.
#' @return A character vector, `NA` where `x` is `NA`, not finite, or
#'   outside the calendar's range (a day count beyond the `int32_t` range,
#'   about plus or minus 5.8 million years).
#' @export
#' @examples
#' fast_format_datetime(as.POSIXct("2024-03-01 12:30:00.25", tz = "UTC"))
#' fast_format_datetime(Sys.time(), digits = 0)
#' fast_format_datetime(as.Date("2024-03-01"))
fast_format_datetime <- function(x, digits = NULL) {
  if (inherits(x, "Date")) {
    return(.Call(zufast_format_date, as.double(unclass(x))))
  }
  if (!inherits(x, "POSIXct")) {
    invalid_argument("`x` must be a POSIXct or Date vector.")
  }
  if (is.null(digits)) {
    digits <- -1L
  } else if (!is.numeric(digits) || length(digits) != 1L || is.na(digits) ||
             digits < 0 || digits > 9 || digits != round(digits)) {
    invalid_argument("`digits` must be NULL or a whole number from 0 to 9.")
  }
  .Call(zufast_format_datetime, as.double(unclass(x)), as.integer(digits))
}

check_character <- function(x) {
  if (!is.character(x)) {
    invalid_argument("`x` must be a character vector.", call = sys.call(-1))
  }
}
