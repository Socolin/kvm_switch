// AI-GENERATED
#pragma once

/*
 * Jest / git-style colored diff assertion for Unity.
 *
 *   TEST_ASSERT_EQUAL_STRING_DIFF(expected, actual);
 *
 * On mismatch it prints a unified, LCS-aligned, ANSI-colored diff:
 *   - red   lines  (expected, "-")
 *   + green lines  (actual,   "+")
 *     dim context  (unchanged)
 * then fails the test on the calling line.
 *
 * Colors auto-disable when stderr is not a TTY or when NO_COLOR is set.
 * Host-side only (uses malloc + fprintf). Not for bare-metal targets.
 */

#include "unity.h"

void UnityAssertEqualStringDiff(const char *expected,
                                const char *actual,
                                unsigned int line);

#define TEST_ASSERT_EQUAL_STRING_DIFF(expected, actual) \
    UnityAssertEqualStringDiff((expected), (actual), __LINE__)
