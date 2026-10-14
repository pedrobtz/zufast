/* The test harness for zufast/wide.h, compiled twice: by zufast_test_wide.c
   with the native 128-bit paths and by zufast_test_wide_fallback.c with
   ZUF_NO_INT128. Each includes <zufast/wide.h> and "zufast_r.h", defines
   ZUF_TW(name) to its own entry-point prefix, then includes this file.
   test-wide.R runs every vector through both and requires identical
   results.

   Values cross the .Call boundary as 32 lower-case hex digits, high word
   first; signed values in two's complement. */

static uint64_t hex_word(const char *s)
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < 16; i++) {
        int c = s[i], d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (d < 0) Rf_error("not a hex digit: %s", s);
        v = v << 4 | (uint64_t)d;
    }
    return v;
}

static zuf_u128 hex_u(SEXP x, R_xlen_t i)
{
    const char *s = CHAR(STRING_ELT(x, i % XLENGTH(x)));
    if (strlen(s) != 32) Rf_error("expected 32 hex digits: %s", s);
    return zuf_u128_make(hex_word(s), hex_word(s + 16));
}

static zuf_i128 hex_i(SEXP x, R_xlen_t i)
{
    return zuf_i128_from_u128(hex_u(x, i));
}

static void put_hex(char *d, zuf_u128 v)
{
    snprintf(d, 33, "%016llx%016llx", (unsigned long long)v.high, (unsigned long long)v.low);
}

static void put_hex_i(char *d, zuf_i128 v)
{
    put_hex(d, zuf_u128_from_i128(v));
}

/* op(name, a, b): one string per element of the longer of a and b.
   Checked operations give "F" on failure; divisions give "q r". */
