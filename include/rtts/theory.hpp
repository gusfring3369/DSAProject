// theory.hpp
// Module 7: Theory tests.
//
// These say, WITHOUT simulating, whether a task set will meet all deadlines.
//
//     Test          Policy   Kind
//     -----------   ------   ------------------------------------------------
//     Liu-Layland   RM       sufficient: "yes" is always right, "no" may be wrong
//     Hyperbolic    RM       sufficient, a bit less pessimistic than Liu-Layland
//     RTA           RM       exact (response-time analysis)
//     U <= 1        EDF      exact when D = T
//
// All our task sets have D = T, so the EDF test is exact.

#pragma once

#include <map>

#include "rtts/task.hpp"

namespace rtts {

struct TheoryResult {
    bool liuLayland = false;
    bool hyperbolic = false;
    bool rta = false;
    bool edf = false;
};

double liuLaylandBound(int n);                 // n (2^(1/n) - 1)
bool liuLaylandTest(const TaskSet& tasks);

double hyperbolicProduct(const TaskSet& tasks);  // PRODUCT (U_i + 1)
bool hyperbolicTest(const TaskSet& tasks);

// Worst-case response time of every task under RM: task id -> R_i,
// or -1 if the task can miss its deadline.
std::map<int, int> responseTimes(const TaskSet& tasks);
bool rtaTest(const TaskSet& tasks);

bool edfTest(const TaskSet& tasks);

TheoryResult runAllTests(const TaskSet& tasks);

}  // namespace rtts
