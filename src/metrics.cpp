// metrics.cpp

#include "rtts/metrics.hpp"

#include <algorithm>

namespace rtts {

Metrics::Metrics(const TaskSet& tasks) {
    for (const Task& t : tasks) worstResponse[t.id] = 0;
}

void Metrics::recordRelease(const Task& task, int time) {
    releases.emplace_back(time, task.id);
}

void Metrics::recordIdle(int /*time*/) {
    ++idleTicks;
    gantt.push_back(-1);
}

void Metrics::recordRun(const Task& task, int /*time*/) {
    if (task.id != lastRunning) {
        ++contextSwitches;
        lastRunning = task.id;
    }
    gantt.push_back(task.id);
}

void Metrics::recordFinish(const Task& task, int finishTime) {
    // Called when a job completes at the END of a tick (finishTime = t + 1).
    finishes.emplace_back(finishTime, task.id);
    if (finishTime > task.absDeadline)
        misses.push_back({task.id, task.release, task.absDeadline, finishTime});

    int response = finishTime - task.release;
    totalResponse += response;
    ++jobsFinished;
    worstResponse[task.id] = std::max(worstResponse[task.id], response);
}

void Metrics::recordTickCost(long long comparisons) {
    tickCost.push_back(comparisons);
}

void Metrics::endOfRun(int horizon, const std::vector<Task*>& unfinished) {
    // A job still waiting at the end whose deadline has already passed is a miss too.
    for (const Task* t : unfinished)
        if (t->absDeadline <= horizon) misses.push_back({t->id, t->release, t->absDeadline, -1});
}

double Metrics::averageResponse() const {
    return jobsFinished ? static_cast<double>(totalResponse) / jobsFinished : 0.0;
}

double Metrics::cpuBusyPercent() const {
    return gantt.empty() ? 0.0 : 100.0 * (gantt.size() - idleTicks) / gantt.size();
}

double Metrics::averageTickCost() const {
    if (tickCost.empty()) return 0.0;
    long long sum = 0;
    for (long long c : tickCost) sum += c;
    return static_cast<double>(sum) / tickCost.size();
}

long long Metrics::worstTickCost() const {
    return tickCost.empty() ? 0 : *std::max_element(tickCost.begin(), tickCost.end());
}

}  // namespace rtts
