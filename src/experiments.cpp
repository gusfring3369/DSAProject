// experiments.cpp

#include "rtts/experiments.hpp"

#include <algorithm>
#include <chrono>
#include <map>

#include "rtts/generator.hpp"

namespace rtts {

// ------------------------------------------------------------ verdict

VerdictReport runVerdictExperiment(const std::vector<TaskSetRecord>& records, const Progress& progress) {
    struct Counts { int sets = 0, ll = 0, hyp = 0, rta = 0, rmSim = 0, edf = 0, edfSim = 0, bad = 0; double sumU = 0; };
    std::map<std::pair<int, int>, Counts> counts;   // key: (n, U * 100)

    VerdictReport report;
    for (std::size_t i = 0; i < records.size(); ++i) {
        const TaskSetRecord& rec = records[i];
        Verdict v = checkTaskSet(rec.tasks);

        Counts& c = counts[{rec.n, static_cast<int>(rec.targetU * 100 + 0.5)}];
        c.sets++;
        c.sumU += totalUtilization(rec.tasks);
        c.ll += v.liuLayland;
        c.hyp += v.hyperbolic;
        c.rta += v.rta;
        c.rmSim += v.rmSim;
        c.edf += v.edf;
        c.edfSim += v.edfSim;
        if (!v.agrees()) {
            c.bad++;
            report.mismatches.emplace_back(rec, v);
        }
        if (progress) progress(i + 1, records.size());
    }

    for (const auto& [key, c] : counts) {
        VerdictRow row;
        row.n = key.first;
        row.targetU = key.second / 100.0;
        row.sets = c.sets;
        row.realU = c.sumU / c.sets;
        row.liuLayland = double(c.ll) / c.sets;
        row.hyperbolic = double(c.hyp) / c.sets;
        row.rta = double(c.rta) / c.sets;
        row.rmSim = double(c.rmSim) / c.sets;
        row.edf = double(c.edf) / c.sets;
        row.edfSim = double(c.edfSim) / c.sets;
        row.mismatches = c.bad;
        report.rows.push_back(row);
    }
    report.totalSets = static_cast<int>(records.size());
    return report;
}

// ---------------------------------------------------------- benchmark

std::vector<BenchmarkRow> runBenchmark(const BenchmarkConfig& config, const Progress& progress) {
    using Clock = std::chrono::steady_clock;
    Random rng(config.seed);

    // Generate all task sets first, so every queue type sees exactly the same input.
    std::map<int, std::vector<TaskSet>> taskSets;
    for (int n : config.sizes)
        for (int i = 0; i < config.setsPerSize; ++i)
            taskSets[n].push_back(generateTaskSet(n, config.utilization, rng, benchmarkPeriods()));

    const std::size_t total = config.sizes.size() * config.queues.size() * config.setsPerSize;
    std::size_t done = 0;
    std::vector<BenchmarkRow> rows;

    for (int n : config.sizes) {
        for (const std::string& queue : config.queues) {
            BenchmarkRow row;
            row.queue = queue;
            row.n = n;
            long long sumComparisons = 0, ticks = 0;
            double sumMicros = 0;

            for (const TaskSet& tasks : taskSets[n]) {
                row.actualU += totalUtilization(tasks) / config.setsPerSize;
                Simulator sim(tasks, config.policy, queue);
                const long horizon = hyperperiod(tasks);

                for (long t = 0; t < horizon; ++t) {
                    auto start = Clock::now();
                    sim.step();
                    double micros = std::chrono::duration<double, std::micro>(Clock::now() - start).count();
                    sumMicros += micros;
                    row.worstMicros = std::max(row.worstMicros, micros);
                }
                for (long long c : sim.metrics().tickCost) {
                    sumComparisons += c;
                    row.worstComparisons = std::max(row.worstComparisons, c);
                }
                ticks += horizon;
                if (progress) progress(++done, total);
            }
            row.avgComparisons = double(sumComparisons) / ticks;
            row.avgMicros = sumMicros / ticks;
            rows.push_back(row);
        }
    }
    return rows;
}

}  // namespace rtts
