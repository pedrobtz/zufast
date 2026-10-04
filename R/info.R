#' Information about the compiled zufast headers
#'
#' Reports, from compiled code, the version of the zufast headers the
#' package's own shared object was built with, the versions of the vendored
#' libraries compiled into it, and the compiler and build flags that built
#' it.
#' Consumers that use `LinkingTo: zufast` carry their own copy of the code;
#' this describes zufast's copy only.
#'
#' @return A named list with elements `version` (a string such as `"0.1.0"`),
#'   `version_major`, `version_minor`, `version_patch` (integers),
#'   `compiler` (a string), `build` (a named character vector: `c_standard`,
#'   the value of `__STDC_VERSION__`; `optimized`, `ndebug` and `int128`,
#'   each `"true"` or `"false"`; `fortify_source`, the `_FORTIFY_SOURCE`
#'   level or `"0"`; `endian`; and `simd`, the widest x86 or Arm vector
#'   extension the compiler targeted) and `vendored` (a named character
#'   vector of upstream versions).
#' @export
#' @examples
#' fast_info()$version
fast_info <- function() {
  .Call(zufast_info)
}
