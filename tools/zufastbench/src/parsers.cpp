// The parsers compared by inst/benchmark/comparison.R.
//
// The first six follow RcppFastFloat's benchmark harness exactly: each copies
// the element into a std::string and parses that, so the only difference
// between them is the parser. zufast is the one added here.
//
// The *_spans variants parse the CHARSXP bytes in place, with no copy, which
// is how a reader in the zu family calls a parser: they measure the parsers
// rather than the std::string allocation that dominates the harness above.
//
// This file also proves that the zufast headers compile as C++ next to
// fast_float's, which RcppFastFloat compiles as C++11 or later.

#include <Rcpp.h>
#include <fast_float/fast_float.h>
#include <zufast.h>

#include <cstdio>
#include <cstdlib>
#include <string>

// [[Rcpp::export]]
void zufast(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    double d = NA_REAL;
    std::string s(invec[i]);
    zuf_parse_f64(s.data(), s.data() + s.size(), &d);
    outvec[i] = d;
  }
}

// [[Rcpp::export]]
void fastfloat(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    double d = NA_REAL;
    std::string s(invec[i]);
    fast_float::from_chars(s.data(), s.data() + s.size(), d);
    outvec[i] = d;
  }
}

// [[Rcpp::export]]
void strtod(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    std::string s(invec[i]);
    outvec[i] = std::strtod(s.c_str(), NULL);
  }
}

// [[Rcpp::export]]
void atof(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    std::string s(invec[i]);
    outvec[i] = std::atof(s.c_str());
  }
}

// [[Rcpp::export]]
void sscanf(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    std::string s(invec[i]);
    double d = NA_REAL;
    std::sscanf(s.c_str(), "%lf", &d);
    outvec[i] = d;
  }
}

// [[Rcpp::export]]
void stod(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    std::string s(invec[i]);
    outvec[i] = std::stod(s);
  }
}

// [[Rcpp::export]]
void zufast_spans(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    SEXP s = STRING_ELT(invec, i);
    const char *p = CHAR(s);
    double d = NA_REAL;
    zuf_parse_f64(p, p + LENGTH(s), &d);
    outvec[i] = d;
  }
}

// [[Rcpp::export]]
void fastfloat_spans(Rcpp::CharacterVector invec, Rcpp::NumericVector outvec) {
  R_xlen_t n = invec.size();
  for (R_xlen_t i = 0; i < n; i++) {
    SEXP s = STRING_ELT(invec, i);
    const char *p = CHAR(s);
    double d = NA_REAL;
    fast_float::from_chars(p, p + LENGTH(s), d);
    outvec[i] = d;
  }
}
