/*
 * zufast/detail/number_impl.h -- the wrappers that translate between
 * zufast's number API and ffc.h. Internal: zufast/number.h is the interface.
 */
#ifndef ZUFAST_DETAIL_NUMBER_IMPL_H
#define ZUFAST_DETAIL_NUMBER_IMPL_H

#include "../status.h"
#include "vendor_ffc.h"

#define ZUF_INT_NUM_JSON          1u
#define ZUF_INT_NUM_LEADING_PLUS  2u
#define ZUF_INT_NUM_SKIP_SPACE    4u

ZUF_INLINE ffc_parse_options zuf_int_ffc_options(uint32_t flags, char decimal_point)
{
    ffc_parse_options o;
    o.format = (flags & ZUF_INT_NUM_JSON) ? (ffc_format)FFC_PRESET_JSON : (ffc_format)FFC_PRESET_GENERAL;
    if (flags & ZUF_INT_NUM_LEADING_PLUS) o.format |= (ffc_format)FFC_FORMAT_FLAG_ALLOW_LEADING_PLUS;
    if (flags & ZUF_INT_NUM_SKIP_SPACE) o.format |= (ffc_format)FFC_FORMAT_FLAG_SKIP_WHITE_SPACE;
    o.decimal_point = decimal_point ? decimal_point : '.';
    return o;
}

ZUF_INLINE zuf_result zuf_int_from_ffc(const char *first, ffc_result r)
{
    if (r.outcome == FFC_OUTCOME_OK) return zuf_int_result(r.ptr, ZUF_OK);
    if (r.outcome == FFC_OUTCOME_OUT_OF_RANGE) return zuf_int_result(r.ptr, ZUF_ERR_RANGE);
    return zuf_int_result(first, ZUF_ERR_INVALID);
}

/* The first byte after the leading whitespace the flags allow. */
ZUF_INLINE const char *zuf_int_num_skip(const char *first, const char *last, uint32_t flags)
{
    const char *p = first;
    if (flags & ZUF_INT_NUM_SKIP_SPACE)
        while (p < last && (*p == ' ' || (*p >= '\t' && *p <= '\r'))) p++;
    return p;
}

/* ffc accepts a "nan(n-char-sequence)" payload, which zufast's grammar
   excludes: the number ends after "nan", as std::from_chars ends it at a
   byte the grammar does not allow. The value is NaN either way. */
ZUF_INLINE zuf_result zuf_int_float_result(const char *first, const char *last, ffc_result fr,
                                           uint32_t flags)
{
    zuf_result r = zuf_int_from_ffc(first, fr);
    if (r.status == ZUF_OK) {
        const char *p = zuf_int_num_skip(first, last, flags);
        if (p < last && (*p == '-' || *p == '+')) p++;
        if (p < last && (*p == 'n' || *p == 'N') && r.ptr - p > 3) r.ptr = p + 3;
    }
    return r;
}

ZUF_INLINE zuf_result zuf_int_parse_f64(const char *first, const char *last, double *out,
                                        uint32_t flags, char decimal_point)
{
    return zuf_int_float_result(first, last, ffc_from_chars_double_options(
        first, last, out, zuf_int_ffc_options(flags, decimal_point)), flags);
}

ZUF_INLINE zuf_result zuf_int_parse_f32(const char *first, const char *last, float *out,
                                        uint32_t flags, char decimal_point)
{
    return zuf_int_float_result(first, last, ffc_from_chars_float_options(
        first, last, out, zuf_int_ffc_options(flags, decimal_point)), flags);
}

/* Integers. ffc leaves the value unwritten on overflow; zufast writes the
   saturated bound in the direction of the sign that was read. */
ZUF_INLINE bool zuf_int_num_negative(const char *first, const char *last, uint32_t flags)
{
    const char *p = zuf_int_num_skip(first, last, flags);
    return p < last && *p == '-';
}

/* ffc does not apply its JSON preset to integers, so zufast does. Under
   ZUF_NUM_JSON an integer is RFC 8259's: decimal, an optional '-', and no
   leading zero. JSON takes precedence over ZUF_NUM_LEADING_PLUS, as it does
   for floating point; ZUF_NUM_SKIP_SPACE still applies. True when the
   input breaks one of these rules before ffc sees it. */
ZUF_INLINE bool zuf_int_json_int_invalid(const char *first, const char *last, int base)
{
    const char *p = first;
    if (base != 10) return true;
    if (p < last && *p == '-') p++;
    else if (p < last && *p == '+') return true;
    return p + 1 < last && p[0] == '0' && p[1] >= '0' && p[1] <= '9';
}

#define ZUF_INT_DEFINE_PARSE_INT(name, type, ffc_fn, lo, hi)                                    \
    ZUF_INLINE zuf_result name(const char *first, const char *last, type *out, uint32_t flags,  \
                               int base)                                                        \
    {                                                                                           \
        type v = 0;                                                                             \
        zuf_result r;                                                                           \
        if (base == 0) base = 10;                                                               \
        if (base < 2 || base > 36) return zuf_int_result(first, ZUF_ERR_INVALID);               \
        if ((flags & ZUF_INT_NUM_JSON) &&                                                       \
            zuf_int_json_int_invalid(zuf_int_num_skip(first, last, flags), last, base))         \
            return zuf_int_result(first, ZUF_ERR_INVALID);                                      \
        r = zuf_int_from_ffc(first, ffc_fn(first, last, base, &v,                               \
                                          zuf_int_ffc_options(flags, '.')));                    \
        if (r.status == ZUF_OK) *out = v;                                                       \
        else if (r.status == ZUF_ERR_RANGE)                                                     \
            *out = zuf_int_num_negative(first, last, flags) ? (lo) : (hi);                      \
        return r;                                                                               \
    }

ZUF_INT_DEFINE_PARSE_INT(zuf_int_parse_i64, int64_t, ffc_from_chars_i64_options, INT64_MIN, INT64_MAX)
ZUF_INT_DEFINE_PARSE_INT(zuf_int_parse_u64, uint64_t, ffc_from_chars_u64_options, 0, UINT64_MAX)
ZUF_INT_DEFINE_PARSE_INT(zuf_int_parse_i32, int32_t, ffc_from_chars_i32_options, INT32_MIN, INT32_MAX)
ZUF_INT_DEFINE_PARSE_INT(zuf_int_parse_u32, uint32_t, ffc_from_chars_u32_options, 0, UINT32_MAX)

ZUF_INLINE size_t zuf_int_format_fixed(char *dst, size_t cap, double v, int places)
{
    return ffc_format_double_fixed(dst, cap, v, places);
}

#define ZUF_INT_FFC_VERSION_STRING FFC_VERSION_STRING

#endif /* ZUFAST_DETAIL_NUMBER_IMPL_H */
