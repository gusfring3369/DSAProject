// terminal_view.cpp

#include "rtts/terminal_view.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "rtts/generator.hpp"
#include "rtts/queues/queue_factory.hpp"
#include "rtts/theory.hpp"

namespace rtts::view {

static bool gColor = true;

void setColorEnabled(bool enabled) { gColor = enabled; }

static std::string wrap(const std::string& code, const std::string& s) {
    return gColor ? "\033[" + code + "m" + s + "\033[0m" : s;
}

std::string bold(const std::string& s) { return wrap("1", s); }
std::string dim(const std::string& s) { return wrap("2", s); }
std::string green(const std::string& s) { return wrap("32", s); }
std::string red(const std::string& s) { return wrap("31", s); }
std::string yellow(const std::string& s) { return wrap("33", s); }

std::string taskColor(int taskId) {
    // 256-colour palette entries that are easy to tell apart.
    static const int palette[] = {33, 208, 70, 170, 161, 37, 178, 99, 130, 245};   // same order as viewer/js/svg.js
    if (!gColor) return "";
    return "\033[38;5;" + std::to_string(palette[taskId % 10]) + "m";
}

static std::string colored(int taskId, const std::string& s) {
    return gColor ? taskColor(taskId) + s + "\033[0m" : s;
}

static std::string passFail(bool ok) { return ok ? green("PASS") : red("FAIL"); }

static std::string fmt(double x, int digits = 3) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(digits) << x;
    return ss.str();
}

// ------------------------------------------------------------------ tables

void printHeader(const std::string& title) {
    std::string line(title.size() + 4, '=');
    std::cout << "\n" << bold(line) << "\n" << bold("  " + title) << "\n" << bold(line) << "\n";
}

void printTaskTable(const TaskSet& tasks) {
    std::cout << "\n  " << bold("Task    T     C     D     u_i") << "\n";
    for (const Task& t : tasks) {
        std::cout << "  " << colored(t.id, std::string("█ ") + t.name())
                  << std::setw(7 - static_cast<int>(t.name().size())) << " "
                  << std::left << std::setw(6) << t.period << std::setw(6) << t.wcet
                  << std::setw(6) << t.deadline << std::right << fmt(t.utilization()) << "\n";
    }
    std::cout << "  " << dim("U = " + fmt(totalUtilization(tasks)) +
                             ",  hyperperiod H = " + std::to_string(hyperperiod(tasks)) + " ticks")
              << "\n";
}

void printTheory(const TaskSet& tasks) {
    const int n = static_cast<int>(tasks.size());
    const double u = totalUtilization(tasks);
    std::cout << "\n  " << bold("Module 7: theory tests") << "\n";
    std::cout << "  Liu-Layland (RM, sufficient)  U = " << fmt(u) << " <= " << fmt(liuLaylandBound(n))
              << "   " << passFail(liuLaylandTest(tasks)) << "\n";
    std::cout << "  Hyperbolic  (RM, sufficient)  PRODUCT(U_i + 1) = " << fmt(hyperbolicProduct(tasks))
              << " <= 2   " << passFail(hyperbolicTest(tasks)) << "\n";

    std::cout << "  RTA         (RM, exact)       R = ";
    for (const auto& [id, r] : responseTimes(tasks))
        std::cout << colored(id, "T" + std::to_string(id)) << ":" << (r < 0 ? red("miss") : std::to_string(r)) << " ";
    std::cout << "  " << passFail(rtaTest(tasks)) << "\n";
    std::cout << "  EDF         (exact, D = T)    U = " << fmt(u) << " <= 1   " << passFail(edfTest(tasks)) << "\n";
}

// ------------------------------------------------------------------- gantt