SEXP ZUF_TW(op)(SEXP name, SEXP a, SEXP b)
{
    const char *op = CHAR(STRING_ELT(name, 0));
    R_xlen_t i, n = XLENGTH(a) > XLENGTH(b) ? XLENGTH(a) : XLENGTH(b);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        char buf[80] = "F";
        zuf_u128 ua = hex_u(a, i), ub = hex_u(b, i), uq, ur;
        zuf_i128 ia = hex_i(a, i), ib = hex_i(b, i), iq, ir;
        int64_t r64;
        uint64_t u64;
        if (!strcmp(op, "u_add"))           { if (zuf_u128_add(ua, ub, &uq)) put_hex(buf, uq); }
        else if (!strcmp(op, "u_sub"))      { if (zuf_u128_sub(ua, ub, &uq)) put_hex(buf, uq); }
        else if (!strcmp(op, "u_mul"))      { if (zuf_u128_mul(ua, ub, &uq)) put_hex(buf, uq); }
        else if (!strcmp(op, "u_add_wrap")) put_hex(buf, zuf_u128_add_wrap(ua, ub));
        else if (!strcmp(op, "u_sub_wrap")) put_hex(buf, zuf_u128_sub_wrap(ua, ub));
        else if (!strcmp(op, "u_mul_wrap")) put_hex(buf, zuf_u128_mul_wrap(ua, ub));
        else if (!strcmp(op, "i_add"))      { if (zuf_i128_add(ia, ib, &iq)) put_hex_i(buf, iq); }
        else if (!strcmp(op, "i_sub"))      { if (zuf_i128_sub(ia, ib, &iq)) put_hex_i(buf, iq); }
        else if (!strcmp(op, "i_mul"))      { if (zuf_i128_mul(ia, ib, &iq)) put_hex_i(buf, iq); }
        else if (!strcmp(op, "i_neg"))      { if (zuf_i128_neg(ia, &iq)) put_hex_i(buf, iq); }
        else if (!strcmp(op, "i_add_wrap")) put_hex_i(buf, zuf_i128_add_wrap(ia, ib));
        else if (!strcmp(op, "i_sub_wrap")) put_hex_i(buf, zuf_i128_sub_wrap(ia, ib));
        else if (!strcmp(op, "i_mul_wrap")) put_hex_i(buf, zuf_i128_mul_wrap(ia, ib));
        else if (!strcmp(op, "u_cmp"))      snprintf(buf, sizeof buf, "%d", zuf_u128_cmp(ua, ub));
        else if (!strcmp(op, "i_cmp"))      snprintf(buf, sizeof buf, "%d", zuf_i128_cmp(ia, ib));
        else if (!strcmp(op, "i_is_neg"))   snprintf(buf, sizeof buf, "%d", (int)zuf_i128_is_neg(ia));
        else if (!strcmp(op, "u_mul_u64_u64")) put_hex(buf, zuf_u128_mul_u64_u64(ua.low, ub.low));
        else if (!strcmp(op, "i_mul_i64_i64")) {
            int64_t x, y;
            memcpy(&x, &ua.low, 8); memcpy(&y, &ub.low, 8);
            put_hex_i(buf, zuf_i128_mul_i64_i64(x, y));
        }
        else if (!strcmp(op, "u_divmod")) {
            if (zuf_u128_divmod(ua, ub, &uq, &ur)) { put_hex(buf, uq); buf[32] = ' '; put_hex(buf + 33, ur); }
        } else if (!strcmp(op, "u_divmod_u64")) {
            if (zuf_u128_divmod_u64(ua, ub.low, &uq, &u64)) {
                put_hex(buf, uq); buf[32] = ' '; put_hex(buf + 33, zuf_u128_from_u64(u64));
            }
        } else if (!strcmp(op, "i_divmod_trunc") || !strcmp(op, "i_divmod_floor")) {
            bool ok = op[9] == 't' ? zuf_i128_divmod_trunc(ia, ib, &iq, &ir)
                                    : zuf_i128_divmod_floor(ia, ib, &iq, &ir);
            if (ok) { put_hex_i(buf, iq); buf[32] = ' '; put_hex_i(buf + 33, ir); }
        } else if (!strcmp(op, "i_divmod_trunc_i64") || !strcmp(op, "i_divmod_floor_i64")) {
            int64_t y;
            bool ok;
            memcpy(&y, &ub.low, 8);
            ok = op[9] == 't' ? zuf_i128_divmod_trunc_i64(ia, y, &iq, &r64)
                               : zuf_i128_divmod_floor_i64(ia, y, &iq, &r64);
            if (ok) { put_hex_i(buf, iq); buf[32] = ' '; put_hex_i(buf + 33, zuf_i128_from_i64(r64)); }
        }
        else if (!strcmp(op, "i_to_i64"))   { if (zuf_i128_to_i64(ia, &r64)) put_hex_i(buf, zuf_i128_from_i64(r64)); }
        else if (!strcmp(op, "u_to_u64"))   { if (zuf_u128_to_u64(ua, &u64)) put_hex(buf, zuf_u128_from_u64(u64)); }
        else if (!strcmp(op, "u_write"))    *zuf_write_u128(buf, ua) = 0;
        else if (!strcmp(op, "i_write"))    *zuf_write_i128(buf, ia) = 0;
        else Rf_error("unknown op %s", op);
        SET_STRING_ELT(out, i, Rf_mkChar(buf));
    }
    UNPROTECT(1);
    return out;
}

/* parse(kind, x): "status consumed hex" per element; hex is "-" unless the
   status is OK or RANGE. */
SEXP ZUF_TW(parse)(SEXP kind, SEXP x)
{
    bool is_signed = CHAR(STRING_ELT(kind, 0))[0] == 'i';
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        const char *s = CHAR(STRING_ELT(x, i));
        size_t len = strlen(s);
        char buf[80], hex[33] = "-";
        zuf_u128 u = zuf_u128_make(0x5A5A, 0x5A5A);
        zuf_i128 v = zuf_int_i128_make(0x5A5A, 0x5A5A);
        zuf_result r = is_signed ? zuf_parse_i128(s, s + len, &v) : zuf_parse_u128(s, s + len, &u);
        if (r.status == ZUF_OK || r.status == ZUF_ERR_RANGE) {
            if (is_signed) put_hex_i(hex, v); else put_hex(hex, u);
        } else if (is_signed ? (v.high != 0x5A5A || v.low != 0x5A5A) : (u.high != 0x5A5A || u.low != 0x5A5A)) {
            strcpy(hex, "WRITTEN");   /* *out must be untouched on INVALID */
        }
        snprintf(buf, sizeof buf, "%d %d %s", (int)r.status, (int)(r.ptr - s), hex);
        SET_STRING_ELT(out, i, Rf_mkChar(buf));
    }
    UNPROTECT(1);
    return out;
}

