/*
 * Reads lines of Nigel Tao's parse-number-fxx-test-data
 * (https://github.com/nigeltao/parse-number-fxx-test-data, MIT):
 *
 *     <f16 hex> <f32 hex> <f64 hex> <decimal string>
 *
 * from stdin and checks that zuf_parse_f32 and zuf_parse_f64 consume the
 * whole string and produce exactly the reference bits. Prints the first
 * failures and a summary; exits non-zero on any failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zufast.h>

int main(void)
{
    char line[4096];
    unsigned long long lines = 0, bad = 0;
    while (fgets(line, sizeof line, stdin)) {
        char *s16 = strtok(line, " "), *s32 = strtok(NULL, " "), *s64 = strtok(NULL, " ");
        char *num = strtok(NULL, "\r\n");
        uint32_t want32, got32;
        uint64_t want64, got64;
        double d;
        float f;
        zuf_result r64, r32;
        size_t n;
        if (!s16 || !s32 || !s64 || !num) continue;
        lines++;
        want32 = (uint32_t)strtoul(s32, NULL, 16);
        want64 = (uint64_t)strtoull(s64, NULL, 16);
        n = strlen(num);
        r64 = zuf_parse_f64(num, num + n, &d);
        r32 = zuf_parse_f32(num, num + n, &f);
        memcpy(&got64, &d, 8);
        memcpy(&got32, &f, 4);
        if (r64.status == ZUF_ERR_INVALID || r64.ptr != num + n || got64 != want64 ||
            r32.status == ZUF_ERR_INVALID || r32.ptr != num + n || got32 != want32) {
            if (bad < 20)
                printf("MISMATCH %s: f64 %016llx want %016llx, f32 %08x want %08x\n", num,
                       (unsigned long long)got64, (unsigned long long)want64, got32, want32);
            bad++;
        }
    }
    printf("%llu cases, %llu mismatches\n", lines, bad);
    return bad != 0 || lines == 0;
}
