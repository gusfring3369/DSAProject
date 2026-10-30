// theory.cpp

#include "rtts/theory.hpp"

#include <algorithm>
#include <cmath>

#include "rtts/generator.hpp"

namespace rtts {

double liuLaylandBound(int n) {
    // 1.0 for one task, ~0.828 for two, and it falls towards ln 2 ~ 0.693.
    return n * (std::pow(2.0, 1.0 / n) - 1.0);
}

bool liuLaylandTest(const TaskSet& tasks) {
    return totalUtilization(tasks) <= liuLaylandBound(static_cast<int>(tasks.size()));
}

double hyperbolicProduct(const TaskSet& tasks) {
    double product = 1.0;
    for (const Task& t : tasks) product *= 1.0 + t.utilization();
    return product;
}

bool hyperbolicTest(const TaskSet& tasks) {
    // The tiny tolerance stops a product of exactly 2 (like 1.25 * 1.6) from
    // failing because of floating-point rounding.
    return hyperbolicProduct(tasks) <= 2.0 + 1e-9;
}

std::map<int, int> responseTimes(const TaskSet& tasks) {
    // RM priority order: shorter period first, ties broken by smaller id.
    TaskSet ordered = tasks;
    std::sort(ordered.begin(), ordered.end(), [](const Task& a, const Task& b) {
        return a.period != b.period ? a.period < b.period : a.id < b.id;
    });

    // R_i is the smallest fixed point of
    //     R = C_i + SUM over higher-priority j of ceil(R / T_j) * C_j
    // Start at R = C_i and iterate until R stops changing or passes D_i.
    std::map<int, int> result;
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const Task& task = ordered[i];
        int r = task.wcet;
        while (true) {
            int interference = 0;
            for (std::size_t j = 0; j < i; ++j) {
                int releases = (r + ordered[j].period - 1) / ordered[j].period;   // ceil(r / T_j)
                interference += releases * ordered[j].wcet;
            }
            int rNew = task.wcet + interference;
            if (rNew > task.deadline) { result[task.id] = -1; break; }
            if (rNew == r) { result[task.id] = r; break; }
            r = rNew;
        }
    }
    return result;
}

bool rtaTest(const TaskSet& tasks) {
    for (const auto& entry : responseTimes(tasks))
        if (entry.second < 0) return false;
    return true;
}

bool edfTest(const TaskSet& tasks) {
    // Whole numbers only: the work demanded in one hyperperiod H is
    // SUM C_i * (H / T_i), and it must fit in H ticks. Same as U <= 1,
    // but never misjudged by rounding when U is exactly 1.
    long h = hyperperiod(tasks);
    long work = 0;
    for (const Task& t : tasks) work += static_cast<long>(t.wcet) * (h / t.period);
    return work <= h;
}

TheoryResult runAllTests(const TaskSet& tasks) {
    TheoryResult r;
    r.liuLayland = liuLaylandTest(tasks);
    r.hyperbolic = hyperbolicTest(tasks);
    r.rta = rtaTest(tasks);
    r.edf = edfTest(tasks);
    return r;
}

}  // namespace rtts