void printGantt(const TaskSet& tasks, const Metrics& m, int from, int to) {
    to = std::min<int>(to, static_cast<int>(m.gantt.size()));
    const int width = 60;   // ticks per block, so the chart fits a normal terminal

    std::set<std::pair<int, int>> releases(m.releases.begin(), m.releases.end());
    std::set<std::pair<int, int>> missedDeadlines;   // (deadline time, task)
    for (const Miss& miss : m.misses) missedDeadlines.insert({miss.deadline, miss.taskId});

    for (int start = from; start < to; start += width) {
        int end = std::min(to, start + width);
        std::cout << "\n";
        for (const Task& t : tasks) {
            // Row 1: the bars. A coloured block means "this task ran in this tick".
            std::cout << "  " << colored(t.id, t.name()) << std::string(5 - t.name().size(), ' ') << "|";
            for (int tick = start; tick < end; ++tick)
                std::cout << (m.gantt[tick] == t.id ? colored(t.id, "█") : dim("·"));
            std::cout << "|\n";

            // Row 2: markers. ^ = job released, X = deadline missed.
            std::cout << "       ";
            for (int tick = start; tick < end; ++tick) {
                if (missedDeadlines.count({tick, t.id})) std::cout << red("X");
                else if (releases.count({tick, t.id})) std::cout << colored(t.id, "^");
                else std::cout << " ";
            }
            std::cout << "\n";
        }
        // Time axis with a label every 10 ticks.
        std::cout << "       ";
        for (int tick = start; tick < end; ++tick) std::cout << (tick % 10 == 0 ? "|" : " ");
        std::cout << "\n       ";
        for (int tick = start; tick < end;) {
            if (tick % 10 == 0) {
                std::string label = std::to_string(tick);
                std::cout << label;
                tick += static_cast<int>(label.size());
            } else {
                std::cout << " ";
                ++tick;
            }
        }
        std::cout << "\n";
    }
    std::cout << "  " << dim("█ = running   ^ = job released   X = deadline missed   time in ticks") << "\n";
}

// ------------------------------------------------------------------ queues

static void printNode(const NodeView& node, const std::string& prefix, bool last, bool isRoot, int topId) {
    std::string label = colored(node.taskId, "T" + std::to_string(node.taskId)) +
                        dim(" key=" + std::to_string(node.key));
    if (node.taskId == topId) label += yellow("  <- top");
    if (node.marked) label += red("  (marked)");

    std::cout << "    " << prefix;
    if (!isRoot) std::cout << (last ? "└── " : "├── ");
    std::cout << label << "\n";

    std::string childPrefix = isRoot ? prefix : prefix + (last ? "    " : "│   ");
    for (std::size_t i = 0; i < node.children.size(); ++i)
        printNode(node.children[i], childPrefix, i + 1 == node.children.size(), false, topId);
}

void printQueue(const QueueView& q, const std::string& title) {
    std::cout << "  " << bold(title) << dim("  [" + queueLabel(q.kind) + "]") << "\n";
    if (q.roots.empty()) {
        std::cout << "    " << dim("(empty)") << "\n";
        return;
    }
    if (q.kind == "sorted_list" || q.kind == "rb_tree") {
        // A sorted list is simplest to read as one row. std::set hides its
        // internal tree, so the red-black tree is shown in order too.
        std::cout << "    ";
        for (std::size_t i = 0; i < q.roots.size(); ++i) {
            const NodeView& n = q.roots[i];
            std::cout << colored(n.taskId, "T" + std::to_string(n.taskId)) << dim("(" + std::to_string(n.key) + ")");
            if (i + 1 < q.roots.size()) std::cout << " -> ";
        }
        std::cout << "\n";
        return;
    }
    if (q.kind == "fibonacci")
        std::cout << "    " << dim("root list has " + std::to_string(q.roots.size()) + " tree(s)") << "\n";
    for (std::size_t i = 0; i < q.roots.size(); ++i)
        printNode(q.roots[i], "", true, true, q.topId);
}

// ----------------------------------------------------------------- metrics

