fast_float v8.3.0 (https://github.com/fastfloat/fast_float, commit
b0ab987b3dfdde13fa1915f65ef2a5c068d9208c), include/fast_float/ unmodified,
MIT licence (one of MIT / Apache-2.0 / Boost-1.0).

Used only by tools/fuzz/fuzz_upstream.cpp, the differential fuzz target
that checks zufast's ffc-based parsers against the C++ library ffc was
ported from. Not part of the package (tools/ is in .Rbuildignore).
Update it to fast_float's latest release when vendor-upstream reports a
new ffc or fast_float release.
