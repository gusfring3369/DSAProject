// op_benchmark.hpp
// Operation-level benchmark: the data structures on their own, without the
// scheduler around them.
//
// For every queue type and every size N we measure four operations:
//
//   insert        N inserts into an empty queue (random keys)
//   extract_min   then N extract-mins until the queue is empty
//   decrease_key  N - 1 decrease-keys on a full queue
//   (for small N these rounds are repeated until 100,000 operations are measured)
//   hold          extract-min, then insert the same task with a later key.
//                 This is the classic priority-queue benchmark (the "hold
//                 model", Vaucher & Duval 1975; Jones 1986), and it is exactly
//                 what our Sleep queue does every time a job finishes:
//                 next wake-up = now + period. 100,000 holds per run.
//
// For each operation we record the average and the worst number of key
// comparisons per operation (exact) and the average time per operation
// (median of a few repeats, because timings are noisy).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rtts/experiments.hpp"

namespace rtts {

struct OperationRow {
    std::string queue;
    int n = 0;
    std::string operation;           // insert, extract_min, decrease_key, hold
    long long ops = 0;               // how many operations were measured
    double avgComparisons = 0;
    long long worstComparisons = 0;
    double avgNs = 0;                // nanoseconds per operation
};

struct OperationConfig {
    std::vector<int> sizes = {10, 100, 1000, 10000, 100000};
    std::vector<std::string> queues = {"sorted_list", "rb_tree", "binary", "fibonacci"};
    // The sorted list needs O(N) per insert, so N = 100,000 would take
    // minutes. Above this size it is skipped.
    int sortedListMaxN = 10000;
    int repeats = 3;
    std::uint32_t seed = 42;
};

const std::vector<std::string>& operationNames();

std::vector<OperationRow> runOperationBenchmark(const OperationConfig& config,
                                                const Progress& progress = nullptr);

}  // namespace rtts
