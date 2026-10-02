#pragma once
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace surv_test {

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failures() {
    static int f = 0;
    return f;
}

inline int& assertions() {
    static int a = 0;
    return a;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline void reportFailure(const char* file, int line, const std::string& msg) {
    std::printf("FAIL %s:%d: %s\n", file, line, msg.c_str());
    failures()++;
}

#define CHECK(cond)                                                                          \
    do {                                                                                     \
        assertions()++;                                                                      \
        if (!(cond)) {                                                                       \
            surv_test::reportFailure(__FILE__, __LINE__, "CHECK(" #cond ")");                \
        }                                                                                    \
    } while (0)

#define CHECK_EQ(a, b)                                                                       \
    do {                                                                                     \
        assertions()++;                                                                      \
        const auto va = (a);                                                                 \
        const auto vb = (b);                                                                 \
        if (!(va == vb)) {                                                                   \
            surv_test::reportFailure(__FILE__, __LINE__,                                     \
                                     "CHECK_EQ(" #a ", " #b ")");                            \
        }                                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                                \
    do {                                                                                     \
        assertions()++;                                                                      \
        const auto va = (a);                                                                 \
        const auto vb = (b);                                                                 \
        if (std::fabs(static_cast<double>(va) - static_cast<double>(vb)) > (eps)) {          \
            surv_test::reportFailure(__FILE__, __LINE__,                                     \
                                     "CHECK_NEAR(" #a ", " #b ", " #eps ")");                \
        }                                                                                    \
    } while (0)

inline int runAll(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : "";
    int ran = 0;
    for (auto& test : registry()) {
        if (filter[0] != '\0' && std::string(test.name).find(filter) == std::string::npos) {
            continue;
        }
        std::printf("RUN  %s\n", test.name);
        std::fflush(stdout);
        try {
            test.fn();
        } catch (const std::exception& e) {
            std::printf("THROW %s: %s\n", test.name, e.what());
            failures()++;
        } catch (...) {
            std::printf("THROW %s: unknown\n", test.name);
            failures()++;
        }
        std::fflush(stdout);
        ran++;
    }
    std::printf("\n%d assertions, %d failures, %d tests\n", assertions(), failures(), ran);
    std::fflush(stdout);
    return failures() == 0 ? 0 : 1;
}

} // namespace surv_test

#define TEST(name)                                                                           \
    static void test_##name();                                                               \
    static surv_test::Registrar reg_##name(#name, test_##name);                              \
    static void test_##name()