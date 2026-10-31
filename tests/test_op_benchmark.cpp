// test_op_benchmark.cpp
// TC26: the operation benchmark measures what the theory predicts.

#include <cmath>

#include "rtts/op_benchmark.hpp"
#include "test_framework.hpp"

using namespace rtts;

TEST(TC26_operation_costs_follow_the_theory) {
    OperationConfig config;
    config.sizes = {1000};
    config.repeats = 1;
    auto rows = runOperationBenchmark(config);
    CHECK_EQ(rows.size(), 4u * 4u);   // 4 queues x 4 operations

    auto get = [&](const std::string& queue, const std::string& op) {
        for (const OperationRow& r : rows)
            if (r.queue == queue && r.operation == op) return r;
        return OperationRow{};
    };
    const double log2n = std::log2(1000.0);

    // Fibonacci insert: exactly one comparison (with the min pointer), never more.
    CHECK(get("fibonacci", "insert").worstComparisons == 1);
    // Binary heap insert: at most one comparison per level.
    CHECK(get("binary", "insert").worstComparisons <= static_cast<long long>(log2n) + 1);
    // Sorted list insert: O(N), the worst insert walks most of the list.
    CHECK(get("sorted_list", "insert").worstComparisons > 500);
    // Fibonacci extract-min: the first one after 1000 lazy inserts has to
    // link the ~1000 one-node roots, so the worst case is O(N) ...
    CHECK(get("fibonacci", "extract_min").worstComparisons > 500);
    // ... but the average stays O(log N).
    CHECK(get("fibonacci", "extract_min").avgComparisons < 3 * log2n);
    // Fibonacci decrease-key: at most 2 comparisons (with the parent and with
    // the min); cuts move pointers but compare nothing. Binary heap: one
    // comparison per level it climbs, at most log2 N.
    CHECK(get("fibonacci", "decrease_key").worstComparisons <= 2);
    CHECK(get("binary", "decrease_key").worstComparisons <= static_cast<long long>(log2n) + 1);
    // Every operation was actually measured.
    for (const OperationRow& r : rows) CHECK(r.ops > 0);
}
