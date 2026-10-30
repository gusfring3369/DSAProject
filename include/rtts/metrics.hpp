// metrics.hpp
// Module 6: Metrics and log.
//
// The scheduler calls the record* functions as things happen. This class keeps
// everything we later want to show: the Gantt chart, deadline misses,
// response times, context switches, and how much work the queues did per tick.

#pragma once

#include <map>
#include <utility>
#include <vector>

#include "rtts/task.hpp"

namespace rtts {

struct Miss {
    int taskId;
    int release;
    int deadline;
    int finish;    // -1 = still unfinished when the simulation ended
};

class Metrics {
public:
    explicit Metrics(const TaskSet& tasks);

    void recordRelease(const Task& task, int time);
    void recordIdle(int time);
    void recordRun(const Task& task, int time);
    void recordFinish(const Task& task, int finishTime);
    void recordTickCost(long long comparisons);
    void endOfRun(int horizon, const std::vector<Task*>& unfinished);

    // ---- results
    std::vector<int> gantt;                       // gantt[t] = task id that ran at tick t, -1 = idle
    std::vector<std::pair<int, int>> releases;    // (time, task id), the ▲ markers
    std::vector<std::pair<int, int>> finishes;    // (time, task id)
    std::vector<Miss> misses;
    std::vector<long long> tickCost;              // key comparisons done during each tick
    std::map<int, int> worstResponse;             // task id -> worst response time
    int idleTicks = 0;
    int contextSwitches = 0;
    int jobsFinished = 0;
    long totalResponse = 0;
    int lastRunning = -1;                         // task that ran in the previous tick

    int missCount() const { return static_cast<int>(misses.size()); }
    double averageResponse() const;
    double cpuBusyPercent() const;
    double averageTickCost() const;
    long long worstTickCost() const;
};

}  // namespace rtts
