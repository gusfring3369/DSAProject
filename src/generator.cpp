// generator.cpp

#include "rtts/generator.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace rtts {

const std::vector<int>& defaultPeriods() {
    static const std::vector<int> periods = {10, 20, 25, 40, 50, 100, 200};
    return periods;
}

const std::vector<int>& benchmarkPeriods() {
    static const std::vector<int> periods = {1000, 2000, 2500, 4000, 5000, 10000, 20000};
    return periods;
}

std::vector<double> uunifast(int n, double totalU, Random& rng) {
    std::vector<double> utils;
    double remaining = totalU;
    for (int i = 1; i < n; ++i) {
        // The exponent 1/(n - i) is what makes every split equally likely.
        double next = remaining * std::pow(rng.uniform01(), 1.0 / (n - i));
        utils.push_back(remaining - next);
        remaining = next;
    }
    utils.push_back(remaining);
    return utils;
}

TaskSet generateTaskSet(int n, double totalU, Random& rng, const std::vector<int>& periods) {
    // C must be a whole number of ticks, so we round u_i * T. The real
    // utilization is then close to totalU, not exactly equal. The theory
    // tests always use the real C and T, so the verdict check is not affected.
    TaskSet tasks;
    std::vector<double> utils = uunifast(n, totalU, rng);
    for (int id = 0; id < n; ++id) {
        int period = periods[rng.index(periods.size())];
        int wcet = std::max(1, static_cast<int>(std::lround(utils[id] * period)));
        tasks.emplace_back(id, period, wcet, period);   // D = T
    }
    return tasks;
}

long hyperperiod(const TaskSet& tasks) {
    long h = 1;
    for (const Task& t : tasks) h = std::lcm(h, static_cast<long>(t.period));
    return h;
}

double totalUtilization(const TaskSet& tasks) {
    double u = 0;
    for (const Task& t : tasks) u += t.utilization();
    return u;
}

TaskSet sampleTaskSet() {
    return {Task(0, 10, 2, 10), Task(1, 20, 5, 20), Task(2, 40, 12, 40)};
}

}  // namespace rtts
