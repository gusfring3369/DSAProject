// main.cpp
// Command-line front end of the Real-Time Task Scheduler Simulator.
//
//   rtts demo        run one task set, print the Gantt chart, export it for the viewer
//   rtts step        walk through the simulation tick by tick, showing both heaps
//   rtts compare     binary heap vs Fibonacci heap vs sorted list on the same task set
//   rtts dataset     generate the synthetic dataset as CSV
//   rtts verdict     Module 8 over the whole dataset (simulator vs theory)
//   rtts benchmark   cost per tick of the three queues as the task count grows
//   rtts all         demo + verdict + benchmark, ready for the viewer
//
// Run "rtts help" for all options.

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtts/dataset.hpp"
#include "rtts/experiments.hpp"
#include "rtts/generator.hpp"
#include "rtts/json_export.hpp"
#include "rtts/queues/queue_factory.hpp"
#include "rtts/scheduler.hpp"
#include "rtts/terminal_view.hpp"
#include "rtts/theory.hpp"

using namespace rtts;

namespace {

// ------------------------------------------------------------ option parsing

// Options look like  --name value  or  --flag.  We keep them in a map.
struct Options {
    std::map<std::string, std::string> values;

    bool has(const std::string& name) const { return values.count(name) > 0; }
    std::string get(const std::string& name, const std::string& fallback) const {
        auto it = values.find(name);
        return it == values.end() ? fallback : it->second;
    }
    int getInt(const std::string& name, int fallback) const {
        return has(name) ? std::stoi(values.at(name)) : fallback;
    }
    double getDouble(const std::string& name, double fallback) const {
        return has(name) ? std::stod(values.at(name)) : fallback;
    }
};

Options parseOptions(int argc, char** argv, int start) {
    Options opts;
    for (int i = start; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) != 0) throw std::invalid_argument("unexpected argument '" + arg + "'");
        std::string name = arg.substr(2);
        bool hasValue = i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0;
        opts.values[name] = hasValue ? argv[++i] : "true";
    }
    return opts;
}

// The task set for demo / step / compare: the sample from the slides by
// default, or a random one if --n is given.
TaskSet chooseTaskSet(const Options& opts, std::string& title) {
    if (!opts.has("n")) {
        title = "Set 0 from the slides (T = 10, 20, 40; C = 2, 5, 12)";
        return sampleTaskSet();
    }
    int n = opts.getInt("n", 5);
    double u = opts.getDouble("u", 0.8);
    int seed = opts.getInt("seed", 1);
    Random rng(static_cast<std::uint32_t>(seed));
    title = "Random set: n = " + std::to_string(n) + ", target U = " + opts.get("u", "0.8") +
            ", seed = " + std::to_string(seed);
    return generateTaskSet(n, u, rng);
}

std::string viewerDir(const Options& opts) { return opts.get("viewer-dir", "viewer/data"); }
std::string resultsDir(const Options& opts) { return opts.get("results-dir", "results"); }

// ------------------------------------------------------------------ commands

// Runs every policy x queue combination with snapshots and writes the viewer trace.
void exportTrace(const TaskSet& tasks, const std::string& title, const Options& opts) {
    std::vector<std::unique_ptr<Simulator>> sims;
    std::vector<RunToExport> runs;
    for (Policy policy : {Policy::RM, Policy::EDF}) {
        for (const std::string& queue : allQueueKinds()) {
            sims.push_back(std::make_unique<Simulator>(tasks, policy, queue, /*recordSnapshots=*/true));
            sims.back()->run();
            runs.push_back({policy, queue, sims.back().get()});
        }
    }
    std::string path = viewerDir(opts) + "/trace.js";
    writeTraceJs(path, title, tasks, runs);
    std::cout << "\n  Viewer data written to " << path << "  ->  open viewer/index.html in a browser\n";
}

int cmdDemo(const Options& opts) {
    std::string title;
    TaskSet tasks = chooseTaskSet(opts, title);
    Policy policy = parsePolicy(opts.get("policy", "RM"));
    std::string queue = opts.get("queue", "binary");

    view::printHeader("Real-Time Task Scheduler Simulator: demo");
    std::cout << "  " << title << "\n  policy = " << policyName(policy) << ", queue = " << queueLabel(queue) << "\n";
    view::printTaskTable(tasks);
    view::printTheory(tasks);

    Simulator sim(tasks, policy, queue);
    const Metrics& m = sim.run();

    std::cout << "\n  " << view::bold("Schedule (Gantt chart)") << "\n";
    view::printGantt(sim.tasks(), m, 0, opts.getInt("ticks", static_cast<int>(m.gantt.size())));
    view::printMetrics(m);

    bool theorySaysOk = policy == Policy::RM ? rtaTest(tasks) : edfTest(tasks);
    bool simSaysOk = m.missCount() == 0;
    std::cout << "\n  " << view::bold("Module 8: verdict") << "  simulation "
              << (simSaysOk ? "schedulable" : "NOT schedulable") << ", theory "
              << (theorySaysOk ? "schedulable" : "NOT schedulable") << "  ->  "
              << (theorySaysOk == simSaysOk ? view::green("AGREE") : view::red("MISMATCH")) << "\n";

    if (!opts.has("no-export")) exportTrace(tasks, title, opts);
    return 0;
}

