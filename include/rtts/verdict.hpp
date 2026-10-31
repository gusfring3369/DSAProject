// verdict.hpp
// Module 8: Verdict check.
//
// The simulator and the theory must agree. For one task set we:
//   1. run RM and EDF with every queue type we pass in (the experiment uses
//      all four: binary heap, Fibonacci heap, sorted list, red-black tree)
//   2. compare "no deadline misses?" with the exact tests (RTA for RM, U <= 1 for EDF)
//   3. check that the sufficient tests never say "yes" when RM actually misses
//   4. when RM is schedulable, check that each task's worst simulated response
//      time equals the R_i that RTA predicts (the timing check from the slides)
//   5. check that all queue types produce exactly the same schedule
//
// Any disagreement is a mismatch. A mismatch means a bug somewhere, so the
// experiment saves those task sets for debugging.

#pragma once

#include <string>
#include <vector>

#include "rtts/task.hpp"

namespace rtts {

struct Verdict {
    bool liuLayland = false;
    bool hyperbolic = false;
    bool rta = false;
    bool edf = false;
    bool rmSim = false;          // RM simulation had no misses
    bool edfSim = false;         // EDF simulation had no misses
    bool sameSchedule = true;    // every queue type produced the same Gantt chart
    std::vector<std::string> mismatches;

    bool agrees() const { return mismatches.empty(); }
};

Verdict checkTaskSet(const TaskSet& tasks,
                     const std::vector<std::string>& queues = {"binary", "fibonacci"});

}  // namespace rtts
