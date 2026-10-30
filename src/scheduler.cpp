// scheduler.cpp

#include "rtts/scheduler.hpp"

#include <stdexcept>

#include "rtts/generator.hpp"
#include "rtts/queues/queue_factory.hpp"

namespace rtts {

// ---------------------------------------------------------------------- keys
// Smaller key = served first. TaskQueue::before() breaks ties by task id.

static long sleepKey(const Task& t) { return t.wakeUpTime; }
static long rmKey(const Task& t) { return t.period; }
static long edfKey(const Task& t) { return t.absDeadline; }

std::string policyName(Policy p) { return p == Policy::RM ? "RM" : "EDF"; }

Policy parsePolicy(const std::string& text) {
    if (text == "RM" || text == "rm") return Policy::RM;
    if (text == "EDF" || text == "edf") return Policy::EDF;
    throw std::invalid_argument("unknown policy '" + text + "' (use RM or EDF)");
}

// ----------------------------------------------------------------- simulator

Simulator::Simulator(const TaskSet& tasks, Policy policy, const std::string& queueKind,
                     bool recordSnapshots)
    : policy_(policy),
      sleep_(makeQueue(queueKind, sleepKey)),
      ready_(makeQueue(queueKind, policy == Policy::RM ? rmKey : edfKey)),
      metrics_(tasks),
      recordSnapshots_(recordSnapshots) {
    // Work on fresh copies so the caller's tasks are never modified.
    // reserve() first: the queues keep pointers into this vector, so it must
    // never reallocate.
    tasks_.reserve(tasks.size());
    for (const Task& t : tasks) tasks_.push_back(t.freshCopy());

    // At t = 0 every task is asleep with wake-up time 0, so they are all
    // released together on the first tick (the "critical instant").
    for (Task& t : tasks_) {
        t.wakeUpTime = 0;
        sleep_->insert(&t);
    }
}

const Metrics& Simulator::run(int horizon) {
    if (horizon <= 0) horizon = static_cast<int>(hyperperiod(tasks_));
    while (time_ < horizon) step();
    finish();
    return metrics_;
}

void Simulator::finish() {
    metrics_.endOfRun(time_, ready_->tasks());
}

void Simulator::step() {
    const int t = time_;
    const long long workBefore = sleep_->comparisons() + ready_->comparisons();
    std::vector<std::string> events;

    releaseDueJobs(t, events);

    TickSnapshot snap;
    if (recordSnapshots_) {
        snap.time = t;
        snap.sleep = sleep_->view();
        snap.ready = ready_->view();
    }

    int running = dispatch(t, events);

    const long long work = sleep_->comparisons() + ready_->comparisons() - workBefore;
    metrics_.recordTickCost(work);

    if (recordSnapshots_) {
        snap.running = running;
        snap.events = events;
        snap.comparisons = work;
        snapshots_.push_back(std::move(snap));
    }
    ++time_;
}

Task* Simulator::findTask(int id) {
    for (Task& t : tasks_)
        if (t.id == id) return &t;
    return nullptr;
}

// ------------------------------------------------- Modules 2 -> 3: release

void Simulator::releaseDueJobs(int t, std::vector<std::string>& events) {
    while (!sleep_->empty() && sleep_->peek()->wakeUpTime <= t) {
        Task* task = sleep_->extractMin();

        // Start a new job for this task.
        task->release = task->wakeUpTime;
        task->absDeadline = task->release + task->deadline;
        task->remaining = task->wcet;

        ready_->insert(task);
        metrics_.recordRelease(*task, task->release);
        events.push_back(task->name() + " released (deadline " + std::to_string(task->absDeadline) + ")");
    }
}

// ---------------------------------------------------- Module 4: dispatcher

int Simulator::dispatch(int t, std::vector<std::string>& events) {
    if (ready_->empty()) {
        metrics_.recordIdle(t);
        events.push_back("CPU idle");
        return -1;
    }

    Task* task = ready_->peek();

    // Preemption needs no special code: a more urgent job is simply the new
    // top of the Ready queue. We only detect it here to write it in the log.
    Task* previous = findTask(metrics_.lastRunning);
    if (previous != nullptr && previous != task && ready_->contains(previous) && previous->remaining > 0)
        events.push_back(task->name() + " preempts " + previous->name());

    metrics_.recordRun(*task, t);
    task->remaining -= 1;   // 'remaining' is not part of any key, so the heap stays valid

    if (task->remaining == 0) completeJob(task, t, events);
    return task->id;
}

// ------------------------------------------------ Module 5: job completion

void Simulator::completeJob(Task* task, int t, std::vector<std::string>& events) {
    ready_->extractMin();          // the running task is always the top
    const int finish = t + 1;      // it finishes at the END of tick t
    metrics_.recordFinish(*task, finish);

    bool late = finish > task->absDeadline;
    events.push_back(task->name() + " finished at " + std::to_string(finish) +
                     (late ? "  ** DEADLINE MISSED **" : ""));

    // Sleep until the next release. If the job was late, that time may have
    // already passed; the task is then released on the very next tick.
    task->wakeUpTime = task->release + task->period;
    sleep_->insert(task);
}

}  // namespace rtts
