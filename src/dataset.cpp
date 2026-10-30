// dataset.cpp

#include "rtts/dataset.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

#include "rtts/generator.hpp"

namespace rtts {

const std::vector<int>& defaultTaskCounts() {
    static const std::vector<int> counts = {5, 10, 20};
    return counts;
}

const std::vector<double>& defaultULevels() {
    static const std::vector<double> levels = [] {
        std::vector<double> v;
        for (int i = 0; i <= 10; ++i) v.push_back(0.50 + 0.05 * i);
        return v;
    }();
    return levels;
}

std::vector<TaskSetRecord> makeDataset(int setsPerPoint, const std::vector<int>& taskCounts,
                                       const std::vector<double>& uLevels, std::uint32_t seed) {
    Random rng(seed);
    std::vector<TaskSetRecord> records;
    int setId = 0;
    for (int n : taskCounts)
        for (double u : uLevels)
            for (int i = 0; i < setsPerPoint; ++i)
                records.push_back({setId++, n, u, generateTaskSet(n, u, rng)});
    return records;
}

void saveDatasetCsv(const std::vector<TaskSetRecord>& records, const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path);
    out << "set_id,n,target_u,task_id,T,C,D,u_i\n";
    out << std::fixed;
    for (const auto& rec : records)
        for (const Task& t : rec.tasks)
            out << rec.setId << ',' << rec.n << ',' << std::setprecision(2) << rec.targetU << ','
                << t.id << ',' << t.period << ',' << t.wcet << ',' << t.deadline << ','
                << std::setprecision(4) << t.utilization() << '\n';
}

std::vector<TaskSetRecord> loadDatasetCsv(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read " + path + " (run './rtts dataset' first)");

    std::map<int, TaskSetRecord> bySet;   // keeps the sets sorted by id
    std::string line;
    std::getline(in, line);               // skip the header
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> f;
        while (std::getline(ss, field, ',')) f.push_back(field);
        if (f.size() < 7) continue;

        int setId = std::stoi(f[0]);
        TaskSetRecord& rec = bySet[setId];
        rec.setId = setId;
        rec.n = std::stoi(f[1]);
        rec.targetU = std::stod(f[2]);
        rec.tasks.emplace_back(std::stoi(f[3]), std::stoi(f[4]), std::stoi(f[5]), std::stoi(f[6]));
    }

    std::vector<TaskSetRecord> records;
    for (auto& entry : bySet) records.push_back(std::move(entry.second));
    return records;
}

}  // namespace rtts
