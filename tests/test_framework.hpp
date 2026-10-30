// test_framework.hpp
// A deliberately tiny unit-test helper, so the project needs no external
// libraries. Usage:
//
//     TEST(binary_heap_sorts) {
//         CHECK(1 + 1 == 2);
//         CHECK_EQ(heap.size(), 3u);
//     }
//
// Every TEST registers itself; test_main.cpp runs them all and reports.

#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace testing {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> body) {
        registry().push_back({name, std::move(body)});
    }
};

}  // namespace testing

#define TEST(name)                                                        \
    static void test_##name();                                            \
    static testing::Registrar registrar_##name(#name, test_##name);       \
    static void test_##name()

#define CHECK(condition)                                                              \
    do {                                                                              \
        if (!(condition)) {                                                           \
            ++testing::failures();                                                    \
            std::cout << "    FAILED: " << #condition << "  (" << __FILE__ << ":"     \
                      << __LINE__ << ")\n";                                           \
        }                                                                             \
    } while (0)

#define CHECK_EQ(actual, expected)                                                    \
    do {                                                                              \
        auto a_ = (actual);                                                           \
        auto e_ = (expected);                                                         \
        if (!(a_ == e_)) {                                                            \
            ++testing::failures();                                                    \
            std::cout << "    FAILED: " << #actual << " == " << #expected             \
                      << "  got " << a_ << ", expected " << e_ << "  (" << __FILE__   \
                      << ":" << __LINE__ << ")\n";                                    \
        }                                                                             \
    } while (0)
