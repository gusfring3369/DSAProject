// experiments.hpp
// The two experiments we report.
//
// 1. Verdict experiment: run the verdict check (Module 8) over the dataset and
//    count, for every (n, U), how many sets each test and each simulation
//    accepts. Simulator and theory must agree on 100% of the sets.
//
// 2. Heap benchmark: run the same task sets with a sorted list, a binary heap
//    and a Fibonacci heap, and measure the work per tick. We count key
//    comparisons (exact and machine-independent) and also wall-clock time.
//    We keep the AVERAGE and the WORST tick, because a real-time scheduler is
//    judged by its worst tick.
//
// Both take an optional progress(done, total) callback for progress bars.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "rtts/dataset.hpp"
#include "rtts/scheduler.hpp"
#include "rtts/verdict.hpp"

namespace rtts {

using Progress = std::function<void(std::size_t done, std::size_t total)>;

// ------------------------------------------------------------ verdict

struct VerdictRow {
    int n = 0;
    double targetU = 0;
    double realU = 0;      // average utilization after rounding C to whole ticks
    int sets = 0;
    // fraction of sets accepted (0..1)
    double liuLayland = 0, hyperbolic = 0, rta = 0, rmSim = 0, edf = 0, edfSim = 0;
    int mismatches = 0;
};

struct VerdictReport {
    std::vector<VerdictRow> rows;
    std::vector<std::pair<TaskSetRecord, Verdict>> mismatches;
    int totalSets = 0;
    long long simulations = 0;   // task sets x 2 policies x queue types
};

// Every task set runs with all four queue types; they must all give the
// same schedule, and that schedule must agree with the theory.
const std::vector<std::string>& verdictQueues();

VerdictReport runVerdictExperiment(const std::vector<TaskSetRecord>& records,
                                   const Progress& progress = nullptr);

// ---------------------------------------------------------- benchmark

struct BenchmarkRow {
    std::string queue;
    int n = 0;
    double actualU = 0;
    double avgComparisons = 0;     // per tick
    long long worstComparisons = 0;
    double avgMicros = 0;          // wall-clock time per tick
    double worstMicros = 0;
};

struct BenchmarkConfig {
    std::vector<int> sizes = {5, 10, 20, 50, 100, 200, 500, 1000};
    std::vector<std::string> queues = {"sorted_list", "rb_tree", "binary", "fibonacci"};
    int setsPerSize = 5;
    double utilization = 0.7;
    Policy policy = Policy::EDF;
    std::uint32_t seed = 7;
};

std::vector<BenchmarkRow> runBenchmark(const BenchmarkConfig& config,
                                       const Progress& progress = nullptr);

}  // namespace rtts
