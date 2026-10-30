// generator.hpp
// Module 1: Task generator.
//
// Random task sets are made with UUniFast (Bini & Buttazzo, 2005), the standard
// method in real-time scheduling research. It splits a total utilization U
// into n random pieces that always add up to exactly U.

#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "rtts/task.hpp"

namespace rtts {

// Periods are picked from this set. Every value divides 200, so the
// hyperperiod (LCM of all periods) is at most 200 ticks.
const std::vector<int>& defaultPeriods();

// Bigger periods for the benchmark, so even 1000 tasks with C >= 1 tick
// stay under 100% CPU. They all divide 20000, so H is at most 20000 ticks.
const std::vector<int>& benchmarkPeriods();

// A small, portable random number source. std::mt19937 produces the same
// numbers on every compiler, and we do the conversions ourselves, so a seed
// gives the same task sets on Windows, Linux and macOS.
class Random {
public:
    explicit Random(std::uint32_t seed) : engine_(seed) {}
    double uniform01() { return engine_() / 4294967296.0; }               // [0, 1)
    std::size_t index(std::size_t size) { return engine_() % size; }      // [0, size)

private:
    std::mt19937 engine_;
};

std::vector<double> uunifast(int n, double totalU, Random& rng);

TaskSet generateTaskSet(int n, double totalU, Random& rng,
                        const std::vector<int>& periods = defaultPeriods());

long hyperperiod(const TaskSet& tasks);       // LCM of all periods
double totalUtilization(const TaskSet& tasks);

// Set 0 from the slides: T = 10, 20, 40 and C = 2, 5, 12 (U = 0.75).
TaskSet sampleTaskSet();

}  // namespace rtts