int cmdStep(const Options& opts) {
    std::string title;
    TaskSet tasks = chooseTaskSet(opts, title);
    Policy policy = parsePolicy(opts.get("policy", "RM"));
    std::string queue = opts.get("queue", "fibonacci");
    int delayMs = opts.getInt("delay", 0);   // 0 = wait for Enter

    Simulator sim(tasks, policy, queue, /*recordSnapshots=*/true);
    const int horizon = opts.getInt("ticks", static_cast<int>(hyperperiod(tasks)));

    view::printHeader("Step-by-step: " + policyName(policy) + " with " + queueLabel(queue));
    view::printTaskTable(tasks);
    std::cout << "\n  " << (delayMs ? "Auto-playing..." : "Press Enter for the next tick, q + Enter to stop.") << "\n";

    while (sim.time() < horizon) {
        sim.step();
        const TickSnapshot& snap = sim.snapshots().back();

        std::cout << "\n" << view::bold("---------------- tick " + std::to_string(snap.time) + " ----------------") << "\n";
        for (const std::string& e : snap.events) std::cout << "  * " << e << "\n";
        std::cout << "  " << view::dim("queue work this tick: " + std::to_string(snap.comparisons) + " comparisons") << "\n\n";
        view::printQueue(snap.sleep, "Sleep queue (key = wake-up time)");
        view::printQueue(snap.ready, std::string("Ready queue (key = ") + (policy == Policy::RM ? "period)" : "absolute deadline)"));
        view::printGantt(sim.tasks(), sim.metrics(), std::max(0, snap.time - 59), snap.time + 1);

        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        } else {
            std::string line;
            if (!std::getline(std::cin, line) || line == "q") break;
        }
    }
    sim.finish();
    view::printMetrics(sim.metrics());
    return 0;
}

int cmdCompare(const Options& opts) {
    std::string title;
    TaskSet tasks = chooseTaskSet(opts, title);
    Policy policy = parsePolicy(opts.get("policy", "EDF"));

    view::printHeader("Binary heap vs Fibonacci heap vs sorted list");
    std::cout << "  " << title << ", policy = " << policyName(policy) << "\n";
    view::printTaskTable(tasks);

    std::vector<int> reference;
    std::cout << "\n  " << view::bold("queue             avg cmp/tick   worst cmp/tick   misses   same schedule?") << "\n";
    for (const std::string& queue : allQueueKinds()) {
        Simulator sim(tasks, policy, queue);
        const Metrics& m = sim.run();
        if (reference.empty()) reference = m.gantt;
        std::cout << "  " << std::left << std::setw(18) << queueLabel(queue) << std::right
                  << std::setw(12) << std::fixed << std::setprecision(2) << m.averageTickCost()
                  << std::setw(17) << m.worstTickCost() << std::setw(9) << m.missCount() << "      "
                  << (m.gantt == reference ? view::green("yes") : view::red("NO")) << "\n";
    }
    std::cout << "\n  All three queues must give the same schedule; only the work per tick differs.\n";
    return 0;
}

int cmdDataset(const Options& opts) {
    int sets = opts.getInt("sets", 1000);
    std::string out = opts.get("out", "data/tasksets.csv");
    auto records = makeDataset(sets);
    saveDatasetCsv(records, out);
    std::cout << "  Wrote " << records.size() << " task sets (" << sets << " per (n, U) pair) to " << out << "\n";
    return 0;
}

int cmdVerdict(const Options& opts) {
    int sets = opts.getInt("sets", 1000);
    std::vector<TaskSetRecord> records =
        opts.has("in") ? loadDatasetCsv(opts.get("in", "")) : makeDataset(sets);

    view::printHeader("Module 8: verdict check over " + std::to_string(records.size()) + " task sets");
    std::cout << "  Each set runs RM and EDF with binary AND Fibonacci heaps and is compared with theory.\n\n";

    VerdictReport report = runVerdictExperiment(records, [](std::size_t d, std::size_t t) {
        view::progressBar(d, t, "checking");
    });
    view::printVerdictTable(report);

    writeVerdictCsv(resultsDir(opts) + "/verdict_summary.csv", report);
    writeMismatches(resultsDir(opts) + "/mismatches.txt", report);
    writeVerdictJs(viewerDir(opts) + "/verdict.js", report, sets);
    std::cout << "  Results: " << resultsDir(opts) << "/verdict_summary.csv, " << viewerDir(opts) << "/verdict.js\n";
    return report.mismatches.empty() ? 0 : 1;
}

