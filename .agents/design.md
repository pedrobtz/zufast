# zufast — Design

**Revision 2, 2026-10-03.** Replaces the revision 1 draft, which described a package
called "fastc" with an `fc_` prefix, an unspecified linking model and a release plan
staged from 0.1 to 0.6. Revision 2 adopts the `zu*` family conventions, decides on a
header-only consumption mode, fixes the component choices, and collapses every stage
into **v0.1.0, the first CRAN release**. The reasoning behind each change is in the
decision log (§24).

Sibling design documents this one is written against: `../zukomp/.agents/design-zukomp.md`
(§14 namespacing, §15 downstream linkage, §26 family table), `../zuxml/.agents/zuxml-design.md`
(§15 C-callable registration, §18 vendoring), `../zucrypt/.agents/design.md` (§3 inherited
conventions, §8 native interface). Where this document departs from them it says so.

---

## 1. What zufast is

A header-only library of small, fast, portable C99 primitives for the hot path between
external text or bytes and native typed values, distributed to other R packages through
`LinkingTo: zufast`. The primitives are:

- number parsing and formatting, the C equivalent of `std::from_chars` and `std::to_chars`;
- dates and timestamps in the ISO 8601 / RFC 3339 family;
- booleans and literal tokens;
- UUIDs;
- hexadecimal and Base64;
- XXH3 hashing;
- half-precision floats and endian helpers;
- UTF-8 validation.

zufast is infrastructure. The R functions it exports exist to test, benchmark and
demonstrate the C layer, and to make the package useful on its own; they are not the
architectural focus. The consumers are readers and writers of CSV, JSON, YAML, XML, CBOR,
spreadsheets, HTTP and binary formats, in this family and outside it.

The one-line statement that governs every decision below:

> **Provide small, extremely fast, portable C building blocks for converting external data
> into native representations and back, with no run-time dependency on zufast itself.**

## 2. Scope and non-goals

**In scope for v0.1.0** (every item ships in the first release; there are no later stages):

| Area | Delivered by | Section |
|---|---|---|
| Parse `double`, `float`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`; any base 2–36 for integers; JSON-strict mode | vendored `ffc.h` | §8 |
| Shortest round-trip formatting of `double` and `float` | vendored Ryu `d2s`/`f2s` | §9 |
| Fixed-notation formatting of `double` | `ffc.h` | §9 |
| Integer to decimal text | package-owned | §9 |
| Date and timestamp parsing and formatting, calendar arithmetic | package-owned | §10 |
| Boolean and literal matching, ASCII whitespace trimming | package-owned | §11 |
| UUID parse and format | package-owned | §12 |
| Hex encode and decode | package-owned | §13 |
| Base64, standard and URL alphabets, optional padding, scalar | package-owned | §14 |
| XXH3 64 and 128, one-shot and streaming | vendored `xxhash.h` | §15 |
| float16 and bfloat16 conversion; endian load, store and swap | package-owned | §16 |
| UTF-8 validation and code-point count | package-owned DFA | §17 |

**Non-goals**, permanently:

- time zone databases and named zones (`Europe/Zurich`): that layer belongs to R, the
  system, and `zeitig`;
- hash tables, containers, allocators, logging, HTTP, compression, cryptography;
- UUID generation, which needs randomness and belongs with `zurand`;
- Unicode normalisation, collation, case folding, grapheme segmentation;
- a bit-for-bit reimplementation of R's `R_strtod()` (§8.4);
- a registered function table or a static archive (§4).

**Deferred**, not in v0.1.0 and only added when a consumer asks (§25): varints and zig-zag,
CRC32C, SIMD dispatch, HTTP dates, ISO week and ordinal dates, a JSON number classifier.

The admission test for anything new:

> Does this primitive appear directly in the hot path between serialised or textual data and
> a native typed value, in at least one real consumer?

## 3. Position in the `zu*` family

### 3.1 The family rows

The five-repository table in the siblings' design documents (zukomp §26) is changed in all
five together or not at all, so zufast is not added to it here. These are zufast's cells, to
be merged at the next five-repository change:

| | zufast |
|---|---|
| Role | provider, **header mode** |
| R prefix | `fast_` |
| Info function | `fast_info()` |
| Root condition class | `zufast_error` |
| Public C prefix | `zuf_` / `ZUF_` |
| Registered table | none, by design (§4) |
| Static archive | none, by design (§4) |
| How a consumer links | `LinkingTo: zufast` alone; `#include <zufast.h>` |
| Consumers today | none on release; adoption issues filed (§3.2) |
| Upstream licence installed | `licenses/ffc-LICENSE`, `licenses/ryu-LICENSE`, `licenses/xxhash-LICENSE`, plus the notice carried inside each vendored header |
| Symbols hidden (`$(C_VISIBILITY)`) | yes; `zufast.so` exports `R_init_zufast` and nothing else, audited |
| r-actions pin | commit, with the tag in a trailing comment |
| `Depends: R` | 4.1 |

### 3.2 Consumers, as decided rather than as hoped

No sibling can depend on zufast until zufast is on CRAN, so every adoption is tracked as an
issue in the consumer's repository and gated on the CRAN release. The CRAN order is
therefore: **zufast first**, then whichever consumer next releases.

