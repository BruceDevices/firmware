#pragma once
#include <stdio.h>
#include <string.h>

static int esl_checks = 0;
static int esl_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        esl_checks++;                                                          \
        if (!(cond)) {                                                         \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                      \
    } while (0)

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        esl_checks++;                                                          \
        unsigned long long _a = (unsigned long long)(actual);                  \
        unsigned long long _e = (unsigned long long)(expected);                \
        if (_a != _e) {                                                        \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s (got %llu, want %llu)\n",          \
                    __FILE__, __LINE__, #actual, _a, _e);                      \
        }                                                                      \
    } while (0)

#define CHECK_STR(actual, expected)                                            \
    do {                                                                       \
        esl_checks++;                                                          \
        const char *_a = (actual);                                             \
        if (_a == NULL || strcmp(_a, (expected)) != 0) {                       \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s (got '%s', want '%s')\n",          \
                    __FILE__, __LINE__, #actual, _a ? _a : "(null)",           \
                    (expected));                                               \
        }                                                                      \
    } while (0)

#define CHECK_MEM(actual, expected, n)                                         \
    do {                                                                       \
        esl_checks++;                                                          \
        if (memcmp((actual), (expected), (size_t)(n)) != 0) {                  \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: bytes differ\n  got : ",              \
                    __FILE__, __LINE__);                                       \
            for (size_t _i = 0; _i < (size_t)(n); _i++)                        \
                fprintf(stderr, "%02X ",                                       \
                        ((const unsigned char *)(actual))[_i]);                \
            fprintf(stderr, "\n  want: ");                                     \
            for (size_t _i = 0; _i < (size_t)(n); _i++)                        \
                fprintf(stderr, "%02X ",                                       \
                        ((const unsigned char *)(expected))[_i]);              \
            fprintf(stderr, "\n");                                             \
        }                                                                      \
    } while (0)

#define TEST_REPORT(name)                                                      \
    do {                                                                       \
        printf("%s: %d checks, %d failures\n", (name), esl_checks,             \
               esl_failures);                                                  \
        return esl_failures == 0 ? 0 : 1;                                      \
    } while (0)