int cmdBenchmark(const Options& opts) {
    BenchmarkConfig config;
    config.setsPerSize = opts.getInt("sets", config.setsPerSize);
    config.utilization = opts.getDouble("u", config.utilization);
    config.policy = parsePolicy(opts.get("policy", "EDF"));
    if (opts.has("max-n")) {
        int maxN = opts.getInt("max-n", 1000);
        std::vector<int> sizes;
        for (int n : config.sizes) if (n <= maxN) sizes.push_back(n);
        config.sizes = sizes;
    }

    view::printHeader("Benchmark: sorted list vs binary heap vs Fibonacci heap");
    std::cout << "  policy " << policyName(config.policy) << ", target U " << config.utilization << ", "
              << config.setsPerSize << " task sets per size\n\n";

    auto rows = runBenchmark(config, [](std::size_t d, std::size_t t) { view::progressBar(d, t, "running"); });
    view::printBenchmarkTable(rows);

    writeBenchmarkCsv(resultsDir(opts) + "/benchmark.csv", rows);
    writeBenchmarkJs(viewerDir(opts) + "/benchmark.js", rows, config);
    std::cout << "\n  Results: " << resultsDir(opts) << "/benchmark.csv, " << viewerDir(opts) << "/benchmark.js\n";
    return 0;
}

int cmdAll(const Options& opts) {
    cmdDemo(opts);
    int status = cmdVerdict(opts);
    cmdBenchmark(opts);
    std::cout << "\n  Everything is ready. Open viewer/index.html in a browser.\n";
    return status;
}

void printHelp() {
    std::cout << R"(
Real-Time Task Scheduler Simulator (Dual Indexed Min-Heaps: Binary and Fibonacci)

Usage:  rtts <command> [options]

Commands
  demo        Simulate one task set, print the Gantt chart, export it for the viewer
  step        Walk through the simulation tick by tick and watch both queues change
  compare     Same task set with binary heap, Fibonacci heap and sorted list
  dataset     Write the synthetic dataset (UUniFast) to a CSV file
  verdict     Module 8: simulator vs theory over the whole dataset
  benchmark   Cost per tick of each queue as the number of tasks grows
  all         demo + verdict + benchmark
  help        Show this text

Task set options (demo, step, compare)
  --n N            random set with N tasks (default: the sample set from the slides)
  --u U            target utilization for the random set (default 0.8)
  --seed S         random seed (default 1)
  --policy RM|EDF  scheduling policy
  --queue Q        binary | fibonacci | sorted_list
  --ticks K        only simulate / draw the first K ticks

Other options
  --delay MS       step: auto-play with MS milliseconds per tick instead of Enter
  --sets K         dataset/verdict: sets per (n, U) pair (default 1000);
                   benchmark: sets per size (default 5)
  --in FILE        verdict: read task sets from a CSV made by 'rtts dataset'
  --out FILE       dataset: output file (default data/tasksets.csv)
  --max-n N        benchmark: largest task count (default 1000)
  --no-color       plain text output
  --no-export      demo: do not write viewer/data/trace.js

Examples
  rtts demo
  rtts demo --policy EDF --queue fibonacci
  rtts step --queue fibonacci --policy EDF
  rtts demo --n 5 --u 0.95 --seed 42
  rtts verdict --sets 100
  rtts benchmark
)";
}

void setUpConsole() {
#ifdef _WIN32
    // Make Windows terminals understand UTF-8 box characters and ANSI colours.
    SetConsoleOutputCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

}  // namespace

int main(int argc, char** argv) {
    setUpConsole();
    if (argc < 2) {
        printHelp();
        return 0;
    }

    const std::string command = argv[1];
    try {
        Options opts = parseOptions(argc, argv, 2);
        if (opts.has("no-color")) view::setColorEnabled(false);

        if (command == "demo") return cmdDemo(opts);
        if (command == "step") return cmdStep(opts);
        if (command == "compare") return cmdCompare(opts);
        if (command == "dataset") return cmdDataset(opts);
        if (command == "verdict") return cmdVerdict(opts);
        if (command == "benchmark") return cmdBenchmark(opts);
        if (command == "all") return cmdAll(opts);
        if (command == "help" || command == "--help" || command == "-h") {
            printHelp();
            return 0;
        }
        std::cerr << "Unknown command '" << command << "'. Run 'rtts help'.\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
