/* utils.c - input helpers, validation and date utilities */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "library.h"

/* ------------------------------ Input -------------------------------- */

void readLine(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    if (!fgets(buf, (int)size, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
}

/* Reads an integer; returns 1 on success, 0 on invalid input. */
int readInt(const char *prompt, int *out) {
    char buf[64], *end;
    readLine(prompt, buf, sizeof buf);
    if (buf[0] == '\0') return 0;
    long v = strtol(buf, &end, 10);
    if (*end != '\0') return 0;
    *out = (int)v;
    return 1;
}

/* Copies src into dst (capacity = size) and always NUL-terminates. */
void copyStr(char *dst, const char *src, size_t size) {
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ---------------------------- Validation ----------------------------- */

int isBlank(const char *s) {
    while (*s) if (!isspace((unsigned char)*s++)) return 0;
    return 1;
}

int containsIgnoreCase(const char *text, const char *pat) {
    size_t n = strlen(text), m = strlen(pat);
    if (m == 0) return 1;
    for (size_t i = 0; i + m <= n; i++) {
        size_t j = 0;
        while (j < m && tolower((unsigned char)text[i + j]) ==
                        tolower((unsigned char)pat[j])) j++;
        if (j == m) return 1;
    }
    return 0;
}

int validPhone(const char *p) {
    int digits = 0;
    for (; *p; p++) {
        if (isdigit((unsigned char)*p)) digits++;
        else if (*p != '+' && *p != '-' && *p != ' ') return 0;
    }
    return digits >= 7 && digits <= 15;
}

int validEmail(const char *e) {
    const char *at = strchr(e, '@');
    if (!at || at == e) return 0;
    const char *dot = strchr(at, '.');
    return dot && dot > at + 1 && dot[1] != '\0' && !strchr(e, ' ');
}

/* ------------------------------ Dates -------------------------------- */

time_t today(void) {
    time_t now = time(NULL);
    struct tm t = *localtime(&now);
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0; t.tm_isdst = -1;
    return mktime(&t);
}

static time_t makeDate(int d, int m, int y) {
    struct tm t = {0};
    t.tm_mday = d; t.tm_mon = m - 1; t.tm_year = y - 1900; t.tm_isdst = -1;
    time_t r = mktime(&t);
    /* verify that the date was not normalised (e.g. 31-02) */
    struct tm chk = *localtime(&r);
    if (chk.tm_mday != d || chk.tm_mon != m - 1 || chk.tm_year != y - 1900)
        return (time_t)-1;
    return r;
}

/* Parses a DD-MM-YYYY string into a date. Returns 1 if valid. */
int parseDate(const char *s, time_t *out) {
    int d, m, y;
    if (sscanf(s, "%d-%d-%d", &d, &m, &y) != 3 ||
        y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    time_t t = makeDate(d, m, y);
    if (t == (time_t)-1) return 0;
    *out = t;
    return 1;
}

/* Reads DD-MM-YYYY; blank input = today. Returns 1 if valid. */
int readDate(const char *prompt, time_t *out) {
    char buf[32];
    readLine(prompt, buf, sizeof buf);
    if (buf[0] == '\0') { *out = today(); return 1; }
    return parseDate(buf, out);
}

void fmtDate(time_t t, char *buf, size_t size) {
    if (t == 0) { snprintf(buf, size, "-"); return; }
    struct tm *tm = localtime(&t);
    strftime(buf, size, "%d-%m-%Y", tm);
}

int daysBetween(time_t from, time_t to) {
    return (int)((difftime(to, from) / 86400.0) + (to >= from ? 0.5 : -0.5));
}
