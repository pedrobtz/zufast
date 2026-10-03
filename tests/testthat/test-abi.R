# Design 21.2: zufast.so exports R_init_zufast and nothing else.


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
