# Design 21.2: zufast.so exports R_init_zufast and nothing else.

exported_symbols <- function(path) {
  sys <- Sys.info()[["sysname"]]
  if (sys == "Linux") {
    out <- system2("nm", c("-D", "--defined-only", shQuote(path)), stdout = TRUE)
  } else if (sys == "Darwin") {
    out <- system2("nm", c("-gU", shQuote(path)), stdout = TRUE)
  } else {
    return(NULL)
  }
  syms <- sub("^.* ", "", out)
  syms <- sub("^_(R_init)", "\\1", syms)
  # Linker-defined section markers are not code.
  setdiff(syms, c("", "_edata", "_end", "__bss_start", "_init", "_fini",
                  "__end__", "__bss_start__", "_bss_end__", "__bss_end__"))
}

test_that("the shared object exports exactly R_init_zufast", {
  skip_on_cran()
  skip_on_os("windows")
  # covr links the gcov runtime into the shared object, which exports its own
  # symbols.
  skip_if(nzchar(Sys.getenv("R_COVR")), "coverage build")
  skip_if(!nzchar(Sys.which("nm")), "nm not available")
  path <- getLoadedDLLs()[["zufast"]][["path"]]
  syms <- exported_symbols(path)
  skip_if(is.null(syms))
  expect_identical(syms, "R_init_zufast")
})
