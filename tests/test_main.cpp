// test_main.cpp
// Runs every registered TEST in order of its test-case id (TC01, TC02, ...)
// and prints a short report.

#include <algorithm>

#include "test_framework.hpp"

int main() {
    // Tests from different .cpp files register in an unspecified order,
    // so sort them by name; the TCxx prefix gives the intended order.
    auto tests = testing::registry();
    std::sort(tests.begin(), tests.end(),
              [](const testing::TestCase& a, const testing::TestCase& b) { return a.name < b.name; });

    int failedTests = 0;
    for (const auto& test : tests) {
        int before = testing::failures();
        test.body();
        bool ok = testing::failures() == before;
        if (!ok) ++failedTests;
        std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << test.name << "\n";
    }
    std::cout << "\n" << tests.size() - failedTests << " / " << tests.size() << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
