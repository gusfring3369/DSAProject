// dataset.hpp
// The synthetic dataset.
//
// No public dataset is needed: task sets are generated with UUniFast.
// The full dataset from the slides is
//
//     n = 5, 10, 20 tasks
//     U = 0.50, 0.55, ..., 1.00        (11 levels)
//     1,000 sets per (n, U) pair  ->  3 * 11 * 1000 = 33,000 task sets
//
// A fixed seed makes every run reproducible. The CSV has one row per task:
//     set_id, n, target_u, task_id, T, C, D, u_i

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rtts/task.hpp"

namespace rtts {

struct TaskSetRecord {
    int setId = 0;
    int n = 0;
    double targetU = 0;
    TaskSet tasks;
};

const std::vector<int>& defaultTaskCounts();      // 5, 10, 20
const std::vector<double>& defaultULevels();      // 0.50 ... 1.00
constexpr std::uint32_t kDefaultSeed = 2026;

std::vector<TaskSetRecord> makeDataset(int setsPerPoint,
                                       const std::vector<int>& taskCounts = defaultTaskCounts(),
                                       const std::vector<double>& uLevels = defaultULevels(),
                                       std::uint32_t seed = kDefaultSeed);

void saveDatasetCsv(const std::vector<TaskSetRecord>& records, const std::string& path);
std::vector<TaskSetRecord> loadDatasetCsv(const std::string& path);

}  // namespace rtts
