// json_export.hpp
// Writes simulation results as JavaScript files for the HTML viewer.
//
// Why .js and not .json? Browsers refuse to fetch() local .json files when a
// page is opened straight from disk (file://). A <script> tag CAN load a local
// .js file, so we write   window.RTTS_TRACE = { ...json... };   and the viewer
// works with a simple double-click, no web server needed.

#pragma once

#include <string>
#include <vector>

#include "rtts/experiments.hpp"
#include "rtts/scheduler.hpp"
#include "rtts/task.hpp"

namespace rtts {

// One simulation run to export: which policy/queue, and the finished simulator
// (it must have been created with recordSnapshots = true).
struct RunToExport {
    Policy policy;
    std::string queue;
    const Simulator* sim;
};

void writeTraceJs(const std::string& path, const std::string& title, const TaskSet& tasks,
                  const std::vector<RunToExport>& runs);

void writeVerdictJs(const std::string& path, const VerdictReport& report, int setsPerPoint);

void writeBenchmarkJs(const std::string& path, const std::vector<BenchmarkRow>& rows,
                      const BenchmarkConfig& config);

// Plain CSV versions of the experiment results (for Excel / the report).
void writeVerdictCsv(const std::string& path, const VerdictReport& report);
void writeBenchmarkCsv(const std::string& path, const std::vector<BenchmarkRow>& rows);
void writeMismatches(const std::string& path, const VerdictReport& report);

}  // namespace rtts
