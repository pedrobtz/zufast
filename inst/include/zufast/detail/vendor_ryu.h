/*
 * zufast/detail/vendor_ryu.h -- includes the vendored Ryu (d2s, f2s) under
 * zufast's configuration. Internal.
 *
 * Ryu's identifiers are generic (mulShift, to_chars, uint128_t,
 * DOUBLE_BIAS, ...) and its double and float units define functions of the
 * same name with different types. Every identifier is therefore renamed to
 * zuf_int_ryu_* (zuf_int_ryu_d_* and zuf_int_ryu_f_* for the five that the
 * two units both define) while the units are included, and every macro
 * they define, include guards too, is removed afterwards, so that nothing
 * of Ryu's namespace reaches a consumer's translation unit.
 *
 * The lists below were produced by scanning the vendored files for
 * file-scope functions, tables, typedefs and macros; tools/check-headers
 * compiles a probe that would fail on a missed clash.
 */
#ifndef ZUFAST_DETAIL_VENDOR_RYU_H
#define ZUFAST_DETAIL_VENDOR_RYU_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "vendor_config.h"

/* identifiers shared by both units */
#define DIGIT_TABLE zuf_int_ryu_DIGIT_TABLE
#define DOUBLE_POW5_INV_SPLIT zuf_int_ryu_DOUBLE_POW5_INV_SPLIT
#define DOUBLE_POW5_INV_SPLIT2 zuf_int_ryu_DOUBLE_POW5_INV_SPLIT2
#define DOUBLE_POW5_SPLIT zuf_int_ryu_DOUBLE_POW5_SPLIT
#define DOUBLE_POW5_SPLIT2 zuf_int_ryu_DOUBLE_POW5_SPLIT2
#define DOUBLE_POW5_TABLE zuf_int_ryu_DOUBLE_POW5_TABLE
#define FLOAT_POW5_INV_SPLIT zuf_int_ryu_FLOAT_POW5_INV_SPLIT
#define FLOAT_POW5_SPLIT zuf_int_ryu_FLOAT_POW5_SPLIT
#define POW5_INV_OFFSETS zuf_int_ryu_POW5_INV_OFFSETS
#define POW5_OFFSETS zuf_int_ryu_POW5_OFFSETS
#define copy_special_str zuf_int_ryu_copy_special_str
#define d2d zuf_int_ryu_d2d
#define d2d_small_int zuf_int_ryu_d2d_small_int
#define d2s zuf_int_ryu_d2s
#define d2s_buffered zuf_int_ryu_d2s_buffered
#define d2s_buffered_n zuf_int_ryu_d2s_buffered_n
#define decimalLength17 zuf_int_ryu_decimalLength17
#define decimalLength9 zuf_int_ryu_decimalLength9
#define div10 zuf_int_ryu_div10
#define div100 zuf_int_ryu_div100
#define div1e8 zuf_int_ryu_div1e8
#define div1e9 zuf_int_ryu_div1e9
#define div5 zuf_int_ryu_div5
#define double_computeInvPow5 zuf_int_ryu_double_computeInvPow5
#define double_computePow5 zuf_int_ryu_double_computePow5
#define double_to_bits zuf_int_ryu_double_to_bits
#define f2d zuf_int_ryu_f2d
#define f2s zuf_int_ryu_f2s
#define f2s_buffered zuf_int_ryu_f2s_buffered
#define f2s_buffered_n zuf_int_ryu_f2s_buffered_n
#define float_to_bits zuf_int_ryu_float_to_bits
#define floating_decimal_32 zuf_int_ryu_floating_decimal_32
#define floating_decimal_64 zuf_int_ryu_floating_decimal_64
#define log10Pow2 zuf_int_ryu_log10Pow2
#define log10Pow5 zuf_int_ryu_log10Pow5
#define mod1e9 zuf_int_ryu_mod1e9
#define mulPow5InvDivPow2 zuf_int_ryu_mulPow5InvDivPow2
#define mulPow5divPow2 zuf_int_ryu_mulPow5divPow2
#define mulShiftAll zuf_int_ryu_mulShiftAll
#define pow5bits zuf_int_ryu_pow5bits
#define shiftright128 zuf_int_ryu_shiftright128
#define uint128_t zuf_int_ryu_uint128_t
#define umul128 zuf_int_ryu_umul128
#define umulh zuf_int_ryu_umulh

