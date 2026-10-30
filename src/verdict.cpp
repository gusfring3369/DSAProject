// verdict.cpp

#include "rtts/verdict.hpp"

#include <map>

#include "rtts/scheduler.hpp"
#include "rtts/theory.hpp"

namespace rtts {

static std::string yesNo(bool b) { return b ? "yes" : "no"; }

Verdict checkTaskSet(const TaskSet& tasks, const std::vector<std::string>& queues) {
    Verdict v;
    TheoryResult theory = runAllTests(tasks);
    std::map<int, int> rtaR = responseTimes(tasks);

    v.liuLayland = theory.liuLayland;
    v.hyperbolic = theory.hyperbolic;
    v.rta = theory.rta;
    v.edf = theory.edf;

    // gantts[policy][queue] = schedule produced by that run
    std::map<Policy, std::map<std::string, std::vector<int>>> gantts;

    for (const std::string& queue : queues) {
        bool noMisses[2] = {false, false};

        for (Policy policy : {Policy::RM, Policy::EDF}) {
            Simulator sim(tasks, policy, queue);
            const Metrics& m = sim.run();
            noMisses[policy == Policy::RM ? 0 : 1] = (m.missCount() == 0);
            gantts[policy][queue] = m.gantt;

            // Timing check: simulated worst response time == RTA's R_i.
            if (policy == Policy::RM && theory.rta) {
                for (const auto& [taskId, r] : rtaR) {
                    int simulated = m.worstResponse.at(taskId);
                    if (simulated != r)
                        v.mismatches.push_back("[" + queue + "] T" + std::to_string(taskId) +
                                               ": simulated response " + std::to_string(simulated) +
                                               ", RTA says " + std::to_string(r));
                }
            }
        }

        bool rmOk = noMisses[0], edfOk = noMisses[1];
        if (queue == queues.front()) { v.rmSim = rmOk; v.edfSim = edfOk; }

        if (rmOk != theory.rta)
            v.mismatches.push_back("[" + queue + "] RM simulation schedulable=" + yesNo(rmOk) +
                                   " but RTA says " + yesNo(theory.rta));
        if (edfOk != theory.edf)
            v.mismatches.push_back("[" + queue + "] EDF simulation schedulable=" + yesNo(edfOk) +
                                   " but U <= 1 test says " + yesNo(theory.edf));
        if ((theory.liuLayland || theory.hyperbolic) && !rmOk)
            v.mismatches.push_back("[" + queue + "] a sufficient test passed but RM missed a deadline");
    }

    // Every queue type must produce exactly the same schedule.
    for (Policy policy : {Policy::RM, Policy::EDF}) {
        const auto& reference = gantts[policy][queues.front()];
        for (std::size_t i = 1; i < queues.size(); ++i) {
            if (gantts[policy][queues[i]] != reference) {
                v.sameSchedule = false;
                v.mismatches.push_back(policyName(policy) + ": " + queues[i] +
                                       " schedule differs from " + queues.front());
            }
        }
    }
    return v;
}

}  // namespace rtts
