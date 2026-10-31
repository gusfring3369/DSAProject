// op_benchmark.cpp

#include "rtts/op_benchmark.hpp"

#include <algorithm>
#include <chrono>
#include <map>

#include "rtts/generator.hpp"
#include "rtts/queues/queue_factory.hpp"

namespace rtts {

namespace {

using Clock = std::chrono::steady_clock;

// The benchmark orders tasks by wake-up time, like the Sleep queue.
long wakeUpKey(const Task& t) { return t.wakeUpTime; }

// Keys of the hold test live in a window of this many ticks ahead of "now",
// like wake-up times that are at most one (large) period away.
constexpr int kHoldWindow = 1000000;

// Every measurement covers at least this many operations. For small N one
// round takes only nanoseconds, so we repeat it; that keeps the cost of
// reading the clock (two reads per round) small compared with the work.
constexpr long long kTargetOps = 100000;

// Statistics of one operation type in one run.
struct Stats {
    long long ops = 0;
    long long sumComparisons = 0;
    long long worstComparisons = 0;
    double seconds = 0;
};

// Calls op(i) for i = 0 .. count-1 and records the comparisons of every call
// and the total time.
template <typename Op>
void measure(const TaskQueue& q, Stats& s, long long count, Op op) {
    auto start = Clock::now();
    for (long long i = 0; i < count; ++i) {
        long long before = q.comparisons();
        op(i);
        long long used = q.comparisons() - before;
        s.sumComparisons += used;
        s.worstComparisons = std::max(s.worstComparisons, used);
    }
    s.seconds += std::chrono::duration<double>(Clock::now() - start).count();
    s.ops += count;
}

// One complete run of all four operations for one queue type and size.
std::map<std::string, Stats> runOnce(const std::string& kind, int n, std::uint32_t seed) {
    Random rng(seed);
    std::map<std::string, Stats> stats;
    std::vector<Task> tasks;
    tasks.reserve(n);
    for (int i = 0; i < n; ++i) tasks.emplace_back(i, 1, 1, 1);

    const long long rounds = std::max<long long>(1, kTargetOps / n);

    // ---- insert, then extract_min (random keys)
    for (long long round = 0; round < rounds; ++round) {
        for (Task& t : tasks) t.wakeUpTime = static_cast<int>(rng.index(1000000000));
        auto q = makeQueue(kind, wakeUpKey);
        measure(*q, stats["insert"], n, [&](long long i) { q->insert(&tasks[i]); });
        measure(*q, stats["extract_min"], n, [&](long long) { q->extractMin(); });
    }

    // ---- decrease_key: full queue, one extract-min first so the Fibonacci
    // heap has real trees, then make every remaining key smaller.
    for (long long round = 0; round < rounds; ++round) {
        for (Task& t : tasks) t.wakeUpTime = static_cast<int>(rng.index(1000000000));
        auto q = makeQueue(kind, wakeUpKey);
        for (Task& t : tasks) q->insert(&t);
        q->extractMin();
        std::vector<Task*> inQueue = q->tasks();
        // Random amounts are drawn before the clock starts, so only the
        // queue operation itself is timed.
        std::vector<int> amount(inQueue.size());
        for (int& a : amount) a = 1 + static_cast<int>(rng.index(500000000));
        measure(*q, stats["decrease_key"], static_cast<long long>(inQueue.size()), [&](long long i) {
            inQueue[i]->wakeUpTime -= amount[i];
            q->decreaseKey(inQueue[i]);
        });
    }

    // ---- hold: extract the earliest task and put it back with a later key
    {
        for (Task& t : tasks) t.wakeUpTime = static_cast<int>(rng.index(kHoldWindow));
        auto q = makeQueue(kind, wakeUpKey);
        for (Task& t : tasks) q->insert(&t);
        const long long holds = std::max<long long>(n, kTargetOps);
        std::vector<int> later(holds);
        for (int& a : later) a = 1 + static_cast<int>(rng.index(kHoldWindow));
        measure(*q, stats["hold"], holds, [&](long long i) {
            Task* t = q->extractMin();
            t->wakeUpTime += later[i];
            q->insert(t);
        });
    }
    return stats;
}

}  // namespace

const std::vector<std::string>& operationNames() {
    static const std::vector<std::string> names = {"insert", "extract_min", "decrease_key", "hold"};
    return names;
}

std::vector<OperationRow> runOperationBenchmark(const OperationConfig& config, const Progress& progress) {
    // Count the runs first, for the progress bar.
    std::size_t total = 0;
    for (int n : config.sizes)
        for (const std::string& queue : config.queues)
            if (queue != "sorted_list" || n <= config.sortedListMaxN) total += config.repeats;

    std::vector<OperationRow> rows;
    std::size_t done = 0;
    for (int n : config.sizes) {
        for (const std::string& queue : config.queues) {
            if (queue == "sorted_list" && n > config.sortedListMaxN) continue;

            // Every repeat uses the same seed, so the comparison counts are
            // identical and only the timing varies. We keep the median time.
            std::map<std::string, Stats> first;
            std::map<std::string, std::vector<double>> times;
            for (int r = 0; r < config.repeats; ++r) {
                auto stats = runOnce(queue, n, config.seed + static_cast<std::uint32_t>(n));
                if (r == 0) first = stats;
                for (const auto& [op, s] : stats) times[op].push_back(s.seconds);
                if (progress) progress(++done, total);
            }

            for (const std::string& op : operationNames()) {
                const Stats& s = first[op];
                std::vector<double>& t = times[op];
                std::sort(t.begin(), t.end());
                OperationRow row;
                row.queue = queue;
                row.n = n;
                row.operation = op;
                row.ops = s.ops;
                row.avgComparisons = s.ops ? double(s.sumComparisons) / s.ops : 0.0;
                row.worstComparisons = s.worstComparisons;
                row.avgNs = s.ops ? 1e9 * t[t.size() / 2] / s.ops : 0.0;
                rows.push_back(row);
            }
        }
    }
    return rows;
}

}  // namespace rtts