/* to_f64(kind, a): the exact double, or NA. */
SEXP ZUF_TW(to_f64)(SEXP kind, SEXP a)
{
    bool is_signed = CHAR(STRING_ELT(kind, 0))[0] == 'i';
    R_xlen_t i, n = XLENGTH(a);
    SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
    for (i = 0; i < n; i++) {
        double d = 0;
        bool ok = is_signed ? zuf_i128_to_f64(hex_i(a, i), &d) : zuf_u128_to_f64(hex_u(a, i), &d);
        REAL(out)[i] = ok ? d : NA_REAL;
    }
    UNPROTECT(1);
    return out;
}

/* from_f64(kind, x): hex, or "F". */
SEXP ZUF_TW(from_f64)(SEXP kind, SEXP x)
{
    bool is_signed = CHAR(STRING_ELT(kind, 0))[0] == 'i';
    R_xlen_t i, n = XLENGTH(x);
    SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
    for (i = 0; i < n; i++) {
        char buf[33] = "F";
        zuf_u128 u;
        zuf_i128 v;
        if (is_signed) { if (zuf_i128_from_f64(REAL(x)[i], &v)) put_hex_i(buf, v); }
        else if (zuf_u128_from_f64(REAL(x)[i], &u)) put_hex(buf, u);
        SET_STRING_ELT(out, i, Rf_mkChar(buf));
    }
    UNPROTECT(1);
    return out;
}

/* ---- random differential and properties --------------------------------- */

typedef struct { uint64_t s, h; double bad; } wide_state;

static uint64_t wide_next(wide_state *st)
{
    st->s ^= st->s << 13;
    st->s ^= st->s >> 7;
    st->s ^= st->s << 17;
    return st->s;
}

static void wide_mix(wide_state *st, uint64_t v)
{
    st->h = (st->h ^ v) * UINT64_C(0x100000001B3);
    st->h ^= st->h >> 29;
}

static void wide_mix_u(wide_state *st, zuf_u128 v) { wide_mix(st, v.low); wide_mix(st, v.high); }
static void wide_mix_i(wide_state *st, zuf_i128 v) { wide_mix(st, v.low); wide_mix(st, v.high); }

/* A value with a random bit width, so that every division length occurs,
   or one of the boundary values. */
static zuf_u128 wide_value(wide_state *st)
{
    static const uint64_t special[][2] = {
        {0, 0}, {0, 1}, {0, 2}, {~UINT64_C(0), ~UINT64_C(0)}, {~UINT64_C(0), ~UINT64_C(0) - 1},
        {UINT64_C(1) << 63, 0}, {UINT64_C(1) << 63, 1}, {~(UINT64_C(1) << 63), ~UINT64_C(0)},
        {0, ~UINT64_C(0)}, {1, 0}, {0, UINT64_C(1) << 63}, {0, UINT64_C(1) << 53},
        {0, (UINT64_C(1) << 53) + 1}, {0, UINT64_C(10000000000000000000)}
    };
    uint64_t pick = wide_next(st) % 16;
    if (pick < sizeof special / sizeof special[0]) {
        zuf_u128 v = zuf_u128_make(special[pick][0], special[pick][1]);
        /* and their negations */
        return (wide_next(st) & 1) ? zuf_u128_sub_wrap(zuf_u128_make(0, 0), v) : v;
    }
    {
        unsigned w = (unsigned)(wide_next(st) % 129);
        zuf_u128 v = zuf_u128_make(wide_next(st), wide_next(st));
        return w == 0 ? zuf_u128_make(0, 0) : zuf_int_u128_shr(v, 128 - w);
    }
}

