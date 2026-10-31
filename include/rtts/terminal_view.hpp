// terminal_view.hpp
// Everything that draws on the terminal: task tables, the colour Gantt chart,
// heap trees, progress bars. Pure presentation, no scheduling logic in here.

#pragma once

#include <string>
#include <vector>

#include "rtts/experiments.hpp"
#include "rtts/metrics.hpp"
#include "rtts/op_benchmark.hpp"
#include "rtts/queues/task_queue.hpp"
#include "rtts/scheduler.hpp"
#include "rtts/task.hpp"

namespace rtts::view {

// Colours are on by default; --no-color turns them off (useful when the
// output is saved to a file).
void setColorEnabled(bool enabled);

std::string taskColor(int taskId);   // ANSI escape code for a task's colour
std::string bold(const std::string& s);
std::string dim(const std::string& s);
std::string green(const std::string& s);
std::string red(const std::string& s);
std::string yellow(const std::string& s);

void printHeader(const std::string& title);
void printTaskTable(const TaskSet& tasks);
void printTheory(const TaskSet& tasks);

// Gantt chart for ticks [from, to). Long charts are wrapped into blocks.
void printGantt(const TaskSet& tasks, const Metrics& m, int from, int to);

// A queue drawn as a tree (binary heap), a forest (Fibonacci heap) or a row (sorted list).
void printQueue(const QueueView& q, const std::string& title);

void printMetrics(const Metrics& m);
void printVerdictTable(const VerdictReport& report);
void printBenchmarkTable(const std::vector<BenchmarkRow>& rows);
void printOperationsTable(const std::vector<OperationRow>& rows);

void progressBar(std::size_t done, std::size_t total, const std::string& label);

}  // namespace rtts::view
