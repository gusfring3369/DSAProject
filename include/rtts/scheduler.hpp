// scheduler.hpp
// Modules 2 to 5: the simulator.
//
//     Sleep queue (Module 2) - tasks waiting for their next release, key = wake-up time
//     Ready queue (Module 3) - released jobs waiting for the CPU, key = period (RM)
//                                                         or absolute deadline (EDF)
//     Dispatcher  (Module 4) - every tick, run the top job of the Ready queue for 1 tick
//     Completion  (Module 5) - when a job ends, check its deadline, send the task to sleep
//
// One tick of the simulation:
//     1. release:  move every task whose wake-up time has come from Sleep to Ready
//     2. snapshot: (optional) remember what both queues look like, for visualisation
//     3. dispatch: run Ready.peek() for one tick; if its job is done, complete it
//
// Both queues come from makeQueue(), so the SAME code runs with binary heaps,
// Fibonacci heaps or sorted lists. Only the queue name changes.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rtts/metrics.hpp"
#include "rtts/queues/task_queue.hpp"
#include "rtts/task.hpp"

namespace rtts {

enum class Policy { RM, EDF };

std::string policyName(Policy p);
Policy parsePolicy(const std::string& text);   // "RM" or "EDF"

// What the system looked like at one tick (taken after releases, before dispatch).
struct TickSnapshot {
    int time = 0;
    QueueView sleep;
    QueueView ready;
    int running = -1;                  // task that ran during this tick, -1 = idle
    std::vector<std::string> events;   // human-readable log lines
    long long comparisons = 0;         // work done by both queues during this tick
};

class Simulator {
public:
    Simulator(const TaskSet& tasks, Policy policy, const std::string& queueKind,
              bool recordSnapshots = false);

    // Not copyable: the queues hold pointers into tasks_.
    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    const Metrics& run(int horizon = 0);   // 0 = one hyperperiod
    void step();                           // simulate exactly one tick
    void finish();                         // count unfinished jobs as misses

    int time() const { return time_; }
    Policy policy() const { return policy_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<TickSnapshot>& snapshots() const { return snapshots_; }
    const TaskSet& tasks() const { return tasks_; }
    const TaskQueue& sleepQueue() const { return *sleep_; }
    const TaskQueue& readyQueue() const { return *ready_; }

private:
    void releaseDueJobs(int t, std::vector<std::string>& events);   // Module 2 -> 3
    int dispatch(int t, std::vector<std::string>& events);          // Module 4
    void completeJob(Task* task, int t, std::vector<std::string>& events);   // Module 5
    Task* findTask(int id);

    TaskSet tasks_;                       // our own fresh copies
    Policy policy_;
    std::unique_ptr<TaskQueue> sleep_;
    std::unique_ptr<TaskQueue> ready_;
    Metrics metrics_;
    std::vector<TickSnapshot> snapshots_;
    bool recordSnapshots_;
    int time_ = 0;
};

}  // namespace rtts