| Package | What it would replace | zufast primitives | Issue |
|---|---|---|---|
| zuxlsx | libc `strtod()` on every numeric cell (`src/zuxlsx.c`) | `zuf_parse_f64` | [zuxlsx#71](https://github.com/pedrobtz/zuxlsx/issues/71) |
| zuyaml | span-to-buffer copy plus `strtod()` and `errno` in scalar resolution; base-prefixed integers | `zuf_parse_f64`, `zuf_parse_i64_opt`, optionally `zuf_parse_datetime` | [zuyaml#14](https://github.com/pedrobtz/zuyaml/issues/14) |
| zucbor | not yet written; CBOR needs float16, base64url and hex tags, tag 0 timestamps, tag 37 UUIDs, big-endian loads, UTF-8 validation | `zuf_f16_*`, `zuf_base64_*`, `zuf_hex_*`, `zuf_parse_datetime`, `zuf_parse_uuid`, `zuf_load_be*`, `zuf_utf8_valid` | [zucbor#41](https://github.com/pedrobtz/zucbor/issues/41) |
| zuhttp | hand-rolled Base64 for Basic auth and PEM, hex helpers | `zuf_base64_*`, `zuf_hex_*` | [zuhttp#79](https://github.com/pedrobtz/zuhttp/issues/79) |
| zucsv | **not numbers** (§8.4); candidates are UTF-8 validation in pass 1, logical cells, and date columns when they enter scope | `zuf_utf8_valid`, `zuf_parse_bool`, later `zuf_parse_date` | [zucsv#11](https://github.com/pedrobtz/zucsv/issues/11) |
| zujson | nothing: yyjson already parses and writes numbers | — | none |

### 3.3 Relationships to the other providers

- **zeitig** (the `zudate` checkout) implements the Temporal model in Rust over `jiff` and
  exposes no C surface. zufast's date parser serves *readers* that need a byte span turned
  into days or seconds with no allocation; zeitig serves *users* who need zones and
  arithmetic. There is no overlap and no dependency either way.
- **zucrypt** owns cryptographic digests. zufast's XXH3 is not a digest and the
  documentation says so wherever it is mentioned.
- **zurand** owns randomness. zufast parses and formats UUIDs; it never generates one.
- **zukomp**'s `zu_` prefix is reserved family-wide (zukomp §14, decision 16). zufast uses
  `zuf_` everywhere, internally too.

## 4. Consumption: header-only, `LinkingTo` alone

### 4.1 The decision

zufast is consumed by including a header. A consumer declares

```
LinkingTo: zufast
```

and nothing else: no `Imports:`, no `importFrom()`, no `configure`, no `PKG_LIBS`. zufast
need not be installed at run time, only at build time. This is a **third consumption mode**
in the family, beside the registered table and the static archive, and it is the simplest of
the three.

### 4.2 Why not the siblings' modes

The registered table (zuxml §15, zukomp §15) routes every call through a function pointer
fetched with `R_GetCCallable()`. For an XML parse or a DEFLATE stream that cost is invisible.
For writing a 64-bit integer, comparing a cell against `NA`, or loading a big-endian
`uint32_t`, the indirect call is the whole cost, and it forbids inlining, which is where the
speed of these primitives comes from. The table also requires `Imports:` and an import
directive, which makes zufast a run-time dependency of every reader in the family.

The static archive (zukomp §15 *Two consumption modes*) needs a `configure` script in every
consumer to find `lib${R_ARCH}/libzufast.a`, because `LinkingTo` has no library equivalent.
zuxlsx carries one today for three archives. The archive exists in the siblings for C
libraries written against Expat or miniz that cannot be retargeted; zufast has no such
library.

zukomp §15 rejected compiled-in code because it "defeats centralized security updates".
That argument is about codecs that decode hostile streams. zufast's primitives allocate
nothing, hold no state and parse bounded input; a bug in them is a wrong value, not a
memory-safety event reachable from a network. The family already accepts the "fix reaches
the consumer on rebuild" model for every archive consumer (zukomp §26), and it is what
header-only means here.

### 4.3 What a consumer sees

```c
#include <zufast.h>          /* everything */
#include <zufast/number.h>   /* or one area at a time */

double x;
zuf_result r = zuf_parse_f64(first, last, &x);
if (r.status != ZUF_OK || r.ptr != last) { /* not a number, or not all of it */ }
```

Every function is `static inline`. Each translation unit that includes a zufast header gets
its own copy of what it uses and nothing of what it does not; the compiler discards the
rest. There is no `ZUF_IMPL` macro to define, no "exactly one translation unit" rule, and
nothing to link. This is the `XXH_INLINE_ALL` model, chosen over the stb-style
implementation macro because a `static` function declared in one unit and defined in another
is a warning in every unit that calls it, which makes the macro model unusable across the
several translation units a real consumer has.

The cost is compile time: a translation unit that includes `<zufast.h>` compiles roughly
twelve thousand lines of C. A consumer that cares includes only the area headers it needs.

### 4.4 The contract: source compatibility, not ABI

Because nothing crosses a shared-object boundary, there is no ABI. No `struct_size`, no
versioned callable name, no `ZUF_API_HAS()`. The promise, within major version 1:

- a function, type, macro or enumerator that exists in a release exists in every later
  release with the same meaning;
- enumerator values are permanent;
- a consumer built against an older zufast keeps working unchanged, because it carries its
  own copy of the code;
- a consumer that rebuilds against a newer zufast compiles without change.

`ZUFAST_VERSION_MAJOR`, `ZUFAST_VERSION_MINOR`, `ZUFAST_VERSION_PATCH` and the string
`ZUFAST_VERSION` let a consumer assert the version it is compiled against. The real gate on a
header change is CRAN's reverse-dependency check of every `LinkingTo` consumer, and the
`consumer.yaml` workflow that builds each family consumer at `@main` (§21.6).

### 4.5 Linkage rules that make this safe

1. **Every emitted symbol is `static`.** A consumer's shared object must contain no global
   zufast or vendor symbol. Two consumers loaded with `dyn.load(local = FALSE)` would
   otherwise bind one package's call to the other's copy, the failure zukomp §14 describes
   for miniz. The vendored libraries are configured for this: §18.
2. **`static inline`, never plain `static`,** for anything defined in a header, so that a
   unit which includes a header without calling every function is not warned at under
   `-Wall -Wextra -Werror` (the zukomp-r.h rule).
3. **Consumers are told to compile with `PKG_CFLAGS = $(C_VISIBILITY)`** as the family does.
   It is defence in depth, not load-bearing: rule 1 is what prevents the collision.
4. **The headers reference nothing R CMD check forbids in compiled code**: no `printf`,
   `abort`, `exit`, `rand`, `srand`, `stdout`, `stderr`, `puts`. A violation would surface
   as a NOTE in every consumer, not in zufast, so it is audited here (§21.3). ffc.h's only
   occurrences are in comments and behind `FFC_DEBUG`, which is never defined.

## 5. Header layout

```
inst/include/
├── zufast.h                 umbrella: includes every area header below
└── zufast/
    ├── version.h            ZUFAST_VERSION_* macros
    ├── status.h             zuf_status, zuf_result, zuf_status_string()
    ├── number.h             §8 parsing and §9 formatting
    ├── datetime.h           §10
    ├── literal.h            §11 bool, literal match, whitespace
    ├── uuid.h               §12
    ├── hex.h                §13
    ├── base64.h             §14
    ├── hash.h               §15
    ├── bits.h               §16 float16, bfloat16, endian
    ├── utf8.h               §17
    ├── detail/
    │   ├── portability.h    compiler and platform macros: ZUF_INLINE, ZUF_LIKELY,
    │   │                    ZUF_ALIGNED, ZUF_STATIC_ASSERT, 128-bit multiply
    │   ├── digits.h         two-digit table shared by number and datetime formatting
    │   └── vendor_config.h  the defines that configure ffc.h, xxhash.h and Ryu (§18)
    └── vendor/
        ├── ffc.h
        ├── xxhash.h
        ├── ryu/             d2s.h, f2s.h, common.h, digit_table.h, *_intrinsics.h,
        │                    *_full_table.h  (§18.3: the two .c files become headers)
        └── LICENSES/        verbatim upstream licence texts
```

Rules:

- Every header compiles standalone as C99 and as C++11 against only `<stddef.h>`,
  `<stdint.h>`, `<stdbool.h>` and `<string.h>`, under `-Wall -Wextra -Wpedantic -Werror`.
  zufast's own headers include nothing else. The one exception is inside the vendored Ryu
  units, which include `<assert.h>`, `<stdlib.h>`, `<limits.h>`, `<inttypes.h>` and
  `<stdio.h>` themselves; patching them out would widen the vendor patch for no gain, since
  every hosted C implementation has them and `RYU_ASSERT` is defined away.
  `-Wpedantic` is relaxed only inside the vendored headers, by wrapping their inclusion in
  `#pragma GCC diagnostic` guards in `vendor_config.h`.
- No R header, no `SEXP`, anywhere under `inst/include/`. The R layer lives in `src/`.
- No vendor type appears in a public declaration. A consumer may call `zuf_hash64()`
  without knowing xxHash exists. The vendored headers are reachable through
  `<zufast/vendor/...>` and a consumer that includes one directly is on its own; nothing
  stops it, and nothing promises it anything.
- `#include <zufast.h>` and `#include "zufast.h"` both work because R puts `<pkg>/include`
  on `CLINK_CPPFLAGS` for every `LinkingTo` package.

## 6. Naming

| Layer | Prefix | Examples |
|---|---|---|
| R exports | `fast_` | `fast_parse_double()`, `fast_hash()`, `fast_info()` |
| Public C (installed headers) | `zuf_` / `ZUF_` | `zuf_parse_f64()`, `zuf_status`, `ZUF_OK`, `ZUF_B64_URL` |
| Internal helpers inside the headers | `zuf_int_` / `ZUF_INT_` | `zuf_int_parse_two_digits()` |
| Entry points and registration | `zufast_` | `R_init_zufast`, `zufast_parse_double` (`.Call`) |
| Test-only `.Call` symbols | `zufast_test_` | `zufast_test_format_bound()` |
| R condition classes | `zufast_` | `zufast_error`, `zufast_invalid_argument` |

`zuf_int_` symbols are in the installed headers and therefore visible to consumers, but
outside the promise of §4.4. The prefix is checked to be unique in the family (2026-10-03:
no sibling uses `zuf_` or `ZUF_`). The vendored libraries keep their own prefixes (`ffc_`,
`XXH`, `ryu` internals), all `static` (§18).

**Why `fast_` for R.** The family's R prefix is a short word naming the domain (`komp_`,
`xml_`, `crypt_`, `rng_`). `fast_` reads as the domain of this package, pairs naturally
with the verbs the API has (`fast_parse_*`, `fast_format_*`), and collides with nothing on
CRAN of consequence. Alternatives considered: `zf_` (opaque), `chars_` (fits numbers only).

## 7. Result model

```c
typedef enum {
    ZUF_OK = 0,
    ZUF_ERR_INVALID,     /* the input is not a value of the requested kind */
    ZUF_ERR_RANGE,       /* syntactically valid, not representable in the target type */
    ZUF_ERR_INCOMPLETE,  /* the input ends inside a value */
    ZUF_ERR_NO_SPACE     /* the output buffer is too small */
} zuf_status;

typedef struct {
    const char *ptr;     /* where parsing stopped */
    zuf_status  status;
} zuf_result;

static inline const char *zuf_status_string(zuf_status s);
```

- `ZUF_OK` is 0 and no value is negative, so `if (r.status)` means "not success", as in
  every sibling.
- `zuf_status_string()` covers every enumerator and never returns `NULL`, including for a
  value outside the enum; a test asserts both.
- **`ptr` semantics follow `std::from_chars`.** On `ZUF_OK` and `ZUF_ERR_RANGE` it points
  one past the last byte consumed; on `ZUF_ERR_INVALID` it equals `first`. A parser never
  requires that the whole span be consumed; a reader that needs "the entire cell is a
  number" checks `r.ptr == last`. This is the one idiom every consumer has to learn and it
  is stated at the top of every parsing header.
- `ZUF_ERR_RANGE` still writes a value: ±infinity or ±0 for floating point, as `fast_float`
  does; the saturated bound for integers. A consumer that wants "nearest representable"
  on overflow, as zuyaml does, takes the value and ignores the status.
- Formatters return `size_t`, never a `zuf_result` (§9.4).

## 8. Numbers: parsing

### 8.1 API

```c
zuf_result zuf_parse_f64(const char *first, const char *last, double   *out);
zuf_result zuf_parse_f32(const char *first, const char *last, float    *out);
zuf_result zuf_parse_i64(const char *first, const char *last, int64_t  *out);
zuf_result zuf_parse_u64(const char *first, const char *last, uint64_t *out);
zuf_result zuf_parse_i32(const char *first, const char *last, int32_t  *out);
zuf_result zuf_parse_u32(const char *first, const char *last, uint32_t *out);

typedef struct {
    uint32_t flags;          /* ZUF_NUM_* below; 0 is the default */
    int      base;           /* integers only: 2..36; 0 means 10 */
    char     decimal_point;  /* floating point only: 0 means '.' */
} zuf_num_options;

#define ZUF_NUM_JSON          1u  /* JSON grammar: no leading '+', no leading zeros, no "inf"/"nan" */
#define ZUF_NUM_LEADING_PLUS  2u  /* accept a leading '+' */
#define ZUF_NUM_SKIP_SPACE    4u  /* skip leading ASCII whitespace */

zuf_result zuf_parse_f64_opt(const char *first, const char *last, double  *out, const zuf_num_options *opt);
zuf_result zuf_parse_f32_opt(...);
zuf_result zuf_parse_i64_opt(const char *first, const char *last, int64_t *out, const zuf_num_options *opt);
zuf_result zuf_parse_u64_opt(...); zuf_parse_i32_opt(...); zuf_parse_u32_opt(...);
```

The plain functions are the `opt` functions with a zero options struct.

### 8.2 Semantics (the default grammar)

The grammar is `fast_float`'s `chars_format::general`: an optional `-`, digits with an
optional `.` and fraction, an optional exponent `e`/`E` with sign and digits; `nan`, `inf`
and `infinity` case-insensitively, with sign. No leading `+`, no leading whitespace, no hex
floats, no `nan(...)` payloads, no thousands separators, no trailing garbage consumed. For
integers: optional `-` on signed types, digits in the given base, no prefix (`0x` is the
consumer's to strip). Overflow is `ZUF_ERR_RANGE`.

Results are **correctly rounded**: the nearest `double` to the decimal value, ties to even,
on every platform, independent of `long double` width and of the FPU rounding mode.

### 8.3 Implementation

`ffc.h` (§18.1), a faithful C99 port of `fast_float`: Eisel–Lemire for the fast path and a
big-integer fallback for the rest, exactly the algorithm in GCC's `std::from_chars`. The
wrappers map `ffc_outcome` to `zuf_status` and `ffc_result.ptr` to `zuf_result.ptr`, and
translate `zuf_num_options` to `ffc_parse_options`. No zufast code touches the digits.

### 8.4 What this is not: `as.numeric()`

`as.numeric()` accumulates digits in `long double` and scales by a power of ten; it is not
correctly rounded and its result depends on whether R was built with `long double`.
`zucsv` chose, in its design decision 5, to match `as.numeric()` bit for bit, and decision 15
is a verified transcription of `R_strtod5()` that does so. zufast makes the other choice,
which is what `vroom`, `fread`'s competitors outside R, and every language's standard
library make. The two differ on roughly one in five thousand ordinary decimals, by one
unit in the last place.

Consequences, stated so nobody rediscovers them:

- **zucsv will not adopt `zuf_parse_f64`** for doubles. Its evaluation issue (§3.2) is for
  other primitives.
- The R wrapper `fast_parse_double()` documents that it is correctly rounded and may differ
  from `as.numeric()` in the last bit, with the `0.799012` example from zucsv's design.
- zufast offers no "R-compatible" mode. A consumer that needs `as.numeric()` semantics
  calls `R_strtod()` or copies zucsv's transcription.

### 8.5 Consumers and their grammars

| Consumer | Grammar it needs | How |
|---|---|---|
| zuxlsx | the number in a `<v>` element: decimal or exponent form, nothing else | default |
| zuyaml core schema | decimal int, `0x`/`0o` after the tag strips the prefix, float with `.inf`/`.nan` spelled YAML's way | `base` 16 / 8; YAML's spellings are matched with `zuf_equals()` before falling through to the number parser |
| JSON-shaped grammars | RFC 8259 numbers | `ZUF_NUM_JSON` |
| R-facing wrappers | what `as.numeric()` accepts syntactically, minus hex and whitespace | `ZUF_NUM_LEADING_PLUS` plus a trim (§11) |

## 9. Numbers: formatting

### 9.1 Shortest round-trip

```c
#define ZUF_F64_MAX_CHARS 25   /* "-0.000001234567890123456": 17 digits behind five zeros */
#define ZUF_F32_MAX_CHARS 24   /* "-100000000000000000000.0": integral below 1e21, trailing zero */

size_t zuf_format_f64(char *dst, size_t cap, double v);
size_t zuf_format_f32(char *dst, size_t cap, float  v);

#define ZUF_FMT_SCIENTIFIC     1u  /* always d.ddde±x */
#define ZUF_FMT_TRAILING_ZERO  2u  /* "1.0", not "1", for integral values in positional form */

size_t zuf_format_f64_opt(char *dst, size_t cap, double v, uint32_t flags);
size_t zuf_format_f32_opt(char *dst, size_t cap, float  v, uint32_t flags);

/* The digits and exponent themselves, for a consumer that formats its own way. */
typedef struct { uint64_t mantissa; int32_t exponent; bool negative; } zuf_decimal;
zuf_decimal zuf_decimal_f64(double v);
zuf_decimal zuf_decimal_f32(float  v);
```

The digits come from Ryu (§18.3) and are the shortest that round-trip, with ties resolved as
Ryu resolves them. The **notation rule is ECMAScript's `Number::toString`**: positional
without a decimal point for integral values below 10^21, positional with a point when the
decimal exponent is in (−7, 21), otherwise `d.ddde±x` with a lower-case `e`, an explicit
sign and no leading zeros in the exponent. So `1` is `1`, `0.1` is `0.1`, `1e21` is
`1e+21`, `1e-7` is `1e-7`, and `0.30000000000000004` is exactly that. The rule is stated
because Ryu's own output (`1E0`) is not what any reader or writer in the family wants, and
because R's `as.character()` prints fifteen significant digits, which is a different thing
(§20).

Non-finite values are written with R's spellings, `NaN`, `Inf`, `-Inf`. A JSON writer
handles non-finite values before calling, as it must anyway. Negative zero is `-0`, so that
the round-trip property holds bit for bit.

The capacities were first drafted as 24 and 16 from the exponential form alone; the
positional forms the notation rule allows are longer (`-0.000001234567890123456` is 25 bytes,
and a float just below 1e21 with `ZUF_FMT_TRAILING_ZERO` is 24), so the macros are 25 and 24.

Ryu's identifiers are generic (`mulShift`, `to_chars`, `uint128_t`, `DOUBLE_BIAS`) and its
double and float units define functions of the same names with different types, so
`detail/vendor_ryu.h` renames every one of them to `zuf_int_ryu_*` while including the two
units and removes Ryu's macros afterwards; nothing of Ryu's namespace reaches a consumer.

### 9.2 Fixed notation

```c
size_t zuf_format_f64_fixed(char *dst, size_t cap, double v, int places);
```

Exactly `printf("%.*f")` on a libc that prints the exact binary value, every digit exact,
ties to even, no floating-point arithmetic, identical on every platform. This is
`ffc_format_double_fixed()` and the reason Ryu's `d2fixed`, with its large tables, is not
vendored. A buffer of `311 + places` bytes always suffices.

### 9.3 Integers

```c
#define ZUF_U64_MAX_CHARS 20
#define ZUF_I64_MAX_CHARS 20   /* "-9223372036854775808" */
#define ZUF_U32_MAX_CHARS 10
#define ZUF_I32_MAX_CHARS 11

char *zuf_write_u64(char *dst, uint64_t v);   /* writes, returns one past the end */
char *zuf_write_i64(char *dst, int64_t  v);
char *zuf_write_u32(char *dst, uint32_t v);
char *zuf_write_i32(char *dst, int32_t  v);
```

Package-owned, two digits per step through the shared `detail/digits.h` table. The `write`
family is the unchecked fast path for a caller that has reserved `ZUF_*_MAX_CHARS`; it is
the shape a serialiser's inner loop wants, and it is why these do not take a capacity.

### 9.4 Formatter conventions

Every `zuf_format_*` function returns the full length the value needs, writes at most `cap`
bytes, and never NUL-terminates. `cap == 0` measures. A caller that passed too small a
buffer sees a return value greater than `cap` and nothing is corrupted. The `ZUF_*_MAX_CHARS`
macros are the capacities that always suffice, so the common case needs no check at all.

## 10. Dates and timestamps

### 10.1 What is parsed

The machine-readable subset of ISO 8601 and RFC 3339 that readers actually meet:

```
YYYY-MM-DD
YYYY-MM-DDTHH:MM
YYYY-MM-DDTHH:MM:SS
YYYY-MM-DDTHH:MM:SS.fff          fraction of 1 to 9 digits, further digits validated and discarded
YYYY-MM-DDTHH:MM:SSZ             Z or z
YYYY-MM-DDTHH:MM:SS+01:00        ±HH:MM, ±HHMM, ±HH
YYYY-MM-DD HH:MM:SS...           a single space in place of T, as YAML and SQL write it
YYYY-MM-DDtHH:MM:SS...           lower-case t, as YAML permits
```

Validation is complete: four-digit year 0000–9999, month 1–12, day valid for the month in the
proleptic Gregorian calendar (1900-02-29 invalid, 2000-02-29 valid, 2100-02-29 invalid),
hour 0–23, minute 0–59, second 0–60 (RFC 3339 allows the leap second), offset within
±23:59. `24:00:00`, a comma as the fraction separator, expanded years, week dates, ordinal
dates and the basic format without separators are rejected; the first two are rare and the
rest are deferred (§25).

### 10.2 API

```c
typedef struct {
    int32_t  year;
    uint8_t  month, day;
    uint8_t  hour, minute, second;
    uint32_t nanosecond;
    int32_t  offset_seconds;   /* meaningful only when has_offset */
    bool     has_time;         /* false for a bare date */
    bool     has_offset;       /* Z or a numeric offset was present */
} zuf_datetime;

zuf_result zuf_parse_date(const char *first, const char *last, zuf_datetime *out);     /* YYYY-MM-DD only */
zuf_result zuf_parse_datetime(const char *first, const char *last, zuf_datetime *out); /* every form above */

/* Calendar arithmetic, proleptic Gregorian, days relative to 1970-01-01. */
int32_t zuf_days_from_civil(int32_t year, uint32_t month, uint32_t day);
void    zuf_civil_from_days(int32_t days, int32_t *year, uint32_t *month, uint32_t *day);
bool    zuf_is_leap_year(int32_t year);
uint32_t zuf_days_in_month(int32_t year, uint32_t month);

typedef struct { int64_t seconds; uint32_t nanoseconds; } zuf_timestamp;

int32_t       zuf_datetime_days(const zuf_datetime *dt);           /* the date part */
zuf_timestamp zuf_datetime_timestamp(const zuf_datetime *dt);      /* applies offset_seconds when has_offset;
                                                                      otherwise treats the wall time as UTC */

#define ZUF_DATE_CHARS      10
#define ZUF_DATETIME_MAX_CHARS 42   /* any int32_t year: "-2147483648-MM-DDTHH:MM:SS.nnnnnnnnn+HH:MM" */

size_t zuf_format_date(char *dst, size_t cap, int32_t days);
size_t zuf_format_datetime(char *dst, size_t cap, const zuf_datetime *dt);   /* RFC 3339; Z when offset is 0 */
```

**The parser returns fields, not an epoch.** Revision 1 returned an adjusted epoch, which
discards the offset and cannot distinguish "no offset given" from "UTC". A reader needs
both: zuyaml keeps the offset, zucbor's tag 0 may need to re-emit it, and a CSV column
without offsets must be interpreted in a zone zufast knows nothing about. The two
conversion helpers are explicit about what they assume, and `has_offset` is the consumer's
to honour.

`zuf_format_datetime` writes the fraction with 0, 3, 6 or 9 digits, the shortest group that
represents `nanosecond` exactly, and `Z` for a zero offset. R's representations fall out
directly: `Date` is `zuf_datetime_days()`, `POSIXct` is `zuf_datetime_timestamp().seconds`
plus the nanoseconds as a fraction.

### 10.3 Implementation

Package-owned, the one original algorithmic component:

- fixed-position separator checks, since every form above has its separators at known
  offsets once the length is known;
- `YYYY-MM-` validated and decoded through SWAR on one 8-byte little-endian load: an XOR
  with the template and two masked tests check every digit and separator at once, and the
  four year digits combine in two multiply-and-mask steps. The load is assembled from bytes,
  which GCC and clang compile to a single load on a little-endian target and which is
  correct on any other, so no compile-time fallback is needed. The remaining fixed-width
  fields are checked and decoded in one pass each; the per-byte pattern match runs only on
  the error path, to tell a short input (`ZUF_ERR_INCOMPLETE`) from a malformed one;
- calendar conversion by the Euclidean affine functions of Neri and Schneider (2022), which
  are faster than the days-from-civil formula of revision 1 and are valid far beyond the
  year range accepted;
- no `strptime()`, no `mktime()`, no `time.h`, no allocation, no locale, no global state.

No SIMD in v0.1.0 (§19.3). The scalar path is the baseline every later path must match bit
for bit.

## 11. Boolean and literal matching

```c
#define ZUF_BOOL_LOWER   1u   /* true  false */
#define ZUF_BOOL_UPPER   2u   /* TRUE  FALSE */
#define ZUF_BOOL_TITLE   4u   /* True  False */
#define ZUF_BOOL_LETTER  8u   /* T F t f */
#define ZUF_BOOL_DIGIT  16u   /* 1 0 */
#define ZUF_BOOL_YESNO  32u   /* yes no y n on off, in the three capitalisations above */

#define ZUF_BOOL_R     (ZUF_BOOL_LOWER | ZUF_BOOL_UPPER | ZUF_BOOL_TITLE | ZUF_BOOL_LETTER)  /* as.logical() */
#define ZUF_BOOL_YAML12 (ZUF_BOOL_LOWER | ZUF_BOOL_UPPER | ZUF_BOOL_TITLE)                   /* core schema */

zuf_result zuf_parse_bool(const char *first, const char *last, uint32_t accept, bool *out);

bool zuf_equals   (const char *first, const char *last, const char *lit, size_t lit_len);  /* exact */
bool zuf_equals_ci(const char *first, const char *last, const char *lit, size_t lit_len);  /* ASCII case-insensitive */

const char *zuf_skip_space(const char *first, const char *last);   /* ASCII space, \t \n \v \f \r */
void        zuf_trim_space(const char **first, const char **last);
```

The flags are fine-grained because the consumers disagree: zucsv accepts exactly four
spellings (`ZUF_BOOL_LOWER | ZUF_BOOL_UPPER`), `as.logical()` accepts `ZUF_BOOL_R`, YAML 1.2
accepts `ZUF_BOOL_YAML12`, YAML 1.1 adds `ZUF_BOOL_YESNO`. zufast hard-codes no policy.
Missing-value tokens (`NA`, `N/A`, `NULL`, `null`, `.`) are a policy too, so there is no
`zuf_parse_na()`; a consumer matches its own list with `zuf_equals()`.

## 12. UUID

```c
typedef struct { uint8_t bytes[16]; } zuf_uuid;   /* network order, as RFC 9562 and CBOR tag 37 store it */
#define ZUF_UUID_CHARS 36

zuf_result zuf_parse_uuid (const char *first, const char *last, zuf_uuid *out);
size_t     zuf_format_uuid(char *dst, size_t cap, const zuf_uuid *u, bool upper);
```

Parses `8-4-4-4-12` hexadecimal in either case, and the 32-digit form without hyphens.
Braces, URN prefixes and Microsoft's `{}` form are not accepted. Sixteen bytes rather than
two `uint64_t`, so that no byte-order question arises when the value is stored or compared.

## 13. Hex

```c
size_t     zuf_hex_encode(const void *src, size_t n, char *dst, size_t cap, bool upper);  /* needs 2n */
zuf_status zuf_hex_decode(const char *first, const char *last, void *dst, size_t cap, size_t *out_len);
```

Decoding accepts either case, rejects an odd length (`ZUF_ERR_INVALID`), any non-hex byte,
and a buffer shorter than half the input (`ZUF_ERR_NO_SPACE`, nothing written). Table
driven, package-owned.

Both decoders, hex and Base64, follow one contract: a byte outside the alphabet takes
precedence over every other error, so `ZUF_ERR_INCOMPLETE` and `ZUF_ERR_NO_SPACE` mean the
input is otherwise well formed; `*out_len` is 0 on any error; `ZUF_ERR_NO_SPACE` writes
nothing; and on `ZUF_ERR_INVALID` the output up to the decoded length may have been written.
That last clause lets each decoder read its input once.

## 14. Base64

```c
#define ZUF_B64_URL     1u   /* -_ alphabet (RFC 4648 §5) instead of +/ */
#define ZUF_B64_NO_PAD  2u   /* encode: omit '='; decode: padding optional */

size_t     zuf_base64_encode_bound(size_t n);
size_t     zuf_base64_encode(const void *src, size_t n, char *dst, size_t cap, uint32_t flags);
size_t     zuf_base64_decode_bound(size_t n);
zuf_status zuf_base64_decode(const char *first, const char *last, void *dst, size_t cap,
                             size_t *out_len, uint32_t flags);
```

Decoding is strict: no whitespace, no characters outside the selected alphabet, padding
complete, and non-zero unused trailing bits rejected, so that every accepted input has
exactly one encoding. The one exception is `ZUF_B64_NO_PAD`, under which padding is optional
rather than forbidden, so that a decoder configured for unpadded input still accepts the
canonical form: `"Zg"` and `"Zg=="` both decode. A consumer that must accept MIME line
breaks strips them first. `ZUF_ERR_INCOMPLETE` for a final quantum of one character.

Package-owned, scalar, table-driven, three bytes per step. No SIMD in v0.1.0: aklomp/base64
would bring runtime dispatch, but its codecs need per-file compile flags that a consumer's
portable `Makevars` cannot supply, and a scalar codec already exceeds a gigabyte per second,
which is faster than any reader in the family that would feed it.

## 15. Hashing

```c
typedef struct { uint64_t low, high; } zuf_digest128;   /* not zuf_hash128: that is the function */

uint64_t      zuf_hash64 (const void *data, size_t n);
uint64_t      zuf_hash64_seed (const void *data, size_t n, uint64_t seed);
zuf_digest128 zuf_hash128(const void *data, size_t n);
zuf_digest128 zuf_hash128_seed(const void *data, size_t n, uint64_t seed);

/* Streaming. The state is opaque by size, not by type: a consumer can put it on the stack
   without naming a vendor type. A static assertion holds the size and alignment. */
#define ZUF_HASHER_SIZE  640
typedef union { zuf_int_xxh3_state state; ZUF_ALIGNED(64) unsigned char opaque[ZUF_HASHER_SIZE]; } zuf_hasher;

void        zuf_hasher_init  (zuf_hasher *h, uint64_t seed);
void        zuf_hasher_update(zuf_hasher *h, const void *data, size_t n);
uint64_t    zuf_hasher_digest64 (const zuf_hasher *h);
zuf_digest128 zuf_hasher_digest128(const zuf_hasher *h);
```

XXH3 (§18.2). The results are bit-identical to `XXH3_64bits()` and `XXH3_128bits()` with the
same seed, on every platform and whichever vector path xxHash selected at compile time;
that identity is a test, since it is what makes a zufast hash comparable with one computed
elsewhere.

**XXH3 is not a cryptographic hash.** It is for caches, dictionaries, deduplication,
fingerprints and change detection. Anything that needs collision resistance against an
adversary uses `zucrypt`. Every piece of documentation that names the function says this.

## 16. Half-precision floats and endian helpers

```c
float    zuf_f16_to_f32(uint16_t bits);
uint16_t zuf_f32_to_f16(float v);        /* round to nearest even; overflow to infinity */
float    zuf_bf16_to_f32(uint16_t bits);
uint16_t zuf_f32_to_bf16(float v);
bool     zuf_f32_fits_f16(float v);      /* exact after a round trip: what CBOR's preferred serialisation asks */
bool     zuf_f64_fits_f32(double v);

uint16_t zuf_load_le16(const void *p);  uint32_t zuf_load_le32(const void *p);  uint64_t zuf_load_le64(const void *p);
uint16_t zuf_load_be16(const void *p);  uint32_t zuf_load_be32(const void *p);  uint64_t zuf_load_be64(const void *p);
void zuf_store_le16(void *p, uint16_t v);   /* ... le32, le64, be16, be32, be64 */
uint16_t zuf_bswap16(uint16_t v);  uint32_t zuf_bswap32(uint32_t v);  uint64_t zuf_bswap64(uint64_t v);
```

All through `memcpy` and shifts, so they are correct on any alignment and any byte order;
the compiler turns them into single instructions. Package-owned. The `fits` predicates exist
because CBOR's preferred serialisation (RFC 8949 §4.2.2) encodes a float in the shortest
width that represents it exactly, and that decision should be one call.

## 17. UTF-8 validation

```c
bool   zuf_utf8_valid(const char *data, size_t n);
size_t zuf_utf8_count(const char *data, size_t n, bool *valid);   /* code points, or the count up to the first error */
```

Höhrmann's DFA: one 400-byte state table and a dozen lines, rejecting overlong forms,
surrogates and code points above U+10FFFF, exactly as R's `validUTF8()` does. The table
is transcribed from the author's published decoder under its MIT notice, so it is recorded
in `inst/COPYRIGHTS` and as a `cph` entry in `Authors@R`, although it is not vendored as a
library. No SIMD in v0.1.0; the DFA is the correctness baseline a later path must match.
Normalisation, case folding and grapheme segmentation are out of scope permanently.

## 18. Vendoring

Three upstream libraries, under `inst/include/zufast/vendor/`. The tooling is the zucrypt
generation: `tools/vendor/manifest.tsv` with the columns
`source repo tag commit version_string archive archive_sha256 license defines patches`,
`tools/vendor/checksums.sha256`, and the three scripts `fetch` (network, maintainer only),
`record` and `verify` (offline). `verify` checks the tree against the checksums, the
archive hash against the manifest, the patch set against `tools/patches/<source>/` and
`inst/COPYRIGHTS`, and the define set in `detail/vendor_config.h` against the manifest's
`defines` column. CI runs it through `pedrobtz/r-actions`' `vendor.yml` with `vendor-dir`
pointing at the include tree. A pull request that touches the vendor tree without updating
both the manifest and the checksums fails.

Vendored files are never edited in place. Patches go in `tools/patches/<source>/NNNN-*.patch`,
are applied by `fetch`, and each one is named in the manifest, in `inst/COPYRIGHTS` and in
`cran-comments.md`. Every patch is also submitted upstream so that the set shrinks.

| | ffc.h | Ryu | xxHash |
|---|---|---|---|
| Repository | `kolemannix/ffc.h` | `ulfjack/ryu` | `Cyan4973/xxHash` |
| Pin | tag `v26.09.01` (2026-09-23), release asset `ffc.h` + `ffc.h.sha256` | tag `v2.0` | tag `v0.8.4` |
| Files | `ffc.h` | `ryu/d2s.c`, `f2s.c`, `common.h`, `digit_table.h`, `d2s.h`, `d2s_intrinsics.h`, `d2s_full_table.h`, `ryu.h` (the v2.0 file set) | `xxhash.h` only |
| Licence chosen | MIT, of the MIT / Apache-2.0 / Boost triple | Boost Software License 1.0, of the Apache-2.0 / Boost pair | BSD-2-Clause (the library; the `xxhsum` tool in the same repository is GPL and is not vendored) |
| Installed licence text | `licenses/ffc-LICENSE` | `licenses/ryu-LICENSE` | `licenses/xxhash-LICENSE` |
| Patches | 0001: add `FFC_LINKAGE` and `FFC_LINKAGE_EXTERN` (defaults: empty and `extern`) in front of every public declaration and definition, so zufast can make them all `static`; 0002: drop the `-Wfloat-equal` pragmas, which R CMD check reports; 0003: fix the u64 overflow check at the maximum digit count (a 20-digit value wrapped once passed it), ported from fast_float 8.3 | 0001: `d2s.c` and `f2s.c` become `d2s_impl.h` and `f2s_impl.h`, their entry points `static inline`, their includes relative, `assert` routed through `RYU_ASSERT` | none |
| Configuration | `FFC_IMPL` defined; `FFC_LINKAGE` = `FFC_LINKAGE_EXTERN` = `static`; `FFC_ROUNDS_TO_NEAREST` **not** defined | `RYU_OPTIMIZE_SIZE` not defined (full tables) | `XXH_INLINE_ALL`, `XXH_NO_STDLIB`; the vector path left to xxHash's compile-time choice |

### 18.1 ffc.h

The canonical repository is `kolemannix/ffc.h`; `yuval-herman/ffc.h` is a fork. The header
is used by valkey and hiredis, carries the `fast_float` authors in its notice, and its
`ffc_internal` functions are already `static`. At v26.09.01 the 27 public functions carry no
linkage macro (`FFC_API` is only the guard of the declaration block, and the two
`ffc_from_chars_double*` definitions are `extern FFC_IMPL_INLINE`), so the patch adds
`FFC_LINKAGE` and `FFC_LINKAGE_EXTERN` in front of each declaration and definition; with
both defined as `static`, a probe compiled with `FFC_IMPL` has no global symbol. The few
`ffc_internal` functions that are `static` without `inline` would warn when unused, so
`vendor_config.h` relaxes `-Wunused-function` (and `-Wpedantic`, `-Wmissing-field-initializers`)
inside the vendored header only; GCC and clang apply diagnostic pragmas by location.
`FFC_ROUNDS_TO_NEAREST` is left undefined: it would remove a check that costs a few
instructions, and an R session can have its rounding mode changed by another package.

### 18.2 xxHash

`XXH_INLINE_ALL` makes every function `static` with an `unused` attribute, which is exactly
the §4.5 contract; `XXH_NO_STDLIB` drops the `malloc`-backed state allocators that zufast
does not use. xxHash selects SSE2 on x86-64 and NEON on AArch64 at compile time; both are
baseline instruction sets, no `-m` flag is involved, and the vendored-sources exemption in the
family's C99 rule covers it. `XXH_VECTOR` is not forced, so a consumer's compiler sees the
same choice zufast's own build sees.

### 18.3 Ryu

Ryu is shipped as `.c` files with a tiny public header. The patch turns the two
double-and-float-to-shortest units into headers with `static inline` entry points and keeps
the tables `static const`. The fixed-notation unit `d2fixed.c` and its tables are not
vendored (§9.2). `RYU_OPTIMIZE_SIZE` stays off: the full tables are about ten kilobytes and
the small-table path is measurably slower.

### 18.4 DESCRIPTION and attribution

```
Copyright: See inst/COPYRIGHTS and tools/vendor/manifest.tsv.
```

`Authors@R` carries a `cph` entry per upstream holder with a `comment` naming what they hold:
the `fast_float` authors and Koleman Nix for `ffc.h`, Ulf Adams for Ryu, Yann Collet for
xxHash, Björn Höhrmann for the UTF-8 state table. `inst/COPYRIGHTS` explains that the
installed package carries each library both as a header under `include/zufast/vendor/` and
compiled into `zufast.so`, and that a consumer compiles the same code into its own shared
object, which is a redistribution the licence texts under `licenses/` cover.

## 19. Build, portability and SIMD policy

### 19.1 zufast's own build

```
src/
├── init.c             R_init_zufast: registers .Call entry points, R_useDynamicSymbols(FALSE),
│                      R_forceSymbols(TRUE); no R_RegisterCCallable, there is nothing to register
├── zufast_r.c         the .Call wrappers behind R/ (§20)
├── zufast_test.c      the always-compiled test harness (§21.4)
└── Makevars           PKG_CPPFLAGS = -I../inst/include
                       PKG_CFLAGS   = $(C_VISIBILITY)
                       OBJECTS listed explicitly; portable make only
```

The wrappers include `<zufast.h>` like any consumer. That is what makes CRAN's instrumented
flavours (UBSan, ASan, valgrind, LTO, noLD) exercise the header: the package's own `.so`
contains every primitive, and every test runs through it. `Makevars.win` is identical.

No `configure`. No `install.libs.R`: there is no archive to install, and R's own `inst`
step copies the headers.

### 19.2 Portability

Targets: Linux x86-64 and AArch64, macOS x86-64 and ARM64, Windows x86-64 under Rtools.
Compilers: GCC, clang, Apple clang. MSVC is not a target, because no R toolchain uses it;
the headers compile as C++11 so that cpp11 and Rcpp consumers can include them.

Project code is C99 with `<stdbool.h>`: no C11 atomics, no intrinsics, no assembly, no
threads. The 128-bit multiply in `detail/portability.h` uses `__uint128_t` where the compiler
offers it and a portable 64-bit emulation otherwise, as ffc.h does internally. Byte order is
handled by construction in §16 and by the vendored libraries; no CI leg runs big-endian, and
the design says so rather than claiming coverage.

Every function is pure: no allocation, no `errno`, no locale, no global state, no `time.h`.
Any function may therefore be called from any thread, including OpenMP workers inside a
consumer, with no fetch on the main thread first. That is stated in each header.

### 19.3 SIMD policy

None in v0.1.0, with one exception already covered: xxHash's compile-time baseline paths.
The decision for later is recorded now so that nobody designs for the wrong mechanism:
consumers compile zufast's code with their own flags, and portable `make` cannot give one
object file `-mavx2`, so the only route to an above-baseline path is function-level
`__attribute__((target("avx2")))` with `__builtin_cpu_supports()` dispatch inside the
header, which GCC, clang and Rtools' GCC all support. A SIMD path is admitted only with a
benchmark showing a consumer-visible gain and a scalar path that it matches bit for bit.

### 19.4 CRAN

`R CMD check --as-cran` is 0/0/0 on R-release, R-devel and R-oldrel on all three platforms;
`cran-comments.md` explains any NOTE that remains. No `-W`, `-O` or `-march` flag anywhere.
The headers under `inst/include` are installed verbatim, so the compiled-code symbol audit
of §21.3 is what keeps every consumer NOTE-free.

## 20. R API

Deliberately small, prefixed `fast_`, vectorised, `NA` in means `NA` out, invalid input is
`NA` with no warning unless asked for, as `as.numeric()` behaves. Conditions are classed:
`zufast_invalid_argument` for argument problems, all inheriting `zufast_error`; the C layer
never raises.

```r
fast_parse_double(x)                 # character -> double; correctly rounded (§8.4)
fast_parse_integer(x)                # character -> integer; NA on overflow, no silent truncation
fast_format_double(x, scientific = FALSE, trailing_zero = FALSE)   # shortest round-trip (§9.1)
fast_parse_date(x)                   # "YYYY-MM-DD" -> Date
fast_parse_datetime(x)               # RFC 3339 -> POSIXct in UTC; no offset means UTC, documented
fast_datetime_fields(x)              # the zuf_datetime struct as a data frame, for callers that apply a zone
fast_format_datetime(x, digits = NULL)
fast_hash(x, bits = 64, seed = 0)    # raw or character -> hex string of XXH3
fast_base64_encode(x, url = FALSE, pad = TRUE); fast_base64_decode(x, url = FALSE)
fast_hex_encode(x, upper = FALSE);   fast_hex_decode(x)
fast_utf8_valid(x)
fast_info()                          # vendored versions from compiled code, build flags, ZUFAST_VERSION
```

Documented differences from base R, each with an example in the help page:
`fast_parse_double()` is correctly rounded where `as.numeric()` is not;
`fast_format_double()` prints the shortest round-trip digits where `as.character()` prints
fifteen significant; `fast_parse_integer()` refuses what does not fit rather than returning
`NA` with a warning after a double conversion.

The R layer works on `CHARSXP`s with known lengths and never calls `strlen()`, recognises
`NA_STRING` before touching bytes, and allocates its output once. Nothing more
R-specific is attempted in v0.1.0: every reader in the family hands zufast byte spans, not
`STRSXP`s, so the R vector layer sketched in revision 1 has no consumer.

## 21. Testing and gates

Performance is secondary to bit-exact correctness. Every gate below is wired into CI except
the benchmarks, and a gate counts only once it has been seen to fail (zuxml's rule).

### 21.1 The header

`abi.yaml`, after zukomp's, on Linux and macOS:

- each header under `inst/include/` compiles standalone as C99 under
  `-Wall -Wextra -Wpedantic -Werror`, and as C++11 under `-Wall -Wextra -Werror`;
- a probe translation unit that includes `<zufast.h>` and uses nothing compiles warning-free
  under `-Wall -Wextra -Werror` with GCC and with clang, which is the unused-function check
  of §4.5;
- a probe that calls every public function compiles warning-free the same way;
- a comment-stripped grep of the installed headers finds no `SEXP`, `Rf_`, `R.h`, and no
  `ffc_`, `XXH`, `ryu` identifier outside `vendor/` and `detail/vendor_config.h`.

### 21.2 The shared object

`test-abi.R` reads the installed `zufast.so` and asserts that the export set is exactly
`R_init_zufast`, that no `ffc_`, `XXH` or `ryu` symbol is global, and that `fast_info()`
reports the pinned upstream versions from compiled code (`FFC_VERSION_*`,
`XXH_VERSION_NUMBER`, the Ryu tag recorded in `vendor_config.h`).

### 21.3 The compiled-code audit

`tools/run-symbol-audit` compiles the full-use probe of §21.1 into a shared object with R's
own flags, at R's optimisation level and at `-O0`, and scans it with
`tools:::check_so_symbols()`, the function R CMD check itself runs on compiled code, so the
platform's own table applies (glibc's fortified `__printf_chk`, Darwin's underscores,
`sprintf`, the C RNG). A probe with a planted `printf()` must fail first, so a pass means the
scan looked. The fixture
package of §21.6 covers the same ground through a real `R CMD check`, and its log must show
no "compiled code" NOTE.

### 21.4 Correctness

- **Numbers, parsing.** Differential against `strtod()` in the C locale and against
  `R_strtod()` over random decimals with 1–40 significant digits and exponents in
  [−340, 340], with the expected disagreements with `R_strtod()` classified and counted
  rather than hidden; halfway values, subnormals, overflow, underflow, `-0`, 800-digit
  mantissas, `0e9999`. `tools/run-parse-corpus` runs the `parse-number-fxx-test-data`
  corpus (Nigel Tao, MIT, several million cases with reference bits) on demand and on a
  weekly schedule; it is not part of the test suite because of its size.
- **Numbers, formatting.** The round-trip property `zuf_parse_f64(zuf_format_f64(x)) == x`
  bitwise over random doubles of every exponent and over the classic Ryu test vectors;
  `zuf_format_f64_fixed()` against `sprintf("%.*f")` on glibc, in the Linux CI leg only,
  since other libcs are not exact; integer writers exhaustively at the type boundaries.
- **Dates.** Table tests for every rule in §10.1; differential against `as.Date()` and
  `as.POSIXct(tz = "UTC")` over random dates in 0001–9999 and random timestamps with every
  offset form; the calendar functions against each other and against R over every day of
  four centuries; `zuf_format_datetime()` round-trips through `zuf_parse_datetime()`.
- **UUID, hex, Base64.** Exhaustive over every input length up to 6 bytes for encoders;
  decoders against reference implementations written in R inside the test helpers, so that
  no Suggests dependency is needed; every malformed class (bad length, bad byte, partial
  padding, non-zero trailing bits, whitespace) rejected with the right status.
- **Hashing.** Equality with xxHash's published test vectors and with `XXH3_*` called
  directly from the harness, for lengths 0–1024 and a 1 MiB buffer, at seed 0 and a
  non-zero seed; streaming equals one-shot at every split point.
- **float16.** Exhaustive over all 65 536 bit patterns in both directions against a
  reference conversion in R; bfloat16 the same.
- **UTF-8.** The full set of Markus Kuhn's stress-test cases, every overlong and surrogate
  encoding, and agreement with `validUTF8()` over random byte strings.

The always-compiled `.Call` harness in `zufast_test.c` drives every C function with
caller-chosen buffer capacities, so the `cap`-too-small branches of §9.4, §13 and §14 are
tested, not assumed.

Tests are self-sufficient, pass under `devtools::test(shuffle = TRUE)`, assert on condition
classes never on message text, and finish in under a minute.

### 21.5 Hardening

`hardening.yaml` compiles the headers into libFuzzer targets with ASan and UBSan, no R in
the picture: `fuzz_number`, `fuzz_datetime`, `fuzz_uuid`, `fuzz_base64`, `fuzz_hex`,
`fuzz_utf8`, `fuzz_hash`, each with a seed corpus and a canary that must crash.
`fuzz_number` also checks the parsers against the C library: doubles against glibc's
correctly rounded `strtod`, integers in every base against `strtoll`/`strtoull`.
`fuzz_hash` checks the streaming hasher, fed in input-chosen chunks, against the one-shot
digests. `fuzz_upstream` (C++) checks every number parser, under every option and base,
against fast_float itself (pinned in `tools/fuzz/fast_float`, outside the package): ffc is
a port, so a divergence is a porting bug or an upstream fix ffc lacks. The one intended
divergence is ffc's RFC 8259 rule that a JSON exponent needs digits (`1e` is invalid). `native-checks.yaml` runs the package under UBSan, ASan, valgrind, LTO, gctorture
and rchk; `arch.yaml` runs the suite on i386, musl and big-endian s390x; and
`vendor-upstream.yaml` opens an issue when a vendored library releases. All through
`pedrobtz/r-actions`, pinned by commit.

### 21.6 The consumer fixture

`tools/zufasttest` is a complete package in the shape every consumer will have:
`LinkingTo: zufast`, no `Imports:`, nothing in `NAMESPACE` but `useDynLib`, two translation
units that both include `<zufast.h>` and call into it, and a testthat suite that exercises
every public function. `consumer.yaml` installs zufast, installs the fixture, runs its tests
on Linux, macOS and Windows, asserts with `nm` that its shared object defines no global
`zuf_` or vendor symbol, and finally moves the installed zufast out of the library path and
proves the fixture still loads and works. A `R CMD check --as-cran` of the fixture must
show no compiled-code NOTE. The same workflow builds each family consumer at `@main` once it
has adopted zufast, as zukomp's builds zuxlsx.

### 21.7 Vendoring

`tools/vendor/verify` on every push through `vendor.yml`; a pull request touching the
vendor tree must update the manifest and the checksums.

## 22. Benchmarks

`tools/benchmarks.R` and `tools/run-benchmarks`, in the repository and not in CI, since
shared-runner timings are too noisy to gate on. Compared, each with the package in
`Suggests` of nothing and installed by the script:

| Area | Against | Inputs |
|---|---|---|
| Parse doubles | `R_strtod()`, `strtod()`, RcppFastFloat, `as.numeric()` | small integers, ordinary decimals, scientific, long mantissas, subnormals, the canada and mesh corpora |
| Format doubles | `sprintf("%.17g")`, `as.character()`, yyjson's writer through zujson | random, integral, tiny, huge |
| Dates | `as.Date()`, `as.POSIXct()`, `fasttime::fastPOSIXct()`, `clock` | the four forms of §10.1 separately |
| XXH3 | `digest::digest(algo = "xxhash64")` | 8 B, 16 B, 32 B, 64 B, 1 KiB, 1 MiB |
| Base64 | `base64enc`, `jsonlite::base64_enc()` | 16 B, 1 KiB, 1 MiB |

Results are recorded per machine in `.agents/benchmarks.md` with the commit they were taken
at, on x86-64 Linux and Apple ARM64 at minimum.

## 23. v0.1.0: deliverables and acceptance criteria

Everything in §2's in-scope table, plus the package around it, ships in one release. The
work is tracked in a single GitHub milestone, one issue per area header, each closed by the
criteria below; there are no stages and no `.9000` releases before 0.1.0.

Deliverables:

1. the headers of §5, every function of §7–§17, each documented in its header;
2. the vendor tree, manifest, scripts and patches of §18;
3. the R API of §20 with roxygen documentation, runnable examples and a `NEWS.md` entry;
4. `tools/zufasttest` and `consumer.yaml` (§21.6);
5. the gates of §21.1–§21.5 and §21.7, each with a log showing it exercised its target;
6. `README.md` with a "Using zufast from C" section that is the consumer recipe in full;
7. a `vignette("linking")` stating the §4 contract and the §8.4 and §9.1 differences from
   base R;
8. `inst/COPYRIGHTS`, `LICENSE.note`, `cran-comments.md`;
9. the five adoption issues of §3.2, filed and linked from `README.md`;
10. `.agents/benchmarks.md` with first measurements.

Acceptance criteria:

1. Every header compiles standalone as C99 and C++11 under the flags of §21.1, on Linux
   and macOS, and a unit that includes everything and uses nothing is warning-free.
2. `zufast.so` exports exactly `R_init_zufast`.
3. The fixture package builds, tests green and loads with zufast uninstalled, on all three
   OSes, and its `R CMD check` shows no compiled-code NOTE.
4. `tools/vendor/verify` is clean, and every patch is named in the manifest,
   `inst/COPYRIGHTS` and `cran-comments.md`.
5. The parse corpus agrees on every case; the shortest-format round-trip property holds on
   the full random and vector suites; the date differential against R is exact.
6. XXH3 results equal xxHash's published vectors.
7. Every fuzz target has run for at least one cumulative hour without a finding, and each
   canary was seen to crash.
8. `devtools::check(cran = TRUE)` is 0/0/0 on all three platforms and the test suite passes
   shuffled.
9. The upstream licence texts are installed and `inst/COPYRIGHTS` points at them.
10. `README.md`'s consumer recipe has been followed verbatim by the fixture, with nothing
    the fixture needed that the recipe omits.

Then: tag `v0.1.0`, submit, and move `main` to `0.1.0.9000`.

## 24. Decision log

| # | Question | Decision, and why |
|---|---|---|
| 1 | Package and prefix | `zufast`, `zuf_`/`ZUF_`, R `fast_`. Revision 1's `fastc`/`fc_` predates the family conventions; `zu_` is zukomp's. |
| 2 | Consumption mode | **Header-only, `LinkingTo` alone** (§4). The table forbids inlining and makes zufast a run-time dependency; the archive needs a `configure` in every consumer; neither reason the siblings have for them applies to pure primitives. |
| 3 | Implementation macro or `static inline` everywhere? | `static inline` everywhere, the `XXH_INLINE_ALL` model. The stb-style macro fails across translation units once functions are `static`. |
| 4 | ABI versioning | None: source compatibility only, with `ZUFAST_VERSION_*` macros. No `struct_size`, no versioned callable. |
| 5 | Number parser | `ffc.h` v26.09.01, MIT option. The canonical C port of `fast_float`, single file, tagged releases with checksums, 32-bit fallback, fixed-notation formatter included. |
| 6 | Correctly rounded or `as.numeric()`-exact? | Correctly rounded (§8.4). zucsv keeps its own transcription; zufast documents the difference and offers no R-exact mode. |
| 7 | Shortest float formatting | Ryu `d2s`/`f2s` v2.0 under Boost. No C port of Dragonbox exists; yyjson's writer is entangled in yyjson. `d2fixed` is replaced by ffc's fixed formatter. |
| 8 | Notation of shortest output | ECMAScript `Number::toString`, `e` lower-case, R spellings for non-finite (§9.1). Ryu's native `1E0` suits no consumer. |
| 9 | Integer writers | Package-owned, two-digit table; the unchecked `zuf_write_*` shape is what serialisers want. |
| 10 | Date result type | **Fields, not an epoch** (§10.2). The adjusted epoch of revision 1 lost the offset and the "no offset" fact. |
| 11 | Calendar arithmetic | Neri–Schneider affine functions, package-owned. |
| 12 | Base64 | Package-owned scalar codec, strict decoder. aklomp/base64's SIMD needs per-file flags consumers cannot provide. |
| 13 | UTF-8 | Höhrmann DFA, transcribed with attribution. Correctness baseline first; SIMD only against it. |
| 14 | Hashing | xxHash v0.8.4 `xxhash.h` alone, `XXH_INLINE_ALL`, `XXH_NO_STDLIB`; streaming state opaque by size. |
| 15 | UUID representation | Sixteen bytes in network order, not two `uint64_t`. |
| 16 | Boolean policy | Flag per spelling family; no missing-token parser. Policies belong to consumers. |
| 17 | SIMD | None in v0.1.0; later only through target attributes with runtime dispatch inside the header (§19.3). MSVC dropped from the targets. |
| 18 | Vendoring layout | Under `inst/include/zufast/vendor/`, since the headers are the product; zucrypt-generation manifest and scripts; a small patch set (linkage and CRAN-pragma fixes), each upstreamed. |
| 19 | Where the R layer stops | §20: the `STRSXP` optimisations of revision 1 have no consumer and are dropped. |
| 20 | Release plan | One release, v0.1.0, with every area; no stages. The delivery mechanism, not the breadth, is the risk, and it is proven by the fixture package. |
| 21 | Varints, CRC32C, HTTP dates | Deferred until a consumer asks (§25). |

## 25. Deferred

Each of these is admitted by the §2 test only when a consumer names it:

- **varints and zig-zag**: CBOR is fixed-width; no protobuf-shaped consumer exists;
- **CRC32C**: Parquet and Kafka would want it; neither has a reader in the family;
- **HTTP dates** (IMF-fixdate): zuhttp's adoption issue invites the request;
- **ISO week and ordinal dates, basic format, expanded years, `24:00:00`, comma fractions**;
- **a JSON number classifier** (integer, unsigned, double), which `ffc_parse_json_number()`
  already provides and would be a thin wrapper;
- **SIMD paths** for dates, Base64, hex and UTF-8, under the §19.3 rule;
- **a `bit64::integer64` return** from the R layer; the C layer already parses `int64_t`.
