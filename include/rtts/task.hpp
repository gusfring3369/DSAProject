// task.hpp
// The Task model.
//
// A periodic real-time task is described by three numbers:
//
//     T (period)   - a new job is released every T ticks
//     C (wcet)     - each job needs C ticks of CPU time
//     D (deadline) - each job must finish within D ticks of its release
//
// On top of that, a task carries the state of its *current job* (release time,
// absolute deadline, work left). The scheduler updates those fields while the
// simulation runs.

#pragma once

#include <string>
#include <vector>

namespace rtts {

struct Task {
    int id = 0;
    int period = 1;     // T
    int wcet = 1;       // C, worst-case execution time
    int deadline = 1;   // D, relative deadline (D = T in all our experiments)

    // State of the current job, filled in by the scheduler.
    int release = 0;       // r, when the current job was released
    int absDeadline = 0;   // d = r + D
    int remaining = 0;     // ticks of work left for the current job
    int wakeUpTime = 0;    // when the task leaves the Sleep queue

    // Where the task sits inside a binary heap (-1 = not in one). This is the
    // "index" of the indexed heap: the heap updates it on every swap, so the
    // heap can find any task in O(1). A task is in at most one binary heap at
    // a time (it is either asleep or ready).
    int heapPos = -1;

    Task() = default;
    Task(int id, int period, int wcet, int deadline)
        : id(id), period(period), wcet(wcet), deadline(deadline) {}

    // U_i = C / T, the share of the CPU this task needs.
    double utilization() const { return static_cast<double>(wcet) / period; }

    std::string name() const { return "T" + std::to_string(id); }

    // Same T, C, D with all job state cleared. Every simulation works on fresh
    // copies, so running RM and then EDF never leaks state between runs.
    Task freshCopy() const { return Task(id, period, wcet, deadline); }
};

using TaskSet = std::vector<Task>;

}  // namespace rtts
