// A test harness of about thirty lines, so the suite has no dependency to
// install, vendor or keep in step with the compiler.

#pragma once

#include <cstdio>
#include <string>

namespace pandora::test {

inline int g_checks = 0;
inline int g_failures = 0;

inline void expect(bool condition, const std::string& description) {
    ++g_checks;
    if (condition) {
        std::printf("  ok   %s\n", description.c_str());
    } else {
        ++g_failures;
        std::printf("  FAIL %s\n", description.c_str());
    }
}

inline int summary(const char* suite) {
    std::printf("%s: %d check(s), %d failure(s)\n", suite, g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

}  // namespace pandora::test

#define EXPECT(condition) ::pandora::test::expect((condition), #condition)

#define EXPECT_EQ(expected, actual) \
    ::pandora::test::expect((expected) == (actual), #actual " == " #expected)

#define EXPECT_STR_EQ(expected, actual)               \
    ::pandora::test::expect(                          \
        std::string(expected) == std::string(actual), \
        std::string(#actual " == ") + #expected + " (got '" + std::string(actual) + "')")
