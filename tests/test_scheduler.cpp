// test_scheduler.cpp
// TC17 - TC21: the simulator (Modules 2 - 6), checked against the worked
// example on the slides.

#include <string>

#include "rtts/generator.hpp"
#include "rtts/queues/queue_factory.hpp"
#include "rtts/scheduler.hpp"
#include "test_framework.hpp"

using namespace rtts;

namespace {

// Gantt chart as a string, one character per tick ('.' = idle), easy to compare.
std::string ganttString(const Metrics& m) {
    std::string s;
    for (int id : m.gantt) s += id < 0 ? '.' : static_cast<char>('0' + id);
    return s;
}

}  // namespace

TEST(TC17_worked_example_under_rm) {
    // Slide "Worked Example (Set 0 under RM)":
    //   at t = 10 Task 0 preempts Task 2, and Task 2 finishes at 28.
    for (const std::string& queue : allQueueKinds()) {
        Simulator sim(sampleTaskSet(), Policy::RM, queue);
        const Metrics& m = sim.run();
        CHECK_EQ(ganttString(m), std::string("0011111222002222222200111112..00........"));
        CHECK_EQ(m.missCount(), 0);
        CHECK_EQ(m.worstResponse.at(2), 28);
        CHECK_EQ(m.jobsFinished, 4 + 2 + 1);
    }
}

TEST(TC18_edf_schedules_what_rm_cannot) {
    TaskSet tasks = {Task(0, 4, 2, 4), Task(1, 10, 5, 10)};   // U = 1.0
    Simulator rm(tasks, Policy::RM, "binary");
    Simulator edf(tasks, Policy::EDF, "fibonacci");
    CHECK(rm.run().missCount() > 0);
    CHECK_EQ(edf.run().missCount(), 0);
    CHECK_EQ(edf.metrics().idleTicks, 0);   // U = 1: the CPU is never idle
}

TEST(TC19_overload_is_detected) {
    TaskSet tasks = {Task(0, 10, 6, 10), Task(1, 20, 10, 20)};   // U = 1.1
    for (const std::string& queue : allQueueKinds()) {
        Simulator sim(tasks, Policy::EDF, queue);
        CHECK(sim.run().missCount() > 0);
    }
}

TEST(TC20_snapshots_show_the_queues) {
    Simulator sim(sampleTaskSet(), Policy::RM, "fibonacci", true);
    sim.run();
    CHECK_EQ(sim.snapshots().size(), 40u);

    // Tick 0: all three tasks were just released, so Sleep is empty and the
    // Ready heap holds all of them with T0 on top.
    const TickSnapshot& first = sim.snapshots()[0];
    CHECK(first.sleep.roots.empty());
    CHECK_EQ(first.ready.topId, 0);
    CHECK_EQ(first.ready.roots.size(), 3u);   // lazy inserts: three separate trees
    CHECK_EQ(first.running, 0);
}

TEST(TC21_same_schedule_with_every_queue) {
    Random rng(11);
    for (int i = 0; i < 50; ++i) {
        TaskSet tasks = generateTaskSet(8, 0.9, rng);
        for (Policy policy : {Policy::RM, Policy::EDF}) {
            Simulator a(tasks, policy, "binary"), b(tasks, policy, "fibonacci"),
                      c(tasks, policy, "sorted_list"), d(tasks, policy, "rb_tree");
            CHECK(a.run().gantt == b.run().gantt);
            CHECK(a.metrics().gantt == c.run().gantt);
            CHECK(a.metrics().gantt == d.run().gantt);
        }
    }
}