static int wide_lt0(zuf_i128 v) { return zuf_i128_is_neg(v); }
static int wide_is0(zuf_i128 v) { return (v.high | v.low) == 0; }

static void wide_check_pair(wide_state *st, zuf_u128 ua, zuf_u128 ub)
{
    zuf_i128 ia = zuf_i128_from_u128(ua), ib = zuf_i128_from_u128(ub), iq, ir, t;
    zuf_u128 uq, ur, ut;
    const zuf_i128 imax = zuf_int_i128_make(~(UINT64_C(1) << 63), ~UINT64_C(0));
    const zuf_i128 imin = zuf_int_i128_make(UINT64_C(1) << 63, 0);
    char text[ZUF_I128_MAX_CHARS + 1];
    bool ok;
    int fl;
    int64_t r64;
    uint64_t u64;

    /* unsigned add, sub, mul */
    ok = zuf_u128_add(ua, ub, &ut);
    wide_mix(st, ok); wide_mix_u(st, zuf_u128_add_wrap(ua, ub));
    if (ok != (zuf_u128_cmp(ua, zuf_u128_sub_wrap(zuf_u128_make(~UINT64_C(0), ~UINT64_C(0)), ub)) <= 0)) st->bad++;
    if (ok && zuf_u128_cmp(ut, zuf_u128_add_wrap(ua, ub)) != 0) st->bad++;
    ok = zuf_u128_sub(ua, ub, &ut);
    wide_mix(st, ok); wide_mix_u(st, zuf_u128_sub_wrap(ua, ub));
    if (ok != (zuf_u128_cmp(ua, ub) >= 0)) st->bad++;
    ok = zuf_u128_mul(ua, ub, &ut);
    wide_mix(st, ok); wide_mix_u(st, zuf_u128_mul_wrap(ua, ub));
    if (ok && zuf_u128_cmp(ut, zuf_u128_mul_wrap(ua, ub)) != 0) st->bad++;
    if (!zuf_int_u128_is_zero(ua)) {
        /* the product fits iff dividing the wrapped product by a gives b back exactly */
        zuf_u128 p = zuf_u128_mul_wrap(ua, ub);
        bool fits;
        zuf_u128_divmod(p, ua, &uq, &ur);
        fits = zuf_u128_cmp(uq, ub) == 0 && zuf_int_u128_is_zero(ur);
        if (ok != fits) st->bad++;
    } else if (!ok) st->bad++;

    /* unsigned division */
    ok = zuf_u128_divmod(ua, ub, &uq, &ur);
    wide_mix(st, ok);
    if (ok) {
        wide_mix_u(st, uq); wide_mix_u(st, ur);
        if (zuf_u128_cmp(ur, ub) >= 0) st->bad++;
        if (zuf_u128_cmp(zuf_u128_add_wrap(zuf_u128_mul_wrap(uq, ub), ur), ua) != 0) st->bad++;
    } else if (!zuf_int_u128_is_zero(ub)) st->bad++;
    if (zuf_u128_divmod_u64(ua, ub.low, &uq, &u64)) {
        wide_mix_u(st, uq); wide_mix(st, u64);
        if (u64 >= ub.low) st->bad++;
    }

    /* signed add, sub, mul, neg */
    ok = zuf_i128_add(ia, ib, &t);
    wide_mix(st, ok); wide_mix_i(st, zuf_i128_add_wrap(ia, ib));
    {
        bool fits = wide_lt0(ib) ? zuf_i128_cmp(ia, zuf_i128_sub_wrap(imin, ib)) >= 0
                                 : zuf_i128_cmp(ia, zuf_i128_sub_wrap(imax, ib)) <= 0;
        if (ok != fits) st->bad++;
    }
    ok = zuf_i128_sub(ia, ib, &t);
    wide_mix(st, ok); wide_mix_i(st, zuf_i128_sub_wrap(ia, ib));
    {
        bool fits = wide_lt0(ib) ? zuf_i128_cmp(ia, zuf_i128_add_wrap(imax, ib)) <= 0
                                 : zuf_i128_cmp(ia, zuf_i128_add_wrap(imin, ib)) >= 0;
        if (ok != fits) st->bad++;
    }
    ok = zuf_i128_mul(ia, ib, &t);
    wide_mix(st, ok); wide_mix_i(st, zuf_i128_mul_wrap(ia, ib));
    if (ok && (t.high != zuf_i128_mul_wrap(ia, ib).high || t.low != zuf_i128_mul_wrap(ia, ib).low)) st->bad++;
    if (!wide_is0(ia)) {
        zuf_i128 p = zuf_i128_mul_wrap(ia, ib);
        bool fits = zuf_i128_divmod_trunc(p, ia, &iq, &ir) && zuf_i128_cmp(iq, ib) == 0 && wide_is0(ir);
        /* MIN / -1 cannot be divided back: MIN * -1 overflows, -1 * MIN too */
        if (ok != fits) st->bad++;
    } else if (!ok) st->bad++;
    ok = zuf_i128_neg(ia, &t);
    wide_mix(st, ok);
    if (ok != (zuf_i128_cmp(ia, imin) != 0)) st->bad++;
    wide_mix(st, (uint64_t)(zuf_i128_cmp(ia, ib) + 1)); wide_mix(st, (uint64_t)(zuf_u128_cmp(ua, ub) + 1));

    /* signed division: q * b + r == a, |r| < |b|, and the sign rules */
    for (fl = 0; fl < 2; fl++) {
        ok = fl ? zuf_i128_divmod_floor(ia, ib, &iq, &ir) : zuf_i128_divmod_trunc(ia, ib, &iq, &ir);
        wide_mix(st, ok);
        if (!ok) {
            if (!wide_is0(ib) && !(zuf_i128_cmp(ia, imin) == 0 && zuf_i128_cmp(ib, zuf_i128_from_i64(-1)) == 0))
                st->bad++;
            continue;
        }
        wide_mix_i(st, iq); wide_mix_i(st, ir);
        if (zuf_i128_cmp(zuf_i128_add_wrap(zuf_i128_mul_wrap(iq, ib), ir), ia) != 0) st->bad++;
        if (zuf_u128_cmp(zuf_int_i128_magnitude(ir), zuf_int_i128_magnitude(ib)) >= 0) st->bad++;
        if (!wide_is0(ir) && wide_lt0(ir) != wide_lt0(fl ? ib : ia)) st->bad++;
    }
    {
        int64_t y;
        memcpy(&y, &ub.low, 8);
        for (fl = 0; fl < 2; fl++) {
            zuf_i128 q2, r2;
            bool ok2;
            ok = fl ? zuf_i128_divmod_floor_i64(ia, y, &iq, &r64) : zuf_i128_divmod_trunc_i64(ia, y, &iq, &r64);
            ok2 = fl ? zuf_i128_divmod_floor(ia, zuf_i128_from_i64(y), &q2, &r2)
                        : zuf_i128_divmod_trunc(ia, zuf_i128_from_i64(y), &q2, &r2);
            wide_mix(st, ok);
            if (ok != ok2) st->bad++;
            if (ok) {
                wide_mix_i(st, iq); wide_mix(st, (uint64_t)r64);
                if (zuf_i128_cmp(iq, q2) != 0 || zuf_i128_cmp(zuf_i128_from_i64(r64), r2) != 0) st->bad++;
            }
        }
    }

    /* the 64 x 64 products against the wrapping 128-bit product */
    if (zuf_u128_cmp(zuf_u128_mul_u64_u64(ua.low, ub.low),
                     zuf_u128_mul_wrap(zuf_u128_from_u64(ua.low), zuf_u128_from_u64(ub.low))) != 0) st->bad++;
    {
        int64_t x, y;
        memcpy(&x, &ua.low, 8); memcpy(&y, &ub.low, 8);
        t = zuf_i128_mul_i64_i64(x, y);
        wide_mix_i(st, t);
        if (zuf_i128_cmp(t, zuf_i128_mul_wrap(zuf_i128_from_i64(x), zuf_i128_from_i64(y))) != 0) st->bad++;
    }

    /* text round trip */
    {
        zuf_u128 ub2;
        zuf_i128 ib2;
        char *e = zuf_write_u128(text, ua);
        zuf_result r = zuf_parse_u128(text, e, &ub2);
        wide_mix(st, (uint64_t)(e - text));
        if (e - text > ZUF_U128_MAX_CHARS || r.status != ZUF_OK || r.ptr != e || zuf_u128_cmp(ub2, ua) != 0) st->bad++;
        e = zuf_write_i128(text, ia);
        r = zuf_parse_i128(text, e, &ib2);
        wide_mix(st, (uint64_t)(e - text));
        if (e - text > ZUF_I128_MAX_CHARS || r.status != ZUF_OK || r.ptr != e || zuf_i128_cmp(ib2, ia) != 0) st->bad++;
    }

    /* 64-bit narrowing */
    ok = zuf_i128_to_i64(ia, &r64);
    wide_mix(st, ok);
    if (ok && zuf_i128_cmp(zuf_i128_from_i64(r64), ia) != 0) st->bad++;
    if (!ok && zuf_i128_cmp(ia, zuf_i128_from_i64(INT64_MIN)) >= 0 && zuf_i128_cmp(ia, zuf_i128_from_i64(INT64_MAX)) <= 0)
        st->bad++;
    ok = zuf_u128_to_u64(ua, &u64);
    if (ok != (ua.high == 0)) st->bad++;

    /* double: exact or false, and back */
    {
        double d;
        zuf_i128 back;
        zuf_u128 uback;
        ok = zuf_i128_to_f64(ia, &d);
        wide_mix(st, ok);
        if (ok) {
            wide_mix(st, zuf_int_wide_f64_bits(d));
            if (!zuf_i128_from_f64(d, &back) || zuf_i128_cmp(back, ia) != 0) st->bad++;
        }
        ok = zuf_u128_to_f64(ua, &d);
        wide_mix(st, ok);
        if (ok && (!zuf_u128_from_f64(d, &uback) || zuf_u128_cmp(uback, ua) != 0)) st->bad++;
    }
}

