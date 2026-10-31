// test_theory.cpp
// TC11 - TC16: the theory tests (Module 7) and the task generator (Module 1),
// checked against the numbers on the "Dataset" slide.

#include <cmath>

#include "rtts/generator.hpp"
#include "rtts/theory.hpp"
#include "test_framework.hpp"

using namespace rtts;

TEST(TC11_sample_set_matches_the_slides) {
    TaskSet tasks = sampleTaskSet();
    CHECK(std::abs(totalUtilization(tasks) - 0.75) < 1e-9);
    CHECK(std::abs(liuLaylandBound(3) - 0.7798) < 1e-3);
    CHECK(liuLaylandTest(tasks));                                  // 0.75 <= 0.780
    CHECK(std::abs(hyperbolicProduct(tasks) - 1.95) < 1e-9);       // 1.2 * 1.25 * 1.3
    CHECK(hyperbolicTest(tasks));
    CHECK(edfTest(tasks));
    CHECK(rtaTest(tasks));

    auto r = responseTimes(tasks);                                  // R = 2, 7, 28
    CHECK_EQ(r[0], 2);
    CHECK_EQ(r[1], 7);
    CHECK_EQ(r[2], 28);
}

TEST(TC12_rta_detects_a_miss) {
    // U = 0.5 + 0.5 = 1.0: fine for EDF, but under RM T1 needs R = 11 > D = 10.
    TaskSet tasks = {Task(0, 4, 2, 4), Task(1, 10, 5, 10)};
    CHECK(edfTest(tasks));
    CHECK(!rtaTest(tasks));
    CHECK_EQ(responseTimes(tasks)[1], -1);
}

TEST(TC13_liu_layland_is_only_sufficient) {
    // Harmonic periods: U = 1.0 is schedulable by RM, but Liu-Layland says no.
    TaskSet tasks = {Task(0, 10, 5, 10), Task(1, 20, 10, 20)};
    CHECK(!liuLaylandTest(tasks));
    CHECK(rtaTest(tasks));
}

TEST(TC14_edf_test_is_exact_at_u_equal_one) {
    TaskSet full = {Task(0, 10, 5, 10), Task(1, 25, 5, 25), Task(2, 50, 15, 50)};   // 0.5 + 0.2 + 0.3
    CHECK(edfTest(full));
    TaskSet over = {Task(0, 10, 5, 10), Task(1, 25, 5, 25), Task(2, 50, 16, 50)};
    CHECK(!edfTest(over));
}

TEST(TC15_uunifast_sums_to_target) {
    Random rng(9);
    for (int n : {2, 5, 20}) {
        auto u = uunifast(n, 0.8, rng);
        double sum = 0;
        for (double x : u) { sum += x; CHECK(x >= 0); }
        CHECK(std::abs(sum - 0.8) < 1e-9);
    }
}

TEST(TC16_hyperperiod_is_lcm) {
    CHECK_EQ(hyperperiod(sampleTaskSet()), 40L);
    TaskSet tasks = {Task(0, 25, 1, 25), Task(1, 40, 1, 40)};
    CHECK_EQ(hyperperiod(tasks), 200L);
}
