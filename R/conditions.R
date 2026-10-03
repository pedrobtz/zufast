# Classed conditions. Every condition zufast raises inherits `zufast_error`;
# the C layer never raises, so these come from argument checks in R only.

zufast_abort <- function(message, class = character(), call = sys.call(-1)) {
  cnd <- structure(
    list(message = message, call = call),
    class = c(class, "zufast_error", "error", "condition")
  )
  stop(cnd)
}

invalid_argument <- function(message, call = sys.call(-1)) {
  zufast_abort(message, "zufast_invalid_argument", call = call)
}