/* random(n): list(digest, failures) over n random pairs, plus every double
   from random bits through from_f64 and back. */
SEXP ZUF_TW(random)(SEXP n)
{
    wide_state st;
    long i, count = (long)Rf_asReal(n);
    char hex[17];
    SEXP out;
    st.s = UINT64_C(0x9E3779B97F4A7C15);
    st.h = UINT64_C(0xCBF29CE484222325);
    st.bad = 0;
    for (i = 0; i < count; i++) {
        zuf_u128 a = wide_value(&st), b = wide_value(&st);
        uint64_t bits = wide_next(&st);
        double d, back;
        zuf_i128 v;
        zuf_u128 u;
        wide_check_pair(&st, a, b);

        /* doubles: an integral double converts exactly and back; others fail */
        memcpy(&d, &bits, 8);
        if (zuf_i128_from_f64(d, &v)) {
            wide_mix_i(&st, v);
            if (!zuf_i128_to_f64(v, &back) || back != d) st.bad++;
        } else {
            wide_mix(&st, 1);
        }
        if (zuf_u128_from_f64(d, &u)) {
            wide_mix_u(&st, u);
            if (!zuf_u128_to_f64(u, &back) || back != d) st.bad++;
        }
    }
    snprintf(hex, sizeof hex, "%016llx", (unsigned long long)st.h);
    out = PROTECT(Rf_allocVector(VECSXP, 2));
    SET_VECTOR_ELT(out, 0, Rf_mkString(hex));
    SET_VECTOR_ELT(out, 1, Rf_ScalarReal(st.bad));
    UNPROTECT(1);
    return out;
}

/* 1 when this unit uses the native type. */
SEXP ZUF_TW(native)(void)
{
    return Rf_ScalarInteger(ZUF_INT_HAVE_INT128);
}