ZUF_INT_VENDOR_BEGIN

#define mulShift zuf_int_ryu_d_mulShift
#define multipleOfPowerOf2 zuf_int_ryu_d_multipleOfPowerOf2
#define multipleOfPowerOf5 zuf_int_ryu_d_multipleOfPowerOf5
#define pow5Factor zuf_int_ryu_d_pow5Factor
#define to_chars zuf_int_ryu_d_to_chars
#include "../vendor/ryu/d2s_impl.h"
#undef mulShift
#undef multipleOfPowerOf2
#undef multipleOfPowerOf5
#undef pow5Factor
#undef to_chars

#define mulShift zuf_int_ryu_f_mulShift
#define multipleOfPowerOf2 zuf_int_ryu_f_multipleOfPowerOf2
#define multipleOfPowerOf5 zuf_int_ryu_f_multipleOfPowerOf5
#define pow5Factor zuf_int_ryu_f_pow5Factor
#define to_chars zuf_int_ryu_f_to_chars
#include "../vendor/ryu/f2s_impl.h"
#undef mulShift
#undef multipleOfPowerOf2
#undef multipleOfPowerOf5
#undef pow5Factor
#undef to_chars

ZUF_INT_VENDOR_END

/* Ryu's own macros, include guards included */
#undef DOUBLE_BIAS
#undef DOUBLE_EXPONENT_BITS
#undef DOUBLE_MANTISSA_BITS
#undef DOUBLE_POW5_BITCOUNT
#undef DOUBLE_POW5_INV_BITCOUNT
#undef FLOAT_BIAS
#undef FLOAT_EXPONENT_BITS
#undef FLOAT_MANTISSA_BITS
#undef FLOAT_POW5_BITCOUNT
#undef FLOAT_POW5_INV_BITCOUNT
#undef HAS_64_BIT_INTRINSICS
#undef HAS_UINT128
#undef POW5_TABLE_SIZE
#undef RYU_32_BIT_PLATFORM
#undef RYU_COMMON_H
#undef RYU_D2S_FULL_TABLE_H
#undef RYU_D2S_H
#undef RYU_D2S_INTRINSICS_H
#undef RYU_DIGIT_TABLE_H
#undef RYU_H

/* the renames; zufast code uses the zuf_int_ryu_ names directly */
#undef DIGIT_TABLE
#undef DOUBLE_POW5_INV_SPLIT
#undef DOUBLE_POW5_INV_SPLIT2
#undef DOUBLE_POW5_SPLIT
#undef DOUBLE_POW5_SPLIT2
#undef DOUBLE_POW5_TABLE
#undef FLOAT_POW5_INV_SPLIT
#undef FLOAT_POW5_SPLIT
#undef POW5_INV_OFFSETS
#undef POW5_OFFSETS
#undef copy_special_str
#undef d2d
#undef d2d_small_int
#undef d2s
#undef d2s_buffered
#undef d2s_buffered_n
#undef decimalLength17
#undef decimalLength9
#undef div10
#undef div100
#undef div1e8
#undef div1e9
#undef div5
#undef double_computeInvPow5
#undef double_computePow5
#undef double_to_bits
#undef f2d
#undef f2s
#undef f2s_buffered
#undef f2s_buffered_n
#undef float_to_bits
#undef floating_decimal_32
#undef floating_decimal_64
#undef log10Pow2
#undef log10Pow5
#undef mod1e9
#undef mulPow5InvDivPow2
#undef mulPow5divPow2
#undef mulShiftAll
#undef pow5bits
#undef shiftright128
#undef uint128_t
#undef umul128
#undef umulh
#undef RYU_ASSERT

#endif /* ZUFAST_DETAIL_VENDOR_RYU_H */
