// AI-GENERATED

#include "string_diff.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* ---- color handling ---------------------------------------------------- */

static const char *C_RED = "", *C_GRN = "", *C_DIM = "", *C_RST = "";

static void diff_colors_init(void) {
    const char *no_color = getenv("NO_COLOR");
    if (!no_color) {
        C_RED = "\x1b[31m";
        C_GRN = "\x1b[32m";
        C_DIM = "\x1b[2m";
        C_RST = "\x1b[0m";
    } else {
        C_RED = C_GRN = C_DIM = C_RST = "";
    }
}

/* ---- split a string into lines (keeps them NUL-terminated) ------------- */

typedef struct {
    char **v;
    int n;
} Lines;

static Lines split_lines(const char *s) {
    Lines L = {0};
    if (!s) s = "";
    int cap = 8;
    L.v = malloc(sizeof(char *) * cap);
    const char *start = s;
    for (const char *p = s;; p++) {
        if (*p == '\n' || *p == '\0') {
            size_t len = (size_t) (p - start);
            char *line = malloc(len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            if (L.n == cap) {
                cap *= 2;
                L.v = realloc(L.v, sizeof(char *) * cap);
            }
            L.v[L.n++] = line;
            if (*p == '\0') break;
            start = p + 1;
        }
    }
    return L;
}

static void free_lines(Lines *L) {
    for (int i = 0; i < L->n; i++) free(L->v[i]);
    free(L->v);
}

/* ---- LCS table over lines --------------------------------------------- */

static void print_diff(const char *expected, const char *actual) {
    Lines a = split_lines(expected); /* expected -> "-" red   */
    Lines b = split_lines(actual); /* actual   -> "+" green */

    /* dp[i][j] = LCS length of a[i..] and b[j..] */
    int rows = a.n + 1, cols = b.n + 1;
    int *dp = calloc((size_t) rows * cols, sizeof(int));
#define DP(i, j) dp[(i) * cols + (j)]
    for (int i = a.n - 1; i >= 0; i--)
        for (int j = b.n - 1; j >= 0; j--)
            DP(i, j) = strcmp(a.v[i], b.v[j]) == 0
                           ? DP(i + 1, j + 1) + 1
                           : (DP(i + 1, j) >= DP(i, j + 1)
                                  ? DP(i + 1, j)
                                  : DP(i, j + 1));

    fprintf(stderr, "\n  %s- expected%s\n  %s+ actual%s\n\n",
            C_RED, C_RST, C_GRN, C_RST);

    int i = 0, j = 0;
    while (i < a.n && j < b.n) {
        if (strcmp(a.v[i], b.v[j]) == 0) {
            fprintf(stderr, "  %s  %s%s\n", C_DIM, a.v[i], C_RST);
            i++;
            j++;
        } else if (DP(i + 1, j) >= DP(i, j + 1)) {
            fprintf(stderr, "  %s- %s%s\n", C_RED, a.v[i], C_RST);
            i++;
        } else {
            fprintf(stderr, "  %s+ %s%s\n", C_GRN, b.v[j], C_RST);
            j++;
        }
    }
    while (i < a.n) fprintf(stderr, "  %s- %s%s\n", C_RED, a.v[i++], C_RST);
    while (j < b.n) fprintf(stderr, "  %s+ %s%s\n", C_GRN, b.v[j++], C_RST);
    fprintf(stderr, "\n");
#undef DP
    free(dp);
    free_lines(&a);
    free_lines(&b);
}

/* ---- the Unity-facing assertion --------------------------------------- */

void UnityAssertEqualStringDiff(const char *expected,
                                const char *actual,
                                unsigned int line) {
    if (expected && actual && strcmp(expected, actual) == 0) return;
    if (!expected && !actual) return;

    diff_colors_init();
    fprintf(stderr, "\nTest: %s\n", Unity.CurrentTestName ? Unity.CurrentTestName : "(unknown)");
    print_diff(expected ? expected : "(null)", actual ? actual : "(null)");
    UNITY_TEST_FAIL(line, "string mismatch (see diff above)");
}
