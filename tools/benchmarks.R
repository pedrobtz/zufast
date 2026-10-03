# Design 22: benchmarks, not run in CI. tools/run-benchmarks installs what is
# missing into a temporary library and runs this from the package root with
# zufast installed. Prints a Markdown table per area for .agents/benchmarks.md.

suppressPackageStartupMessages({
  library(zufast)
  library(bench)
})

# tools/run-benchmarks installs the comparison packages.
for (p in c("RcppFastFloat", "fasttime", "clock", "digest", "base64enc", "jsonlite")) {
  if (!requireNamespace(p, quietly = TRUE)) stop("tools/benchmarks.R needs ", p)
}
set.seed(20261003)
n <- 1e5

report <- function(title, inputs, b) {
  cat("\n### ", title, "\n\n", sep = "")
  cat("| input | expression | median | itr/sec | rows/sec |\n|---|---|---:|---:|---:|\n")
  for (i in seq_len(nrow(b))) {
    sec <- as.numeric(b$median[i]) / 1000   # time_unit = "ms"
    cat(sprintf("| %s | `%s` | %.3f ms | %.0f | %.3g |\n", inputs, as.character(b$expression[i]),
                sec * 1000, b$`itr/sec`[i], inputs_n / sec))
  }
}

mark <- function(...) bench::mark(..., check = FALSE, min_iterations = 5, time_unit = "ms")

cat("# zufast benchmarks\n")
cat("\n", format(Sys.time(), "%Y-%m-%d"), ", R ", R.version.string, ", ",
    Sys.info()[["machine"]], " ", Sys.info()[["sysname"]], ", zufast ", fast_info()$version,
    ", compiler: ", fast_info()$compiler, "\n", sep = "")

# ---- parse doubles -----------------------------------------------------------
inputs_n <- n
sets <- list(
  "small integers" = as.character(sample(0:9999, n, TRUE)),
  "ordinary decimals" = sprintf("%.6f", runif(n, -1000, 1000)),
  "scientific" = sprintf("%.15e", 10^runif(n, -300, 300)),
  "long mantissas" = sprintf("%.25f", runif(n)),
  "subnormals" = sprintf("%.17g", runif(n) * 2^-1030)
)
for (nm in names(sets)) {
  x <- sets[[nm]]
  b <- mark(fast_parse_double(x), as.numeric(x), RcppFastFloat::as.double2(x))
  report(paste("Parse doubles:", nm), nm, b)
}

# ---- format doubles ------------------------------------------------------------
sets <- list(
  random = runif(n, -1e6, 1e6) * 10^sample(-20:20, n, TRUE),
  integral = as.double(sample(-1e9:1e9, n)),
  tiny = runif(n) * 1e-300,
  huge = runif(n) * 1e300
)
for (nm in names(sets)) {
  x <- sets[[nm]]
  # as.character() of a double returns a deferred (ALTREP) vector; nchar()
  # makes every expression materialise its strings.
  b <- mark(nchar(fast_format_double(x)), nchar(sprintf("%.17g", x)), nchar(as.character(x)))
  report(paste("Format doubles:", nm), nm, b)
}

# ---- dates ---------------------------------------------------------------------
days <- sample(-25000:25000, n, TRUE)
secs <- days * 86400 + sample(0:86399, n, TRUE)
iso <- function(t, fmt) format(.POSIXct(t, tz = "UTC"), fmt, tz = "UTC")
d <- iso(secs, "%Y-%m-%d")
dt <- iso(secs, "%Y-%m-%dT%H:%M:%SZ")
dtf <- paste0(iso(secs, "%Y-%m-%dT%H:%M:%S"), ".123456Z")
dto <- paste0(iso(secs, "%Y-%m-%dT%H:%M:%S"), "+01:30")
report("Dates: YYYY-MM-DD", "date", mark(fast_parse_date(d), as.Date(d), clock::date_parse(d)))
b <- mark(
  fast_parse_datetime(dt),
  as.POSIXct(dt, format = "%Y-%m-%dT%H:%M:%SZ", tz = "UTC"),
  fasttime::fastPOSIXct(dt, tz = "UTC"),
  clock::sys_time_parse_RFC_3339(dt)
)
report("Timestamps: ...THH:MM:SSZ", "timestamp", b)
b <- mark(fast_parse_datetime(dtf), fasttime::fastPOSIXct(dtf, tz = "UTC"),
          clock::sys_time_parse_RFC_3339(dtf, precision = "microsecond"))
report("Timestamps with fraction", "timestamp.ffffff", b)
b <- mark(fast_parse_datetime(dto), clock::sys_time_parse_RFC_3339(dto, offset = "%Ez"))
report("Timestamps with offset", "timestamp+hh:mm", b)

# ---- XXH3 ----------------------------------------------------------------------
for (size in c(8, 16, 32, 64, 1024, 2^20)) {
  inputs_n <- 1   # "rows/sec" is calls per second
  x <- as.raw(sample(0:255, size, TRUE))
  b <- mark(fast_hash(x), digest::digest(x, algo = "xxhash64", serialize = FALSE))
  report(sprintf("XXH3, %s bytes", format(size, big.mark = ",")), paste(size, "B"), b)
}

# ---- Base64 --------------------------------------------------------------------
for (size in c(16, 1024, 2^20)) {
  inputs_n <- 1
  x <- as.raw(sample(0:255, size, TRUE))
  b <- mark(fast_base64_encode(x), base64enc::base64encode(x), jsonlite::base64_enc(x))
  report(sprintf("Base64 encode, %s bytes", format(size, big.mark = ",")), paste(size, "B"), b)
}
