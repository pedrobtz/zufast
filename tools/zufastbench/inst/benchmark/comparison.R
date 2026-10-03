# RcppFastFloat's benchmark (its inst/benchmark/comparison.R), with zufast
# added. Same input, same harness: every parser copies each element into a
# std::string first, so the comparison is parser against parser. The *_spans
# rows parse the CHARSXP bytes in place, as a reader would, and measure the
# parsers without the copy.
#
#   Rscript tools/zufastbench/inst/benchmark/comparison.R
#
# with zufast, Rcpp, RcppFastFloat, microbenchmark and zufastbench installed.

library(zufastbench)
library(microbenchmark)

N <- 1e6
set.seed(42)          # does not matter but ensure sample() shuffle fixed
invec <- sample(sqrt(seq(1:N)))
input <- as.character(invec)

outzf <- double(N)
outff <- double(N)
outst <- double(N)
outaf <- double(N)
outsf <- double(N)
outsd <- double(N)
outzs <- double(N)
outfs <- double(N)

res <- microbenchmark(scanf           = sscanf(input, outsf),
                      atof            = atof(input, outaf),
                      strtod          = strtod(input, outst),
                      stod            = stod(input, outsd),
                      fastfloat       = fastfloat(input, outff),
                      zufast          = zufast(input, outzf),
                      fastfloat_spans = fastfloat_spans(input, outfs),
                      zufast_spans    = zufast_spans(input, outzs))

## ensure correct parsing for all
stopifnot(`zf` = all.equal(invec, outzf),
          `ff` = all.equal(invec, outff),
          `st` = all.equal(invec, outst),
          `af` = all.equal(invec, outaf),
          `sf` = all.equal(invec, outsf),
          `sd` = all.equal(invec, outsd),
          `zs` = all.equal(invec, outzs),
          `fs` = all.equal(invec, outfs))

## zufast and fast_float run the same algorithm: bit for bit the same doubles
stopifnot(identical(outzf, outff), identical(outzs, outfs))

print(res)