void printMetrics(const Metrics& m) {
    std::cout << "\n  " << bold("Module 6: metrics") << "\n";
    std::cout << "  ticks simulated        " << m.gantt.size() << "\n";
    std::cout << "  jobs finished          " << m.jobsFinished << "\n";
    std::cout << "  deadline misses        " << (m.missCount() ? red(std::to_string(m.missCount())) : green("0")) << "\n";
    std::cout << "  context switches       " << m.contextSwitches << "\n";
    std::cout << "  CPU busy               " << fmt(m.cpuBusyPercent(), 1) << " %\n";
    std::cout << "  average response time  " << fmt(m.averageResponse(), 2) << " ticks\n";
    std::cout << "  worst response time    ";
    for (const auto& [id, r] : m.worstResponse) std::cout << colored(id, "T" + std::to_string(id)) << ":" << r << " ";
    std::cout << "\n";
    std::cout << "  queue work per tick    avg " << fmt(m.averageTickCost(), 2) << ", worst "
              << m.worstTickCost() << " comparisons\n";
}

void printVerdictTable(const VerdictReport& report) {
    std::cout << "\n  " << bold("  n     U   real U  sets   LL     Hyp    RTA    RM-sim  EDF    EDF-sim  mismatches") << "\n";
    for (const VerdictRow& r : report.rows) {
        auto pct = [](double x) { std::ostringstream s; s << std::setw(5) << std::fixed << std::setprecision(0) << 100 * x << "%"; return s.str(); };
        std::cout << "  " << std::setw(3) << r.n << "  " << fmt(r.targetU, 2) << "  " << fmt(r.realU, 2) << "  " << std::setw(5) << r.sets
                  << "  " << pct(r.liuLayland) << "  " << pct(r.hyperbolic) << "  " << pct(r.rta)
                  << "  " << pct(r.rmSim) << "   " << pct(r.edf) << "  " << pct(r.edfSim) << "    "
                  << (r.mismatches ? red(std::to_string(r.mismatches)) : green("0")) << "\n";
    }
    std::cout << "\n  " << report.totalSets << " task sets checked, "
              << (report.mismatches.empty() ? green("0 mismatches: simulator and theory agree 100%")
                                            : red(std::to_string(report.mismatches.size()) + " mismatches"))
              << "\n";
}

void printBenchmarkTable(const std::vector<BenchmarkRow>& rows) {
    std::cout << "\n  " << bold("queue            n      U     avg cmp/tick   worst cmp/tick   avg us/tick   worst us/tick") << "\n";
    for (const BenchmarkRow& r : rows) {
        std::cout << "  " << std::left << std::setw(15) << queueLabel(r.queue) << std::right
                  << std::setw(5) << r.n << "   " << fmt(r.actualU, 2)
                  << std::setw(13) << fmt(r.avgComparisons, 2) << std::setw(17) << r.worstComparisons
                  << std::setw(14) << fmt(r.avgMicros, 3) << std::setw(16) << fmt(r.worstMicros, 1) << "\n";
    }
}

void printOperationsTable(const std::vector<OperationRow>& rows) {
    // One block per operation, so each block reads like one graph.
    for (const std::string& op : operationNames()) {
        std::cout << "\n  " << bold(op) << "\n";
        std::cout << "  " << bold("queue                  N    avg cmp/op   worst cmp/op     ns/op") << "\n";
        for (const OperationRow& r : rows) {
            if (r.operation != op) continue;
            std::cout << "  " << std::left << std::setw(16) << queueLabel(r.queue) << std::right
                      << std::setw(9) << r.n << std::setw(13) << fmt(r.avgComparisons, 2)
                      << std::setw(15) << r.worstComparisons << std::setw(10) << fmt(r.avgNs, 1) << "\n";
        }
    }
}

void progressBar(std::size_t done, std::size_t total, const std::string& label) {
    // Only redraw when the percentage changes, so the bar does not slow things down.
    static int lastPercent = -1;
    int percent = static_cast<int>(100 * done / std::max<std::size_t>(total, 1));
    if (percent == lastPercent && done != total) return;
    lastPercent = percent;

    const int width = 40;
    int filled = width * percent / 100;
    std::cout << "\r  " << label << " [" << std::string(filled, '#') << std::string(width - filled, '.') << "] "
              << std::setw(3) << percent << "%  (" << done << "/" << total << ")" << std::flush;
    if (done == total) {
        std::cout << "\n";
        lastPercent = -1;
    }
}

}  // namespace rtts::view
