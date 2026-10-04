// Differential: zufast's parsers (ffc.h, a C port of fast_float) against
// fast_float itself (tools/fuzz/fast_float, pinned), under every grammar
// option and base the first byte selects. Status, value and -- on accepted
// input -- the end pointer must agree (on invalid input zufast returns
// `first` by contract, where fast_float may point past skipped space). Any divergence is a porting bug -- or an upstream fix ffc lacks,
// which is how the u64 overflow of ffc patch 0003 was missed.
#include <cstring>
#include <system_error>

#include "fast_float/fast_float.h"

extern "C" {
#include "fuzz.h"
}

namespace {

zuf_status status_of(std::errc ec)
{
    if (ec == std::errc()) return ZUF_OK;
    if (ec == std::errc::result_out_of_range) return ZUF_ERR_RANGE;
    return ZUF_ERR_INVALID;
}

template <typename T>
bool same_bits(T a, T b)
{
    return std::memcmp(&a, &b, sizeof a) == 0 || (a != a && b != b);
}

// An intended divergence (ffc eeb3aa5): in JSON mode an exponent
// marker must be followed by digits (RFC 8259), so ffc rejects "1e" and
// "1.5e+" where fast_float accepts the number before the 'e'.
bool json_dangling_exponent(zuf_result r, std::errc ec, const char *ptr, const char *e,
                            const fast_float::parse_options &o)
{
    return r.status == ZUF_ERR_INVALID && ec != std::errc::invalid_argument &&
           (uint64_t(o.format) & uint64_t(fast_float::chars_format::json)) ==
               uint64_t(fast_float::chars_format::json) &&
           ptr < e && (*ptr == 'e' || *ptr == 'E');
}

// The first byte the grammar looks at: past leading space when allowed.
const char *skip_space(const char *s, const char *e, uint32_t flags)
{
    if (flags & ZUF_NUM_SKIP_SPACE)
        while (s < e && (*s == ' ' || (*s >= '\t' && *s <= '\r'))) s++;
    return s;
}

// Intended divergence (#21): zufast's grammar has no "nan(...)" payload, so
// it ends the number after "nan" where fast_float consumes the payload.
// Both report OK and NaN.
bool nan_payload(zuf_result r, std::errc ec, const char *ptr, const char *s, const char *e,
                 uint32_t flags)
{
    const char *p = skip_space(s, e, flags);
    if (p < e && (*p == '-' || *p == '+')) p++;
    return r.status == ZUF_OK && ec == std::errc() && r.ptr == p + 3 && ptr > r.ptr &&
           *r.ptr == '(' && (*p == 'n' || *p == 'N');
}

// Floating point: on OK and on RANGE both write the value (+-inf or +-0).
template <typename T>
void check_float(zuf_result r, T got, const char *s, const char *e,
                 const fast_float::parse_options &o, uint32_t flags)
{
    T want = 0;
    auto f = fast_float::from_chars_advanced(s, e, want, o);
    if (json_dangling_exponent(r, f.ec, f.ptr, e, o)) return;
    if (nan_payload(r, f.ec, f.ptr, s, e, flags)) { FUZZ_CHECK(got != got); return; }
    FUZZ_CHECK(r.status == status_of(f.ec));
    FUZZ_CHECK(r.status == ZUF_ERR_INVALID || r.ptr == f.ptr);
    if (r.status != ZUF_ERR_INVALID) FUZZ_CHECK(same_bits(got, want));
}

// Integers: fast_float leaves the value alone on RANGE; zufast saturates,
// which test-number.R checks.
//
// Intended divergence (#21): fast_float does not apply its JSON grammar to
// integers, and zufast does: under ZUF_NUM_JSON an integer is decimal, has
// no '+' and no leading zero, and anything else is INVALID at `first`.
template <typename T>
void check_int(zuf_result r, T got, const char *s, const char *e,
               const fast_float::parse_options &o, uint32_t flags, int base)
{
    T want = 0;
    if (flags & ZUF_NUM_JSON) {
        const char *p = skip_space(s, e, flags);
        bool plus = p < e && *p == '+';
        if (p < e && *p == '-') p++;
        bool leading_zero = p + 1 < e && p[0] == '0' && p[1] >= '0' && p[1] <= '9';
        if (base != 10 || plus || leading_zero) {
            FUZZ_CHECK(r.status == ZUF_ERR_INVALID && r.ptr == s);
            return;
        }
    }
    auto f = fast_float::from_chars_advanced(s, e, want, o);
    FUZZ_CHECK(r.status == status_of(f.ec));
    FUZZ_CHECK(r.status == ZUF_ERR_INVALID || r.ptr == f.ptr);
    if (r.status == ZUF_OK) FUZZ_CHECK(got == want);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FUZZ_CANARY(data, size);
    if (size < 1) return 0;
    zuf_num_options opt;
    opt.flags = data[0] & 7u;
    opt.base = 2 + data[0] % 35;
    opt.decimal_point = (data[0] & 0x80) ? ',' : 0;
    const char *s = (const char *)data + 1, *e = (const char *)data + size;

    fast_float::chars_format fmt = (opt.flags & ZUF_NUM_JSON) ? fast_float::chars_format::json
                                                             : fast_float::chars_format::general;
    if (opt.flags & ZUF_NUM_LEADING_PLUS) fmt |= fast_float::chars_format::allow_leading_plus;
    if (opt.flags & ZUF_NUM_SKIP_SPACE) fmt |= fast_float::chars_format::skip_white_space;
    fast_float::parse_options fo(fmt, opt.decimal_point ? opt.decimal_point : '.');
    fast_float::parse_options io(fmt, '.', opt.base);

    double d = 0;
    float f = 0;
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int32_t i32 = 0;
    uint32_t u32 = 0;
    check_float(zuf_parse_f64_opt(s, e, &d, &opt), d, s, e, fo, opt.flags);
    check_float(zuf_parse_f32_opt(s, e, &f, &opt), f, s, e, fo, opt.flags);
    check_int(zuf_parse_i64_opt(s, e, &i64, &opt), i64, s, e, io, opt.flags, opt.base);
    check_int(zuf_parse_u64_opt(s, e, &u64, &opt), u64, s, e, io, opt.flags, opt.base);
    check_int(zuf_parse_i32_opt(s, e, &i32, &opt), i32, s, e, io, opt.flags, opt.base);
    check_int(zuf_parse_u32_opt(s, e, &u32, &opt), u32, s, e, io, opt.flags, opt.base);
    return 0;
}
