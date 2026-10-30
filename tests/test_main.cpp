// test_main.cpp
// Runs every registered TEST and prints a short report.

#include "test_framework.hpp"

int main() {
    int failedTests = 0;
    for (const auto& test : testing::registry()) {
        int before = testing::failures();
        test.body();
        bool ok = testing::failures() == before;
        if (!ok) ++failedTests;
        std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << test.name << "\n";
    }
    std::cout << "\n" << testing::registry().size() - failedTests << " / " << testing::registry().size()
              << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
